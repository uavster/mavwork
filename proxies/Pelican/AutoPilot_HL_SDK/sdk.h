#ifndef SDK_
#define SDK_

/*
#define HTONS(n) (((((unsigned short)(n) & 0xFF)) << 8) | (((unsigned short)(n) & 0xFF00) >> 8))
#define NTOHS(n) (((((unsigned short)(n) & 0xFF)) << 8) | (((unsigned short)(n) & 0xFF00) >> 8))


unsigned short htons(unsigned short n);
unsigned short ntohs(unsigned short n);

#define htons(n) HTONS(n)
#define ntohs(n) NTOHS(n)

*/




extern void SDK_mainloop(void);

// COMPUTER VISION GROUP application; Charlie De Vivero 13-07-2010
// High-Level Communication Protocol (HL Com)
#define HLC_START_SEQ	0x00	// init HLC sequence, waiting for '$' or '#'
#define HLC_REQ_SEQ1	0x01	// data-request HLC sequence, waiting for '>'
#define HLC_REQ_SEQ2	0x02	/* ready HLC sequence, waiting for:
									optional flags:
									'v' - verbose mode
									request code:
									'a' - RC_DATA
									'b' - STATE
									'c' - SENSOR
									'z' - AUX		*/

#define HLC_CMD_SEQ1	0x11	// command HLC sequence, waiting for '>'
#define HLC_CMD_SEQ2	0x12	/* ready HLC sequence, waiting for:
									command code:
									'D' - Disable Control Command
									'M' - Direct Motor Command
									'N' - Nautical Command		*/
#define HLC_CMD_DISABLE	0x15	// Disable Control sequence
#define HLC_CMD_DIRECT	0x16	// Direct Motor Command sequence
#define HLC_CMD_NAUT	0x17	// Nautical Command sequence

#define HLC_REQ_RC_DATA		0x61	// init request for RC_DATA packet
#define HLC_REQ_STATE		0x62	// init request for STATE packet
#define HLC_REQ_SENSOR		0x63	// init request for SENSOR packet
#define HLC_REQ_AUX			0x7A	// init request for AUX packet


#define RC_DATA_id			0x61
#define STATE_id			0x62
#define SENSOR_id			0x63
#define AUX_id				0x7A


extern void HLC_Protocol(char);
extern short HLC_Checksum(void*, int);

char HLC_buffer[256];
//unsigned short HLC_index = 0;

#pragma pack(push, 1)

#define PACKED __attribute__((__packed__))

typedef struct HEADER  {
	char			start[2] PACKED;	// packet start sequence
	unsigned short	length	PACKED;	// payload size
	char			id	PACKED;		// message id
} PACKED HEADER;

typedef struct FOOTER {
	unsigned short 	checksum PACKED;
	char			stop[2] PACKED;
} PACKED FOOTER;

typedef struct RC_DATA {
	unsigned short	pitch_ch PACKED;   // pitch (channel 1) value
	unsigned short	roll_ch PACKED;    // roll (channel 2) value
	unsigned short	thrust_ch PACKED;  // thrust (channel 3) value
	unsigned short	yaw_ch PACKED;     // yaw (channel 4) value
	unsigned short	ser_ch PACKED;     // serial enable (channel 5) value
	unsigned short	auto_ch PACKED;    // autopilot control (channel 6) value
        // all values range [0,4095]
} PACKED RC_DATA;

typedef struct STATE {
        short	pitch_ang PACKED;
        short	roll_ang PACKED;
        unsigned short	yaw_ang PACKED;

        short	pitch_vel PACKED;
        short	roll_vel PACKED;
        short	yaw_vel PACKED;

        int		latitude PACKED;
        int		longitude PACKED;
        int		height PACKED;

        short	x_vel PACKED;
        short	y_vel PACKED;
        short	z_vel PACKED;

        short	x_acc PACKED;
        short	y_acc PACKED;
        short	z_acc PACKED;

        short	flight_mode PACKED;
} PACKED STATE ;

typedef struct SENSOR  {
        short	mag_x PACKED;
        short	mag_y PACKED;
        short	mag_z PACKED;

        short	cam_status PACKED;
        short	cam_pitch PACKED;
        short	cam_roll PACKED;

        short	battery PACKED;
        short	flight_time PACKED;
        short	cpu_load PACKED;
} PACKED SENSOR;

typedef struct AUX  {
        short	sys_flags PACKED;
        char	RC_data[10] PACKED;
        short	dummy_333Hz PACKED;
        char	motor_data[16] PACKED;
        short	battery2 PACKED;
        short	status PACKED;
        short	status2 PACKED;
} PACKED AUX ;

#pragma pack(pop)
// *** //

struct WO_SDK_STRUCT {

	unsigned char ctrl_mode;
								//0x00: "standard scientific interface" => send R/C stick commands to LL
								//0x01:	direct motor control
								//0x02: waypoint control (not yet implemented)

	unsigned char ctrl_enabled; //0x00: Control commands are ignored by LL processor
								//0x01: Control commands are accepted by LL processor

};
extern struct WO_SDK_STRUCT WO_SDK;

struct RO_RC_DATA {

	unsigned short channel[8];
	/*
	 * channel[0]: Pitch
	 * channel[1]: Roll
	 * channel[2]: Thrust
	 * channel[3]: Yaw
	 * channel[4]: Serial interface enable/disable
	 * channel[5]: manual / height control / GPS + height control
	 *
	 * range of each channel: 0..4095
	 */
};
extern struct RO_RC_DATA RO_RC_Data;

struct WO_DIRECT_MOTOR_CONTROL
{
	unsigned char pitch;
	unsigned char roll;
	unsigned char yaw;
	unsigned char thrust;

	/*
	 * commands will be directly interpreted by the mixer
	 * running on each of the motor controllers
	 *
	 * range (pitch, roll, yaw commands): 0..2000010 = - 100..+100 %
	 * range of thrust command: 0..200 = 0..100 %
	 */

};
extern struct WO_DIRECT_MOTOR_CONTROL WO_Direct_Motor_Control;

struct WO_CTRL_INPUT {	//serial commands (= Scientific Interface)
	short pitch;	//Pitch input: -2047..+2047 (0=neutral)
	short roll;		//Roll input: -2047..+2047	(0=neutral)
	short yaw;		//(=R/C Stick input) -2047..+2047 (0=neutral)
	short thrust;	//Collective: 0..4095 = 0..100%
	short ctrl;				/*control byte:
							bit 0: pitch control enabled
							bit 1: roll control enabled
							bit 2: yaw control enabled
							bit 3: thrust control enabled
							bit 4: Height control enabled
							bit 5: GPS position control enabled
							*/
};
extern struct WO_CTRL_INPUT WO_CTRL_Input;

#endif /*SDK_*/
