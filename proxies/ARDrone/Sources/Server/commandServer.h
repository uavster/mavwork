/*
 * server.h
 *
 *  Created on: 12/08/2011
 *      Author: Ignacio Mellado
 */

#ifndef SERVER_H_
#define SERVER_H_

#define COMMAND_SERVER_PORT									11235
#define COMMAND_SERVER_MAX_TIME_NO_COMMAND					1.0
#define COMMAND_SERVER_NUM_BAD_PACKETS_FOR_DISCONNECTION	100

#include <config.h>
#include <VP_Api/vp_api_thread_helper.h>
#include <ardrone_api.h>

extern C_RESULT getBrainHost(char *brainHost);

PROTO_THREAD_ROUTINE(server, data);

#endif /* SERVER_H_ */
