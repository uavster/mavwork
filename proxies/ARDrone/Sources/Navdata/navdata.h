#ifndef _NAVDATA_H_
#define _NAVDATA_H_

#define NAVDATA_MAX_TIME_NO_DRONE_UPDATES	0.25

#include <ardrone_tool/Navdata/ardrone_navdata_client.h>

PROTO_THREAD_ROUTINE(feedbackClient, data);

#endif // _NAVDATA_H_
