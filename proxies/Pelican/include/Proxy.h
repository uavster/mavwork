/*
 * Proxy.h
 *
 *  Created on: 22/05/2012
 *      Author: Ignacio Mellado-Bataller
 */

#ifndef PROXY_H_
#define PROXY_H_

#include "comm/Nav1DycsRdcpInetServer.h"
#include "mav/SyncToAsyncAutopilot_Blocking_ThreadUnsafe.h"
#include "mav/pelican/PelicanAutopilot.h"

#include "comm/Vid1TcpInetServer.h"
#include "cam/camueye/CamUeye.h"
#include "cam/FrameGrabber.h"
#include "cam/hokuyo/HokuyoCam.h"

#define PROXY_NAVSERVER_ADDRESS		Comm::InetAddress("0.0.0.0:11235")
#define PROXY_VIDEOSERVER_ADDRESS	Comm::InetAddress("0.0.0.0:11236")
#define PROXY_RANGESERVER_ADDRESS	Comm::InetAddress("0.0.0.0:11238")

// Navigation communications protocol stack
#define PROXY_NAV_COMM_SUITE		Comm::Nav1DycsRdcpInetServer
// Autopilot technology
#define PROXY_AUTOPILOT				Mav::SyncToAsyncAutopilot_Blocking_ThreadUnsafe<Mav::PelicanAutopilot>

// Video communications protocol stack
#define PROXY_VIDEO_COMM_SUITE		Comm::Vid1TcpInetServer
// Camera technology
#define PROXY_CAMERA				Video::CamUeye

// Depth camera communications protocol stack
#define PROXY_RANGER_COMM_SUITE		Comm::Vid1TcpInetServer
// 3D ranger technology (depth camera)
#define PROXY_RANGER				Video::HokuyoCam

class Proxy {
private:
	PROXY_NAV_COMM_SUITE<PROXY_AUTOPILOT> navServer;
	PROXY_VIDEO_COMM_SUITE videoServer;
	Video::FrameGrabber<PROXY_CAMERA> bottomCamera;

	PROXY_RANGER_COMM_SUITE rangeServer;
	Video::FrameGrabber<PROXY_RANGER> ranger;

public:
	void open();
	void close();
};

#endif /* PROXY_H_ */
