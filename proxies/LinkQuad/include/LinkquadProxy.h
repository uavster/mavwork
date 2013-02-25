/*
 * Proxy.h
 *
 *  Created on: 22/05/2012
 *      Author: Ignacio Mellado-Bataller
 */

#ifndef LINKQUADPROXY_H_
#define LINKQUADPROXY_H_

#include "comm/Nav1DycsRdcpInetServer.h"
#include "mav/linkquad/Autopilot.h"

#include "comm/Vid1TcpInetServer.h"
#include "cam/dc1394/DC1394Cam.h"
#include "cam/FrameGrabber.h"
#include "cam/AsyncVideoTranscoder.h"
//#include "cam/hokuyo/HokuyoCam.h"

#define LINKQUADPROXY_NAVSERVER_ADDRESS			Comm::InetAddress("0.0.0.0:11235")
#define LINKQUADPROXY_BOTTOMCAMSERVER_ADDRESS	Comm::InetAddress("0.0.0.0:11236")
#define LINKQUADPROXY_RANGESERVER_ADDRESS		Comm::InetAddress("0.0.0.0:11238")
#define LINKQUADPROXY_FRONTCAMSERVER_ADDRESS	Comm::InetAddress("0.0.0.0:11239")

// Navigation communications protocol stack
#define LINKQUADPROXY_NAV_COMM_SUITE		Comm::Nav1DycsRdcpInetServer
// Autopilot technology
#define LINKQUADPROXY_AUTOPILOT				Mav::Linkquad::LinkquadAutopilot

// Bottom camera video communications protocol stack
#define LINKQUADPROXY_BOTTOMCAM_COMM_SUITE	Comm::Vid1TcpInetServer
// Bottom camera technology
#define LINKQUADPROXY_BOTTOM_CAMERA			Video::DC1394Cam

// Front camera video communications protocol stack
//#define LINKQUADPROXY_FRONTCAM_COMM_SUITE	Comm::Vid1TcpInetServer
// Front camera technology
//#define LINKQUADPROXY_FRONT_CAMERA			Video::CamUeye

// Depth camera communications protocol stack
/*#define LINKQUADPROXY_RANGER_COMM_SUITE		Comm::Vid1TcpInetServer
// 3D ranger technology (depth camera)
#define LINKQUADPROXY_RANGER				Video::HokuyoCam
*/

class LinkquadProxy {
private:
	LINKQUADPROXY_NAV_COMM_SUITE<LINKQUADPROXY_AUTOPILOT> navServer;

	LINKQUADPROXY_BOTTOMCAM_COMM_SUITE bottomCamServer;
	Video::FrameGrabber<LINKQUADPROXY_BOTTOM_CAMERA> bottomCamera;
	Video::AsyncVideoTranscoder bottomCamTranscoder;

#ifdef LINKQUADPROXY_FRONTCAM_COMM_SUITE
	LINKQUADPROXY_FRONTCAM_COMM_SUITE frontCamServer;
#endif
#ifdef LINKQUADPROXY_FRONT_CAMERA
	Video::FrameGrabber<LINKQUADPROXY_FRONT_CAMERA> frontCamera;
#endif

#ifdef LINKQUADPROXY_RANGER_COMM_SUITE
	LINKQUADPROXY_RANGER_COMM_SUITE rangeServer;
#endif
#ifdef LINKQUADPROXY_RANGER
	Video::FrameGrabber<LINKQUADPROXY_RANGER> ranger;
#endif

public:
	void open();
	void close();
	void trim();
};

#endif /* PROXY_H_ */
