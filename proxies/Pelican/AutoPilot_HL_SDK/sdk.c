#include "main.h"
#include "sdk.h"
#include "LL_HL_comm.h"
#include "uart.h"
#include "system.h"
#include "math.h"
#include "main.h"

#define G_CONSTANT	9806.7	// NACHO [mg]

struct WO_SDK_STRUCT WO_SDK;
struct WO_CTRL_INPUT WO_CTRL_Input;
struct RO_RC_DATA RO_RC_Data;
struct WO_DIRECT_MOTOR_CONTROL WO_Direct_Motor_Control;

/* SDK_mainloop(void) is triggered @ 1kHz.
 *
 * WO_(Write Only) data is written to the LL processor after
 * execution of this function.
 *
 * RO_(Read Only) data is updated before entering this function
 * and can be read to obtain information for supervision or control
 *
 * WO_ and RO_ structs are defined in sdk.h
 *
 * The struct LL_1khz_attitude_data (defined in LL_HL_comm.h) can
 * be used to read all sensor data, results of the data fusion
 * and R/C inputs transmitted from the LL-processor. This struct is
 * automatically updated at 1 kHz.
 * */

// COMPUTER VISION GROUP application; Charlie De Vivero 13-07-2010
// High-Level Communication Protocol (HL Com)
unsigned long	timeCount		= 0;				// time keeper (1 ms per LSB)
short			controlState	= HLC_START_SEQ;	// control protocol state machine - HL Com
char			verboseMode		= 0;				// verbose mode
unsigned short	packetSize		= 0;				// used to determine command packet size
unsigned short	byteIndex		= 0;				// used to process command packets

unsigned char	ctrlMode		= 0x00;
unsigned char	ctrlEnabled		= 0x00;

//test
char ch = 0;

long long x_vel = 0, y_vel = 0;	// NACHO
unsigned int lastTicks;	// NACHO
char firstTime = 1; // NACHO
short lastAccelX, lastAccelY; // NACHO

// *** //

unsigned int getSystemTicks();	// NACHO

void SDK_mainloop(void)
{

	//WO_SDK.ctrl_mode=0x00;	//0x00: absolute angle and throttle control
							//0x01: direct motor control: WO_Direct_Motor_Control
							//		Please note that output mapping is done
							//		directly on the motor controllers

	//WO_SDK.ctrl_enabled=1;  //0: disable control by HL processor
							//1: enable control by HL processor

/*	Example for WO_SDK.ctrl_mode=0x00; */

	//WO_CTRL_Input.ctrl=0x08;	//0x08:enable throttle control only

	//WO_CTRL_Input.thrust=(RO_RC_Data.channel[2]/2);	//use R/C throttle stick input /2 to control thrust (just for testing)


/*	Example for WO_SDK.ctrl_mode=0x01;
 *
 *  Stick commands directly mapped to motors, NO attitude control!
 * */

	/*WO_Direct_Motor_Control.pitch=RO_RC_Data.channel[0]/21;
	WO_Direct_Motor_Control.roll=(4095-RO_RC_Data.channel[1])/21;
	WO_Direct_Motor_Control.thrust=RO_RC_Data.channel[2]/21;
	WO_Direct_Motor_Control.yaw=RO_RC_Data.channel[3]/21;*/

	// ^^^Original Code^^^ //

	// COMPUTER VISION GROUP; Charlie De Vivero 05-08-2010

	WO_SDK.ctrl_mode	= ctrlMode;
	WO_SDK.ctrl_enabled	= ctrlEnabled;

	// ----------- NACHO: accelerations integration [start] -----------
	// Compute the gravity contribution to the accelerometer measures and substract from the readings
	double pitch = (LL_1khz_attitude_data.angle_pitch * M_PI) / (180.0 * 1000.0);	 	// Pelican pitch axis points to the rear corner
	double roll = (LL_1khz_attitude_data.angle_roll * M_PI) / (180.0 * 1000.0);			// Pelican roll axis points left
	double g_x = -G_CONSTANT * cos(roll) * sin(pitch);	// pitch > 0 -> g_x > 0 (translation to the proxy X axis)
	double g_y = G_CONSTANT * sin(roll);				// roll > 0 -> g_y < 0  (translation to the proxy Y axis)
	// Accelerometer X readings are inverted to match the proxy reference frame
	double currentAccelX = -LL_1khz_attitude_data.acc_x - g_x;
	double currentAccelY = LL_1khz_attitude_data.acc_y - g_y;

	unsigned int currentTicks = getSystemTicks();
	if (!firstTime) {
		unsigned long long dt = currentTicks - lastTicks;
		x_vel += ((currentAccelX + lastAccelX) * 0.5) * dt;
		y_vel += ((currentAccelY + lastAccelY) * 0.5) * dt;
	} else {
		x_vel = 0;
		y_vel = 0;
		firstTime = 0;
	}
	lastTicks = currentTicks;
	lastAccelX = currentAccelX;
	lastAccelY = currentAccelY;
	// ----------- NACHO: accelerations integration [end] -----------

	/*timeCount++;



	if (timeCount > 1000) {


		HLC_Protocol(0);
		timeCount = 0;
		//controlState = 0;

	}*/
	// *** //
}

