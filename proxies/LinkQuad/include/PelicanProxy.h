/*
 * Proxy.h
 *
 *  Created on: 22/05/2012
 *      Author: Ignacio Mellado-Bataller
 */

#ifndef PELICANPROXY_H_
#define PELICANPROXY_H_

#include "comm/Nav1DycsRdcpInetServer.h"
#include "mav/pelican/Autopilot.h"
#include "mav/SyncToAsyncAutopilot_Blocking_ThreadUnsafe.h"

#include "comm/Vid1TcpInetServer.h"
#include "cam/camueye/CamUeye.h"
#include "cam/FrameGrabber.h"
//#include "cam/hokuyo/HokuyoCam.h"

#define PELICANPROXY_NAVSERVER_ADDRESS			Comm::InetAddress("0.0.0.0:11235")
#define PELICANPROXY_BOTTOMCAMSERVER_ADDRESS	Comm::InetAddress("0.0.0.0:11236")
#define PELICANPROXY_RANGESERVER_ADDRESS		Comm::InetAddress("0.0.0.0:11238")
#define PELICANPROXY_FRONTCAMSERVER_ADDRESS		Comm::InetAddress("0.0.0.0:11239")

// Navigation communications protocol stack
#define PELICANPROXY_NAV_COMM_SUITE		Comm::Nav1DycsRdcpInetServer
// Autopilot technology
#define PELICANPROXY_AUTOPILOT				Mav::SyncToAsyncAutopilot_Blocking_ThreadUnsafe<Mav::Pelican::PelicanAutopilot>

// Bottom camera video communications protocol stack
#define PELICANPROXY_BOTTOMCAM_COMM_SUITE	Comm::Vid1TcpInetServer
// Bottom camera technology
#define PELICANPROXY_BOTTOM_CAMERA			Video::CamUeye

// Front camera video communications protocol stack
#define PELICANPROXY_FRONTCAM_COMM_SUITE	Comm::Vid1TcpInetServer
// Front camera technology
#define PELICANPROXY_FRONT_CAMERA			Video::CamUeye

// Depth camera communications protocol stack
#define PELICANPROXY_RANGER_COMM_SUITE		Comm::Vid1TcpInetServer
// 3D ranger technology (depth camera)
#define PELICANPROXY_RANGER				Video::HokuyoCam

class PelicanProxy {
private:
	PELICANPROXY_NAV_COMM_SUITE<PELICANPROXY_AUTOPILOT> navServer;

	PELICANPROXY_BOTTOMCAM_COMM_SUITE bottomCamServer;
	Video::FrameGrabber<PELICANPROXY_BOTTOM_CAMERA> bottomCamera;

	PELICANPROXY_FRONTCAM_COMM_SUITE frontCamServer;
	Video::FrameGrabber<PELICANPROXY_FRONT_CAMERA> frontCamera;

//	PROXY_RANGER_COMM_SUITE rangeServer;
//	Video::FrameGrabber<PROXY_RANGER> ranger;

public:
	void open();
	void close();
};

#endif /* PROXY_H_ */
