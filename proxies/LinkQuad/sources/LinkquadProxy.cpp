/*
 * Proxy.cpp
 *
 *  Created on: 22/05/2012
 *      Author: Ignacio Mellado-Bataller
 */

#include "LinkquadProxy.h"

//#define BOTTOM_CAM_SERVER_DISABLE

#ifndef LINKQUADPROXY_BOTTOMCAM_COMM_SUITE
#define BOTTOM_CAM_SERVER_DISABLE
#else
#define BOTTOM_CAM_SERVER_ENCODING	Comm::JPEG
#define BOTTOM_CAM_SERVER_QUALITY	0.75
#define BOTTOM_CAM_SERVER_SCALING	0.5
#endif

#ifndef LINKQUADPROXY_FRONTCAM_COMM_SUITE
#define FRONT_CAM_SERVER_DISABLE
#else
#define FRONT_CAM_SERVER_ENCODING	Comm::RAW_BGR24
#define FRONT_CAM_SERVER_QUALITY	0.1		// Only for lossy encodings like JPEG
#endif

#ifndef LINKQUADPROXY_RANGER_COMM_SUITE
#define RANGE_SERVER_DISABLE
#endif

#define SPEEDESTIMATION_DEBUG

void LinkquadProxy::open() {
	// ---- OPEN SERVERS -----
#ifndef BOTTOM_CAM_SERVER_DISABLE
	bottomCamServer.open(LINKQUADPROXY_BOTTOMCAMSERVER_ADDRESS);
	bottomCamServer.setOutputFormat(BOTTOM_CAM_SERVER_ENCODING, BOTTOM_CAM_SERVER_QUALITY, BOTTOM_CAM_SERVER_SCALING);
#endif
#ifndef FRONT_CAM_SERVER_DISABLE
	frontCamServer.open(LINKQUADPROXY_FRONTCAMSERVER_ADDRESS);
	frontCamServer.setOutputFormat(FRONT_CAM_SERVER_ENCODING, FRONT_CAM_SERVER_QUALITY);
#endif
#ifndef RANGE_SERVER_DISABLE
	rangeServer.open(LINKQUADPROXY_RANGESERVER_ADDRESS);
	rangeServer.setOutputFormat(Comm::RAW_D16);
#endif
	navServer.open(LINKQUADPROXY_NAVSERVER_ADDRESS);

	// ----- CONNECT VIDEO SOURCES TO SERVERS -----
#ifdef SPEEDESTIMATION_DEBUG
	// Enable speed estimation debug on image
	navServer.getAutopilot()->getSpeedEstimator()->enableDebug(true);

#ifndef BOTTOM_CAM_SERVER_DISABLE
	// The bottom camera video server will get the speed estimator as video source
	navServer.getAutopilot()->getSpeedEstimator()->addEventListener(&bottomCamServer);
#endif

#else
	// Disable speed estimation debug on image
	navServer.getAutopilot()->getSpeedEstimator()->enableDebug(false);

#ifndef BOTTOM_CAM_SERVER_DISABLE
	// The bottom camera video server will get the bottom camera as video source
	bottomCamTranscoder.addEventListener(&bottomCamServer);
#endif

#endif
	// The navigation server gets bottom camera images for the speed estimation module
	//bottomCamera.addEventListener(&navServer);
	bottomCamTranscoder.setOutputFeatures(Video::IVideoSource::RAW_RGB24);
	bottomCamera.addEventListener(&bottomCamTranscoder);
	bottomCamTranscoder.addEventListener(&navServer);
	bottomCamTranscoder.start();

#ifndef FRONT_CAM_SERVER_DISABLE
	frontCamera.addEventListener(&frontCamServer);
#endif

#ifndef RANGE_SERVER_DISABLE
	ranger.addEventListener(&rangeServer);
#endif

	// ----- OPEN VIDEO SOURCES -----
	// Linkquad's Point Grey camera returns some raw modes as mono
	bottomCamera.takeMonoModesAsRaw(true);
	bottomCamera.open();	// Bottom camera must be opened for speed estimation, even if the bottom video server is disabled
#ifndef RANGE_SERVER_DISABLE
	ranger.open();
#endif
#ifndef FRONT_CAM_SERVER_DISABLE
	frontCamera.setCameraId(FRONT_CAM_ID);
	frontCamera.open();
#endif
}

void LinkquadProxy::close() {
	// ----- CLOSE VIDEO SOURCES -----
#ifdef SPEEDESTIMATION_DEBUG
	navServer.getAutopilot()->getSpeedEstimator()->removeAllEventListeners();
#endif
	bottomCamera.removeAllEventListeners();
	bottomCamera.close();

	bottomCamTranscoder.removeAllEventListeners();
	bottomCamTranscoder.stop();

#ifndef RANGE_SERVER_DISABLE
	ranger.removeAllEventListeners();
	ranger.close();
#endif
#ifndef FRONT_CAM_SERVER_DISABLE
	frontCamera.removeAllEventListeners();
	frontCamera.close();
#endif

	// ----- CLOSE SERVERS -----
	navServer.close();
#ifndef BOTTOM_CAM_SERVER_DISABLE
	bottomCamServer.close();
#endif
#ifndef RANGE_SERVER_DISABLE
	rangeServer.close();
#endif
#ifndef FRONT_CAM_SERVER_DISABLE
	frontCamServer.close();
#endif
}

void LinkquadProxy::trim() {
	navServer.getAutopilot()->trim();
}
