/*
 * VideoServer.h
 *
 *  Created on: 31/08/2011
 *      Author: Ignacio Mellado
 */

#ifndef VIDEOSERVER_H_
#define VIDEOSERVER_H_

#define VIDEO_SERVER_PORT	11236

#include <ardrone_api.h>
#include <VP_Api/vp_api_thread_helper.h>
#include "Protocol.h"

C_RESULT videoServer_init();
void videoServer_destroy();
void videoServer_endBroadcaster();
bool_t videoServer_isStarted();
void videoServer_lockFrameBuffer();
void videoServer_unlockFrameBuffer();
void videoServer_feedFrame_BGR24(uint64_t timeCode, int8_t *buffer);
void videoServer_setOutputEncoding(VideoFormat outputEncoding);

PROTO_THREAD_ROUTINE(videoServer, data);
PROTO_THREAD_ROUTINE(videoServer_broadcaster, data);

#endif /* VIDEOSERVER_H_ */
