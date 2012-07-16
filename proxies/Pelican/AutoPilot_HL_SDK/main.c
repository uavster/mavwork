/**********************************************************
                  Header files
 **********************************************************/
#include "LPC214x.h"
#include "stdio.h"
#include "main.h"
#include "system.h"
#include "uart.h"
#include "mymath.h"
#include "hardware.h"
#include "irq.h"
#include "i2c.h"
#include "gpsmath.h"
#include "adc.h"
#include "uart.h"
#include "ssp.h"
#include "LL_HL_comm.h"
#include "sdk.h"

/* *********************************************************
               Function declarations
  ********************************************************* */

void Initialize(void);
void feed(void);
void beeper(unsigned char);

/**********************************************************
                  Global Variables
 **********************************************************/
struct HL_STATUS HL_Status;
struct IMU_RAWDATA IMU_RawData;
volatile unsigned int int_cnt=0, cnt=0, mainloop_cnt=0;
volatile unsigned char mainloop_trigger=0;
volatile unsigned int GPS_timeout=0;

extern unsigned char data_requested;
extern int ZeroDepth;

volatile unsigned int trigger_cnt=0;
unsigned int logs_per_second=0, total_logs_per_second=0;

unsigned char packets=0x00;
unsigned char packetsTemp;
unsigned int uart_cnt;
unsigned char DataOutputsPerSecond=20;

struct IMU_CALCDATA IMU_CalcData, IMU_CalcData_tmp;
struct GPS_TIME GPS_Time;
struct SYSTEM_PERMANENT_DATA SYSTEM_Permanent_Data;

// NACHO start
volatile unsigned int numTimer0Ints = 0;

unsigned int getSystemTicks() {
	return numTimer0Ints * T0MR0 + T0TC;
}
// NACHO end

void timer0ISR(void) __irq
{
  T0IR = 0x01;      //Clear the timer 0 interrupt
  IENABLE;
  trigger_cnt++;
  numTimer0Ints++;	// NACHO
  if(trigger_cnt==ControllerCyclesPerSecond)
  {
  	trigger_cnt=0;
  	HL_Status.up_time++;
  	HL_Status.cpu_load=mainloop_cnt;

  	mainloop_cnt=0;
  }

  if(mainloop_trigger<10) mainloop_trigger++;

  IDISABLE;
  VICVectAddr = 0;		// Acknowledge Interrupt
}

/**********************************************************
                       MAIN
**********************************************************/
int	main (void) {

 static int vbat1, vbat2;
 int vbat;
 static int bat_cnt=0, bat_warning=1000;
 static char bat_warning_enabled=1;

#ifdef GPS_BEEP
 static unsigned int gps_beep_cnt;
#endif

  IDISABLE;

  init();
	LL_write_init();
  beeper(OFF);

  HL_Status.up_time=0;

  printf("\n\nProgramm is running ... \n");
  printf("Processor Clock Frequency: %d Hz\n", processorClockFrequency());
  printf("Peripheral Clock Frequency: %d Hz\n", peripheralClockFrequency());

  IENABLE;

  packetsTemp=packets;


  LED(1,ON);

  GPS_init_status=GPS_STARTUP;

  while(1)
  {
      if(mainloop_trigger)
      {
     	if(GPS_timeout<ControllerCyclesPerSecond) GPS_timeout++;
	  	else if(GPS_timeout==ControllerCyclesPerSecond)
	  	{
  	 		GPS_timeout=ControllerCyclesPerSecond+1;
	  		GPS_Data.status=0;
	  		GPS_Data.numSV=0;
	  		if (GPS_init_status==GPS_STARTUP)				//no GPS packet after startup
	  		{
	  			GPS_init_status=GPS_NEEDS_CONFIGURATION;
	  		}
	  	}

        mainloop_cnt++;
        if(++bat_cnt==100) bat_cnt=0;

        //battery monitoring
        vbat1=(vbat1*29+(ADC0Read(VOLTAGE_1)*9872/579))/30;	//voltage in mV //*9872/579

		HL_Status.battery_voltage_1=vbat1;
		HL_Status.battery_voltage_2=vbat2;

		vbat=vbat1;

		if(vbat<BATTERY_WARNING_VOLTAGE)	//decide if it's really an empty battery
		{
			if(bat_warning<ControllerCyclesPerSecond*2) bat_warning++;
			else bat_warning_enabled=1;
		}
		else
		{
			if(bat_warning>10) bat_warning-=5;
			else
			{
				bat_warning_enabled=0;
				beeper(OFF);//IOCLR1 = (1<<17);	//Beeper off
			}
		}
		if(bat_warning_enabled)
		{
			if(bat_cnt>((vbat-9000)/BAT_DIV)) beeper(ON);//IOSET1 = (1<<17);	//Beeper on
			else beeper(OFF);//IOCLR1 = (1<<17);		//Beeper off
		}

#ifdef GPS_BEEP
		//GPS_Beep
		if((GPS_Data.status&0xFF)!=3)	//no lock
		{
			gps_beep_cnt++;
			if(gps_beep_cnt>=1500) gps_beep_cnt=0;
			if(gps_beep_cnt<20) beeper(ON);	//IOSET1 = (1<<17);	//Beeper on
			else if(gps_beep_cnt==40) beeper(OFF);// IOCLR1 = (1<<17); //Beeper off
		}
#endif

	    if(mainloop_trigger) mainloop_trigger--;
        mainloop();
      }
  }
  return 0;
}

