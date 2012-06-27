/*
 * Proxy.cpp
 *
 *  Created on: 22/05/2012
 *      Author: Ignacio Mellado-Bataller
 */

//#define VIDEO_SERVER_DISABLE
#define SPEEDESTIMATION_DEBUG
#define RANGE_SERVER_DISABLE

#include "Proxy.h"

void Proxy::open() {
#ifndef VIDEO_SERVER_DISABLE
	videoServer.open(PROXY_VIDEOSERVER_ADDRESS);
	videoServer.setOutputFormat(videoServer.getOutputFormat(), 0.3);
#endif
#ifndef RANGE_SERVER_DISABLE
	rangeServer.open(PROXY_RANGESERVER_ADDRESS);
	rangeServer.setOutputFormat(Comm::RAW_D16);
#endif

	navServer.open(PROXY_NAVSERVER_ADDRESS);

#ifdef SPEEDESTIMATION_DEBUG
	navServer.getAutopilot()->getAutopilot()->getSpeedEstimator()->enableDebug(true);
#ifndef VIDEO_SERVER_DISABLE
	navServer.getAutopilot()->getAutopilot()->getSpeedEstimator()->addEventListener(&videoServer);
#endif
#else
	navServer.getAutopilot()->getAutopilot()->getSpeedEstimator()->enableDebug(false);
#ifndef VIDEO_SERVER_DISABLE
	bottomCamera.addEventListener(&videoServer);
#endif
#endif
	bottomCamera.addEventListener(&navServer);

#ifndef RANGE_SERVER_DISABLE
	ranger.addEventListener(&rangeServer);
#endif

	bottomCamera.open();
#ifndef RANGE_SERVER_DISABLE
	ranger.open();
#endif
}

void Proxy::close() {
#ifdef SPEEDESTIMATION_DEBUG
	navServer.getAutopilot()->getAutopilot()->getSpeedEstimator()->removeAllEventListeners();
#endif
	bottomCamera.removeAllEventListeners();
	bottomCamera.close();
#ifndef RANGE_SERVER_DISABLE
	ranger.removeAllEventListeners();
	ranger.close();
#endif

	navServer.close();
#ifndef VIDEO_SERVER_DISABLE
	videoServer.close();
#endif
#ifndef RANGE_SERVER_DISABLE
	rangeServer.close();
#endif
}


