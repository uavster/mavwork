#ifndef PELICAN_PACKETS_H
#define PELICAN_PACKETS_H

namespace Mav {
namespace Pelican {

// Atom Com telemetry packet ID's
#define RC_DATA_id	0x61
#define STATE_id	0x62
#define SENSOR_id	0x63
#define AUX_id		0x7A

// Atom Com command packet kinds
//enum CmdType {REQUEST, DISABLE, NAUTICAL};

#ifndef PACKED
#define PACKED __attribute__((__packed__))
#endif

// Telemetry packet structures
#pragma pack(1)
struct HEADER_FIXED {
    unsigned int    utc_sec;
    unsigned short  time_msec;
    char            id;

    HEADER_FIXED() : utc_sec(0), time_msec(0), id(0) {}
} PACKED;

struct HEADER {
    char            startString[2];
    unsigned short  length;
    char            id;
} PACKED;

struct HEADER_COMMAND {
    char            startString[2] ;
    char            type;
} PACKED;



struct RC_DATA {
    short   pitch_ch;   // pitch (channel 1) value
    short   roll_ch;    // roll (channel 2) value
    short   thrust_ch;  // thrust (channel 3) value
    short   yaw_ch;     // yaw (channel 4) value
    short   ser_ch;     // serial enable (channel 5) value
    short   auto_ch;    // autopilot control (channel 6) value
    // all values range [0,4095]

    RC_DATA() : pitch_ch(0), roll_ch(0), thrust_ch(0), yaw_ch(0), ser_ch(0), auto_ch(0) {}
} PACKED;

struct STATE {
    short   pitch_ang;
    short   roll_ang;
    unsigned short   yaw_ang;

    short   pitch_vel;
    short   roll_vel;
    short   yaw_vel;

    int     latitude;
    int     longitude;
    int     height;

    short   x_vel;
    short   y_vel;
    short   z_vel;

    short   x_acc;
    short   y_acc;
    short   z_acc;

    short   flight_mode;

    STATE() : pitch_ang(0), roll_ang(0), yaw_ang(0),
        pitch_vel(0), roll_vel(0), yaw_vel(0),
        latitude(0), longitude(0), height(0),
        x_vel(0), y_vel(0), z_vel(0),
        x_acc(0), y_acc(0), z_acc(0),
        flight_mode(0) {}
} PACKED;

struct SENSOR {
        short	mag_x;
        short	mag_y;
        short	mag_z;

        short	cam_status;
        short	cam_pitch;
        short	cam_roll;

        short	battery;
        short	flight_time;
        short	cpu_load;

        SENSOR() : mag_x(0), mag_y(0), mag_z(0),
            cam_status(0), cam_pitch(0), cam_roll(0),
            battery(0), flight_time(0), cpu_load(0) {}
} PACKED;

struct FOOTER {
    unsigned short  checksum;
    char            stop[2];
} PACKED;

struct AUX {
        short	sys_flags;
        char	RC_data[10];
        short	dummy_333Hz;
        char	motor_data[16];
        short	battery2;
        short	status;
        short	status2;

        AUX() : sys_flags(0), dummy_333Hz(0),
            battery2(0), status(0), status2(0) {}
} PACKED;

// Atom Com command PSEUDO-structures

struct DIRECT {
        unsigned char   pitch;
        unsigned char   roll;
        unsigned char   yaw;
        unsigned char   thrust;

        DIRECT() : pitch(0), roll(0),
            yaw(0), thrust(0) {}
} PACKED;

/* NACHO start */
#define NAUTICAL_PITCH		0x1
#define NAUTICAL_ROLL		0x2
#define NAUTICAL_YAW		0x4
#define NAUTICAL_THRUST		0x8
#define NAUTICAL_ALTITUDE	0x10
#define NAUTICAL_GPS		0x20
/* NACHO end */

struct NAUTICAL {
//        bool    pitch_en;
//        bool    roll_en;
//        bool    yaw_en;
//        bool    thrust_en;
//        bool    height_en;
//        bool    gps_en;
    char control;

                    /*  pitch_enabled,roll enable, yaw enabled, trhust enabled, heigh control, gps */
                    /* 000000  0x00 - Nothing */
                    /* 00111111  0x3F - Pitch + Roll+Yaw+Trust + Heihg +GPS*/
                    /* 00111110  0x3E - Roll+Yaw +T+ Heihg +GPS*/
                    /* 00111101  0x3D - Pitch +Yaw+Thrust + Heihg +GPS*/
                    /* 00111100  0x3C - Yaw+T Heihg +GPS*/
                    /* 00111011  0x3B - Pitch +Roll +Thrust + Heihg +GPS*/
                    /* 00111010  0x3A - Roll+T+Heihg +GPS*/
                    /* 00111001  0x39 - Pitch+Trhust+Heihg +GPS*/
                    /* 00111000  0x38 - Trhust +Heihg +GPS*/
                    /* 00110111  0x37 - Roll +Pitch+Yaw+ Heihg +GPS*/
                    /* 00110110  0x36 - Roll+Yaw++Heihg +GPS*/
                    /* 00110101  0x35 - Roll+Trhust+Heihg +GPS*/
                    /* 00110100  0x34 - Roll+Heihg +GPS*/
                    /* 00110011  0x33 - Pitch + Roll + Heihg +GPS*/
                    /* 00110010  0x32 - Roll+ Heihg +GPS*/
                    /* 00110001  0x31 - Pitch + Heihg +GPS*/
                    /* 00110000  0x30 - Heihg +GPS*/


        short   pitch;
        short   roll;
        short   yaw;
        short   thrust;

        NAUTICAL() : //pitch_en(0), roll_en(0), yaw_en(0),
            //thrust_en(0), height_en(0), gps_en(0),
            pitch(0), roll(0), yaw(0), thrust(0) {}
} PACKED;

}
}

#endif // PACKETS_H