void beeper (unsigned char offon)
{
	if(offon)	//beeper on
	{
		IOSET1 = (1<<17);
	}
	else
	{
		IOCLR1 = (1<<17);
	}
}

void mainloop(void)
{
    static unsigned char led_cnt=0, led_state=1;
	unsigned char * dataptr, * dataptr2;
	unsigned char t,i;

    led_cnt++;


	if((GPS_Data.status&0xFF)==0x03)
	{
		LED(0,OFF);
	}
	else
	{
	    if(led_cnt==150)
	    {
	      LED(0,ON);
	    }
	    else if(led_cnt==200)
	    {
	      led_cnt=0;
	      LED(0,OFF);
	    }
	}

	if(trigger_transmission)
	{
		if(!(IOPIN0&(1<<CTS_RADIO)))
	  	{
	  		trigger_transmission=0;
		    if(ringbuffer(RBREAD, &t, 1))
		    {
		      transmission_running=1;
		      UARTWriteChar(t);
		    }
	  	}
	}

	if(IMU_CalcData_updated)
	{
		IMU_CalcData_updated=0;
		dataptr=(unsigned char *)&IMU_CalcData;
	    dataptr2=(unsigned char *)&IMU_CalcData_tmp;
		for(i=0;i<sizeof(IMU_CalcData);i++)
		{
	         *dataptr=*dataptr2;
	         dataptr++;
	         dataptr2++;
		}
	}

	// packetsTemp=recived_packets();

 /*   if(uart_cnt++==ControllerCyclesPerSecond/DataOutputsPerSecond)
    {


      uart_cnt=0;
        if((packetsTemp&0x01))
        	if((sizeof(HL_Status))<ringbuffer(RBFREE, 0, 0))
        	{
        		UART_SendPacket(&HL_Status, sizeof(HL_Status), PD_HLSTATUS);
        		packetsTemp&=~0x01;

        	}
        if((packetsTemp&0x02)) packetsTemp&=~0x02;
        if((packetsTemp&0x08)) packetsTemp&=~0x08;
        if((packetsTemp&0x10)) packetsTemp&=~0x10;
        if((packetsTemp&0x20)) packetsTemp&=~0x20;
        if((packetsTemp&0x40)) packetsTemp&=~0x40;


        if((packetsTemp==0x04))
        	if((sizeof(IMU_CalcData))<ringbuffer(RBFREE, 0, 0))
        	{

        		UART_SendPacket(&IMU_CalcData, sizeof(IMU_CalcData), PD_IMUCALCDATA);
        		packetsTemp&=~0x04;

        	}
        if((packetsTemp==0x80))
	        if((sizeof(GPS_Data))<ringbuffer(RBFREE, 0, 0))
	        {

	        	UART_SendPacket(&GPS_Data, sizeof(GPS_Data), PD_GPSDATA);
        		packetsTemp&=~0x80;
        	}
        if (!packetsTemp)
        	packetsTemp=packets;
    }
*/
	  SDK_mainloop();

    HL2LL_write_cycle();	//write data to transmit buffer for immediate transfer to LL processor

    if (GPS_init_status==GPS_NEEDS_CONFIGURATION)		//configuration SM of GPS at startup
    {
    	GPS_configure();
    }

    if (gpsDataOkTrigger)
    {
			if(GPS_Data.horizontal_accuracy>12000) GPS_Data.status&=~0x03;
 			if(GPS_timeout>50)//(GPS_Data.status&0xFF)!=0x03)
 			{
    			if(led_state)
    			{
    				led_state=0;
    				LED(1,OFF);
    			}
				else
				{
  					LED(1,ON);
					led_state=1;
				}
 			}
			GPS_timeout=0;
			if (GPS_init_status==GPS_STARTUP) GPS_init_status=GPS_IS_CONFIGURED;		//gps config valid, if received correct packet
    		HL_Status.latitude=GPS_Data.latitude;
    		HL_Status.longitude=GPS_Data.longitude;

    }

}