// COMPUTER VISION GROUP
// Pelican Command Communications - HL Comm
// Charlie De Vivero 2010
void HLC_Protocol(char control) {

	//printf(" hlc_PROTOCOL, %d \n\r", timeCount);

	/*for (unsigned short i = 0; i < 256; i++) {
		printf("%c", HLC_buffer[i]);
	}
	printf("\n\r");*/ // test code





//	if (timeCount % 100 > 50) {
	//if (timeCount % 100 ==1) {

		//printf(" Swithc control State 0X%x , control %c \n\r" ,controlState,control);
		switch (controlState) {
		case HLC_START_SEQ:
			//printf("%c start seq, 0x%x\n\r", control, controlState);
			if (control == '$')			controlState = HLC_REQ_SEQ1;
			else if (control == '#')	controlState = HLC_CMD_SEQ1;
			else						controlState = HLC_START_SEQ;
			break;

		case HLC_REQ_SEQ1:
			//printf("%c req seq 1, 0x%x\n\r", control, controlState);
			if (control == '>')	controlState = HLC_REQ_SEQ2;
			else				controlState = HLC_START_SEQ;
			break;

		case HLC_REQ_SEQ2:
			//printf("%c req seq 2, 0x%x\n\r", control, controlState);
			if (control == 'v' && verboseMode == 0) {
				controlState = HLC_REQ_SEQ2;
				verboseMode = 1;
			}
			else if (control == RC_DATA_id)	{
				if (verboseMode) {
					printf("RC_DATA -\n\r");
					printf("Pitch	(Channel 1) = %d\n\r",RO_RC_Data.channel[0]);
					printf("Roll	(Channel 2) = %d\n\r",RO_RC_Data.channel[1]);
					printf("Thrust	(Channel 3) = %d\n\r",RO_RC_Data.channel[2]);
					printf("Yaw	(Channel 4) = %d\n\r",RO_RC_Data.channel[3]);
					printf("Serial	(Channel 5) = %d\n\r",RO_RC_Data.channel[4]);
					printf("Auto	(Channel 6) = %d\n\r",RO_RC_Data.channel[5]);
				} else {
					char packet[sizeof(HEADER) + sizeof(RC_DATA) + sizeof(FOOTER)];

					HEADER *header = (HEADER *)packet;
					RC_DATA *rc_data = (RC_DATA *)&packet[sizeof(HEADER)];
					FOOTER *footer = (FOOTER *)&packet[sizeof(HEADER) + sizeof(RC_DATA)];

//					printf("On  send packets,\n\r");
					rc_data->pitch_ch = RO_RC_Data.channel[0];
//					printf("Pitch ch \n\r");
					rc_data->roll_ch	= RO_RC_Data.channel[1];
//					printf("Roll ch \n\r");
					rc_data->thrust_ch	= RO_RC_Data.channel[2];
					rc_data->yaw_ch		= RO_RC_Data.channel[3];
					rc_data->ser_ch		= RO_RC_Data.channel[4];
					rc_data->auto_ch	= RO_RC_Data.channel[5];
//					printf("auto ch \n\r");

//					printf("Header packets, 0x%x\n\r", control, controlState);
					//strcpy(header.start,"$>");//header->start	= "$>";
					header->start[0] = '$';
					header->start[1] = '>';
					header->length	= (short)sizeof(RC_DATA);
					header->id		= RC_DATA_id;

//					printf("footer packets, 0x%x\n\r", control, controlState);
					footer->checksum	= HLC_Checksum((char *)&rc_data, sizeof(RC_DATA));
					footer->stop[0] = '<';
					footer->stop[1] = '@';
					//strcpy(header.start,"<@");////footer->stop		= "<@";

					UART_SendPacket_raw(packet, sizeof(packet));

					/*
//					printf("Assembling packets, 0x%x\n\r", control, controlState);
					int index = 0;
					for (int i = 0; i < sizeof(HEADER); i++)
						packet[index + i] = *((char *)&header + i);
					index += sizeof(HEADER);

					for (int i = 0; i < sizeof(RC_DATA); i++)
						packet[index + i] = *((char *)&rc_data + i);
					index += sizeof(RC_DATA);

					for (int i = 0; i < sizeof(FOOTER); i++)
						packet[index + i] = *((char *)&footer + i);
					index += sizeof(FOOTER);

//					printf("before send packets, 0x%x\n\r", control, controlState);
					UART_SendPacket_raw(packet, index);
//					printf("after send packets, 0x%x\n\r", control, controlState);
*/
				}
				verboseMode = 0;
				controlState = HLC_START_SEQ;
			}
			else if (control == STATE_id) {
				if (verboseMode) {
					/*printf("STATE -\n\r");
					printf("Pitch		(deg)	= %f\n\r",LL_1khz_attitude_data.angle_pitch * 0.01);
					printf("Roll		(deg)	= %f\n\r",LL_1khz_attitude_data.angle_roll * 0.01);
					printf("Yaw		(deg)	= %f\n\r",LL_1khz_attitude_data.angle_yaw * 0.01);
					printf("Pitch Vel	(deg/s) = %f\n\r",LL_1khz_attitude_data.angvel_pitch * 0.015);
					printf("Roll Vel	(deg/s) = %f\n\r",LL_1khz_attitude_data.angvel_roll * 0.015);
					printf("Yaw Vel		(deg/s) = %f\n\r",LL_1khz_attitude_data.angvel_yaw * 0.015);
					printf("Latitude		= %d\n\r",LL_1khz_attitude_data.latitude_best_estimate);
					printf("Longitude		= %d\n\r",LL_1khz_attitude_data.longitude_best_estimate);
					printf("Height		(m)	= %f\n\r",LL_1khz_attitude_data.height * 0.001);
					printf("X Vel			= %d\n\r",LL_1khz_attitude_data.speed_x_best_estimate);
					printf("Y Vel			= %d\n\r",LL_1khz_attitude_data.speed_y_best_estimate);
					printf("Z Vel		(m/s)	= %f\n\r",LL_1khz_attitude_data.dheight * 0.001);
					printf("X Acc		(g)	= %f\n\r",LL_1khz_attitude_data.acc_x * 0.001);
					printf("Y Acc		(g)	= %f\n\r",LL_1khz_attitude_data.acc_y * 0.001);
					printf("Z Acc		(g)	= %f\n\r",LL_1khz_attitude_data.acc_z * 0.001);
					printf("Flight mode		= %d\n\r",LL_1khz_attitude_data.flightMode);
					*/
				} else {
					char packet[sizeof(HEADER) + sizeof(STATE) + sizeof(FOOTER)];

					HEADER *header = (HEADER *)packet;
					STATE *state = (STATE *)&packet[sizeof(HEADER)];
					FOOTER *footer = (FOOTER *)&packet[sizeof(HEADER) + sizeof(STATE)];

					state->pitch_ang	= LL_1khz_attitude_data.angle_pitch;
					state->roll_ang		= LL_1khz_attitude_data.angle_roll;
					state->yaw_ang		= LL_1khz_attitude_data.angle_yaw;
					state->pitch_vel	= LL_1khz_attitude_data.angvel_pitch;
					state->roll_vel		= LL_1khz_attitude_data.angvel_roll;
					state->yaw_vel		= LL_1khz_attitude_data.angvel_yaw;
					state->latitude		= LL_1khz_attitude_data.latitude_best_estimate; //LL_1khz_control_input.latitude;
					state->longitude	=     LL_1khz_attitude_data.longitude_best_estimate; // LL_1khz_control_input.longitude;
					state->height		= LL_1khz_attitude_data.height;  // LL_1khz_control_input.height;
					state->x_vel		= (short)(x_vel / peripheralClockFrequency()); // LL_1khz_attitude_data.speed_x_best_estimate;
					state->y_vel		= (short)(y_vel / peripheralClockFrequency()); // LL_1khz_attitude_data.speed_y_best_estimate;
					state->z_vel		= LL_1khz_attitude_data.dheight;
					state->x_acc		= LL_1khz_attitude_data.acc_x;
					state->y_acc		= LL_1khz_attitude_data.acc_y;
					state->z_acc		= LL_1khz_attitude_data.acc_z;
					state->flight_mode	= LL_1khz_attitude_data.flightMode;

					header->start[0] = '$';
					header->start[1] = '>';
					//strcpy(header.start,"$>");//header.start	= "$>";
					header->length	= (short)sizeof(STATE);
					header->id		= STATE_id;

					footer->checksum	= HLC_Checksum((char *)&state, sizeof(STATE));
					footer->stop[0] = '<';
					footer->stop[1] = '@';
					//strcpy(footer.stop,"<@");//footer.stop		= "<@";

					UART_SendPacket_raw(packet, sizeof(packet));

/*					int index = 0;
					for (int i = 0; i < sizeof(HEADER); i++)
						packet[index + i] = *((char *)&header + i);
					index += sizeof(HEADER);

					for (int i = 0; i < sizeof(STATE); i++)
						packet[index + i] = *((char *)&state + i);
					index += sizeof(STATE);

					for (int i = 0; i < sizeof(FOOTER); i++)
						packet[index + i] = *((char *)&footer + i);
					index += sizeof(FOOTER);

					UART_SendPacket_raw(packet, index);
*/

				}
				verboseMode = 0;
				controlState = HLC_START_SEQ;
			}
			else if (control == SENSOR_id) {
				if (verboseMode) {
					/*printf("SENSOR -\n\r");
					printf("Mag X Reading	= %d\n\r",LL_1khz_attitude_data.mag_x);
					printf("Mag Y Reading	= %d\n\r",LL_1khz_attitude_data.mag_y);
					printf("Mag Z Reading	= %d\n\r",LL_1khz_attitude_data.mag_z);
					printf("Camera Status	= %d\n\r",LL_1khz_attitude_data.cam_status);
					printf("Camera Pitch	= %d\n\r",LL_1khz_attitude_data.cam_status);
					printf("Camera Roll	= %d\n\r",LL_1khz_attitude_data.cam_angle_roll);
					printf("Battery (V)	= %f\n\r",LL_1khz_attitude_data.battery_voltage1 * 0.001);
					printf("Flight Time	= %d\n\r",LL_1khz_attitude_data.flight_time);
					printf("CPU Load	= %d\n\r",LL_1khz_attitude_data.cpu_load);
						*/
				} else {
					char packet[sizeof(HEADER) + sizeof(SENSOR) + sizeof(FOOTER)];

					HEADER *header = (HEADER *)packet;
					SENSOR *sensor = (SENSOR *)&packet[sizeof(HEADER)];
					FOOTER *footer = (FOOTER *)&packet[sizeof(HEADER) + sizeof(SENSOR)];

					sensor->mag_x		= LL_1khz_attitude_data.mag_x;
					sensor->mag_y		= LL_1khz_attitude_data.mag_y;
					sensor->mag_z		= LL_1khz_attitude_data.mag_z;
					sensor->cam_status	= LL_1khz_attitude_data.cam_status;
					sensor->cam_pitch	= LL_1khz_attitude_data.cam_angle_pitch;
					sensor->cam_roll	= LL_1khz_attitude_data.cam_angle_roll;
					sensor->battery		= LL_1khz_attitude_data.battery_voltage1;
					sensor->flight_time	= LL_1khz_attitude_data.flight_time;
					sensor->cpu_load	= LL_1khz_attitude_data.cpu_load;

					//strcpy(header.start,"$>");//header.start	= "$>";
					header->start[0] = '$';
					header->start[1] = '>';
					header->length	= (short)sizeof(SENSOR);
					header->id		= SENSOR_id;

					footer->checksum	= HLC_Checksum((char *)&sensor, sizeof(SENSOR));
					footer->stop[0] = '<';
					footer->stop[1] = '@';
					//strcpy(footer.stop,"<@");//footer.stop		= "<@";

					UART_SendPacket_raw(packet, sizeof(packet));

					/*
					int index = 0;
					for (int i = 0; i < sizeof(HEADER); i++)
						packet[index + i] = *((char *)&header + i);
					index += sizeof(HEADER);

					for (int i = 0; i < sizeof(SENSOR); i++)
						packet[index + i] = *((char *)&sensor + i);
					index += sizeof(SENSOR);

					for (int i = 0; i < sizeof(FOOTER); i++)
						packet[index + i] = *((char *)&footer + i);
					index += sizeof(FOOTER);

//
					UART_SendPacket_raw(packet, index);
*/
				}
				verboseMode = 0;
				controlState = HLC_START_SEQ;
			}
			else if (control == AUX_id) {
				if (verboseMode) {
				/*	printf("AUX -\n\r");
					printf("System Flags	= %d\n\r",LL_1khz_attitude_data.system_flags);
					printf("RC Data		= %d\n\r",LL_1khz_attitude_data.RC_data[0]);
					printf("Dummy 333Hz	= %d\n\r",LL_1khz_attitude_data.dummy_333Hz_1);
					printf("Motor Data	= %d\n\r",LL_1khz_attitude_data.motor_data[0]);
					printf("Battery2 (V)	= %f\n\r",LL_1khz_attitude_data.battery_voltage2 * 0.001);
					printf("Status		= %d\n\r",LL_1khz_attitude_data.status);
					printf("Status2		= %d\n\r",LL_1khz_attitude_data.status2);
						*/
				} else {
					char packet[sizeof(HEADER) + sizeof(AUX) + sizeof(FOOTER)];

					HEADER *header = (HEADER *)packet;
					AUX *aux = (AUX *)&packet[sizeof(HEADER)];
					FOOTER *footer = (FOOTER *)&packet[sizeof(HEADER) + sizeof(AUX)];

					aux->sys_flags		= LL_1khz_attitude_data.system_flags;
					for (int i = 0; i < sizeof(aux->RC_data); i++)
						aux->RC_data[i]	= LL_1khz_attitude_data.RC_data[i];
					aux->dummy_333Hz	= LL_1khz_attitude_data.dummy_333Hz_1;
					for (int i = 0; i < sizeof(aux->motor_data); i++)
						aux->motor_data[i] = LL_1khz_attitude_data.motor_data[i];
					aux->battery2		= LL_1khz_attitude_data.battery_voltage2;
					aux->status			= LL_1khz_attitude_data.status;
					aux->status2		= LL_1khz_attitude_data.status2;



					//strcpy(header.start,"$>");
					header->start[0] = '$';
					header->start[1] = '>';
					//header.start	= "$>";
					header->length	= (short)sizeof(AUX);
					header->id		= AUX_id;

					footer->checksum	= HLC_Checksum((char *)&aux, (int)sizeof(AUX));
					//footer.stop		= "<@";
					//strcpy(footer.stop,"<@");
					footer->stop[0] = '<';
					footer->stop[1] = '@';

					UART_SendPacket_raw(packet, sizeof(packet));
/*
					int index = 0;
					for (int i = 0; i < sizeof(HEADER); i++)
						packet[index + i] = *((char *)&header + i);
					index += sizeof(HEADER);

					for (int i = 0; i < sizeof(AUX); i++)
						packet[index + i] = *((char *)&aux + i);
					index += sizeof(AUX);

					for (int i = 0; i < sizeof(FOOTER); i++)
						packet[index + i] = *((char *)&footer + i);
					index += sizeof(FOOTER);

//
					UART_SendPacket_raw(packet, index);
//*/
				}
				verboseMode = 0;
				controlState = HLC_START_SEQ;
			}
			else
				controlState = HLC_START_SEQ;
			break;

		case HLC_CMD_SEQ1:
			//printf("%c cmd seq 1, 0x%x\n\r", control, controlState);

			if (control == '>')	controlState = HLC_CMD_SEQ2;
			else				controlState = HLC_START_SEQ;
			break;

		case HLC_CMD_SEQ2:
			//printf("%c cmd seq 2, 0x%x\n\r", control, controlState);

			if (control == 'D') {
				WO_SDK.ctrl_enabled = 0x00;
				controlState = HLC_START_SEQ;
			}
			else if (control == 'N') {
			//	printf("Pch: 0x%x\n\r", WO_CTRL_Input.pitch);
			//	printf("Rol: 0x%x\n\r", WO_CTRL_Input.roll);
			//	printf("Yaw: 0x%x\n\r", WO_CTRL_Input.yaw);
			//	printf("Thr: 0x%x\n\r", WO_CTRL_Input.thrust);

				controlState = HLC_CMD_NAUT;
			}
			else
				controlState = HLC_START_SEQ;

			byteIndex = 0;
			break;

		case HLC_CMD_NAUT:
			//printf("%c cmd nat, 0x%x\n\r", control, controlState);

			packetSize = 9;

			ctrlEnabled	= 0x01;
			ctrlMode	= 0x00;


			WO_CTRL_Input.ctrl	= byteIndex == 0 ? (unsigned char)control : WO_CTRL_Input.ctrl;

			*((char *)&WO_CTRL_Input.pitch + 0)	= byteIndex == 1 ? (unsigned char)control : *((char *)&WO_CTRL_Input.pitch + 0);
			*((char *)&WO_CTRL_Input.pitch + 1)	= byteIndex == 2 ? (unsigned char)control : *((char *)&WO_CTRL_Input.pitch + 1);

			*((char *)&WO_CTRL_Input.roll + 0)	= byteIndex == 3 ? (unsigned char)control : *((char *)&WO_CTRL_Input.roll + 0);
			*((char *)&WO_CTRL_Input.roll + 1)	= byteIndex == 4 ? (unsigned char)control : *((char *)&WO_CTRL_Input.roll + 1);

			*((char *)&WO_CTRL_Input.yaw + 0)	= byteIndex == 5 ? (unsigned char)control : *((char *)&WO_CTRL_Input.yaw + 0);
			*((char *)&WO_CTRL_Input.yaw + 1)	= byteIndex == 6 ? (unsigned char)control : *((char *)&WO_CTRL_Input.yaw + 1);

			*((char *)&WO_CTRL_Input.thrust + 0)= byteIndex == 7 ? (unsigned char)control : *((char *)&WO_CTRL_Input.thrust + 0);
			*((char *)&WO_CTRL_Input.thrust + 1)= byteIndex == 8 ? (unsigned char)control : *((char *)&WO_CTRL_Input.thrust + 1);

			controlState = byteIndex == packetSize ? HLC_START_SEQ : HLC_CMD_NAUT;

			if( WO_CTRL_Input.pitch> 2000)
			{
				WO_CTRL_Input.pitch= 2000;
			}
			if( WO_CTRL_Input.pitch< -2000)
			{
				WO_CTRL_Input.pitch= -2000;
			}

			if( WO_CTRL_Input.roll> 2000)
			{
					WO_CTRL_Input.roll= 2000;
			}
			if( WO_CTRL_Input.roll< -2000)
			{
				WO_CTRL_Input.roll= -2000;
			}

			if( WO_CTRL_Input.yaw> 1500)
			{
					WO_CTRL_Input.yaw= 1500;
			}
			if( WO_CTRL_Input.yaw< -1500)
			{
				WO_CTRL_Input.yaw= -1500;
			}

			if( WO_CTRL_Input.thrust> 3200)
			{
				WO_CTRL_Input.thrust= 3200;
			}
			if( WO_CTRL_Input.thrust<0)
			{
				WO_CTRL_Input.thrust= 0;
			}

			//WO_CTRL_Input.roll=ntohs(WO_CTRL_Input.roll);
		//	WO_CTRL_Input.yaw=ntohs(WO_CTRL_Input.yaw);
		//	WO_CTRL_Input.thrust=ntohs(WO_CTRL_Input.thrust);

			byteIndex++;
			break;
		}

//	}
}

short HLC_Checksum(void *data, int size) {
	int chk = 0;
	for (int i = 0; i < size; i++)
	{
		chk += *((char *)data + i);
	}

	return (short)(chk & 0xFFFF);
}
// *** //
