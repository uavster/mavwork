/*
 * Protocol.h
 *
 *  Created on: 12/08/2011
 *      Author: nacho
 */

#ifndef PROTOCOL_H_
#define PROTOCOL_H_

#ifndef __PACKED
#define __PACKED	__attribute__((__packed__))
#endif

enum AccessMode { COMMANDER = 0, LISTENER = 1, UNKNOWN_MODE = 2 };
enum FlyingMode { IDLE = 0, TAKEOFF = 1, HOVER = 2, MOVE = 3, LAND = 4, INIT = 5, EMERGENCYSTOP = 6 };

struct CommandData__ {
	unsigned char	requestedAccessMode;
	unsigned int	timeCodeH;
	unsigned int	timeCodeL;
	unsigned char	flyingMode;
	float			phi;
	float			theta;
	float			gaz;
	float			yaw;
} __PACKED;

typedef struct CommandData__ CommandData;

enum DroneState { LANDED = 0, FLYING = 1, HOVERING = 2, TAKINGOFF = 3, LANDING = 4, EMERGENCY = 5, UNKNOWN = 6 };

struct StateInfo__ {
	unsigned char	grantedAccessMode;
	unsigned int	timeCodeH;
	unsigned int	timeCodeL;
	char			commWithDrone;
	char		 	droneMode;
	int				nativeDroneState;
	float			batteryLevel;
	float			roll;
	float			pitch;
	float			yaw;
	float			altitude;
	float			speedX;
	float			speedY;
	float			speedYaw;
} __PACKED;

typedef struct StateInfo__ StateInfo;

typedef struct VideoFrameHeader__ VideoFrameHeader;

typedef enum { CONFIG_READ = 0, CONFIG_WRITE = 1, CONFIG_ACK = 2, CONFIG_NACK = 3 } ConfigMode;

typedef enum { 	CONFIGPARAM_MAX_EULER_ANGLES = 0,			/* deg */
				CONFIGPARAM_MAX_VERTICAL_SPEED = 1,			/* m/s */
				CONFIGPARAM_MAX_YAW_SPEED = 2,				/* deg/s */
				CONFIGPARAM_MAX_ALTITUDE = 3,				/* m */
				CONFIGPARAM_CAMERA_FRAMERATE = 4,			/* fps */
				CONFIGPARAM_VIDEO_ENCODING_TYPE = 5,
				CONFIGPARAM_VIDEO_ENCODING_QUALITY = 6,
				CONFIGPARAM_LEDS_ANIMATION = 7,

				CONFIGPARAM_NUM_PARAMS = 8
				} ConfigParamId;

/* Possible values for LedsAnimation's typeId member with Parrot AR.Drone */
typedef enum	{ 	LEDS_BLINK_GREEN_RED, LEDS_BLINK_GREEN, LEDS_BLINK_RED, LEDS_BLINK_ORANGE,
					LEDS_SNAKE_GREEN_RED, LEDS_FIRE, LEDS_STANDARD, LEDS_RED, LEDS_GREEN, LEDS_RED_SNAKE,
					LEDS_BLANK, LEDS_RIGHT_MISSILE, LEDS_LEFT_MISSILE, LEDS_DOUBLE_MISSILE,
					LEDS_FRONT_LEFT_GREEN_OTHERS_RED, LEDS_FRONT_RIGHT_GREEN_OTHERS_RED,
					LEDS_REAR_RIGHT_GREEN_OTHERS_RED, LEDS_REAR_LEFT_GREEN_OTHERS_RED,
					LEDS_LEFT_GREEN_RIGHT_RED, LEDS_LEFT_RED_RIGHT_GREEN, LEDS_BLINK_STANDARD
				} LedsAnimationType;

/* Structure passed as value for CONFIGPARAMS_LEDS_ANIMATION (write as an array) */
struct LedsAnimation__ {
	unsigned char	typeId;			/* Use values in LedsAnimationType enum */
	float			frequencyHz;
	unsigned int	durationSec;	/* 0 means forever */
} __PACKED;

typedef struct LedsAnimation__ LedsAnimation;

/* CONFIGURATION CHANNEL TRANSACTIONS:
 * Client reads a parameter: Client sends READ with the parameter id, server answers WRITE with the parameter value
 * Client writes a parameter: Client sends WRITE with the parameter id, server answers ACK
 */
struct ConfigHeader__ {
	unsigned short	signature;
	struct info__ {
		unsigned char	mode;			/* Read or write access */
		unsigned int	paramId;		/* Unique ID of the parameter to read/write */
		unsigned int	dataLength;		/* Mode=write: length in bytes of the supplied parameter value. Mode=read: unused. Mode=ack: actually used data length */
	} __PACKED info;
} __PACKED;

typedef struct ConfigHeader__ ConfigHeader;

void float_ntoh(float *f);
void float_hton(float *f);
void CommandPacket_ntoh(CommandPacket *p);
void CommandPacket_hton(CommandPacket *p);
void FeedbackPacket_ntoh(FeedbackPacket *p);
void FeedbackPacket_hton(FeedbackPacket *p);
void VideoFrameHeader_hton(VideoFrameHeader *p);
void VideoFrameHeader_ntoh(VideoFrameHeader *p);
void ConfigHeader_hton(ConfigHeader *p);
void ConfigHeader_ntoh(ConfigHeader *p);

void flyingModeToString(char *s, char fm);
void droneModeToString(char *s, char dm);
#endif /* PROTOCOL_H_ */
