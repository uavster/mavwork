/*
 * ConfigServer.h
 *
 *  Created on: 14/01/2012
 *      Author: Ignacio Mellado
 */

#ifndef CONFIGSERVER_H_
#define CONFIGSERVER_H_

#define CONFIG_SERVER_PORT	11234

#include <ardrone_api.h>
#include <VP_Api/vp_api_thread_helper.h>

C_RESULT configServer_init();
void configServer_destroy();

PROTO_THREAD_ROUTINE(configServer_receptionist, data);
PROTO_THREAD_ROUTINE(configServer_roomService, data);

#endif /* CONFIGSERVER_H_ */
