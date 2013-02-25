/*
 * NavServer.h
 *
 *  Created on: 21/05/2012
 *      Author: Ignacio Mellado-Bataller
 */

#ifndef NAVSERVER_H_
#define NAVSERVER_H_

//#define TEST_STATE_SOURCE

#include "comm/network/inet/InetAddress.h"
#include "comm/transport/cvg/RdcpServer.h"
#include "comm/transport/inet/UdpSocket.h"
#include "comm/session/cvg/DycsServer.h"
#include "comm/application/navigation/NavigationServer.h"
#include "mav/Mav1Protocol.h"
#include "mav/IAsyncAutopilot.h"

typedef Mav::StateInfo MAV_StateInfo;
typedef Comm::NavigationServer<Comm::DycsServer<Comm::RdcpServer<Comm::UdpSocket, Comm::InetAddress> , Comm::RdcpSocket<Comm::UdpSocket, Comm::InetAddress> > , Mav::CommandData, MAV_StateInfo> Nav1DycsRdcpInetServer_base;

namespace Comm {

template<class _AutopilotT> class Nav1DycsRdcpInetServer
	: public virtual Nav1DycsRdcpInetServer_base
{
private:
#ifndef TEST_STATE_SOURCE
		_AutopilotT autopilot;
#endif

		class StateInfoReceiver : public virtual EventListener {
		private:
			Nav1DycsRdcpInetServer *parent;
		public:
			virtual ~StateInfoReceiver() {}
			void gotEvent(EventGenerator &source, const Event &e);
			inline void setParent(Nav1DycsRdcpInetServer *p) { parent = p; }
		};
		StateInfoReceiver stateReceiver;

#ifdef TEST_STATE_SOURCE
		class TestAutopilot : public virtual cvgThread, public virtual EventGenerator {
		private:
			Nav1DycsRdcpInetServer *parent;
		public:
			TestAutopilot() : cvgThread("Test sender") { }
			void run() {
				Mav::StateInfo si;
				si.timeCodeH = si.timeCodeL = 0; si.commWithDrone = 1; si.batteryLevel = 0.75;
				si.droneMode = Mav::MOVE; si.nativeDroneState = 0; si.yaw = 90; si.pitch = 0.1;
				si.roll = 0.2; si.speedX = 0.3; si.speedY = -0.4; si.altitude = 3.14;
				parent->stateReceiver.gotEvent(*this, Mav::IAsyncAutopilot::GotStateInfoEvent(si));
				usleep(33333);
			}
			inline void setParent(Nav1DycsRdcpInetServer *p) { parent = p; }
		};
		TestAutopilot testAutopilot;
#endif

public:
		Nav1DycsRdcpInetServer();
		virtual inline ~Nav1DycsRdcpInetServer() { close(); }

		void open(const Comm::InetAddress &addr);
		void close();

		virtual void gotCommandData(const Mav::CommandData &newCommands);

		inline _AutopilotT *getAutopilot() { return &autopilot; }

		inline virtual void notifyLongTimeSinceLastStateInfoUpdate(MAV_StateInfo *si) { autopilot.notifyLongTimeSinceLastStateInfoUpdate(si); }
};

}

#include "../../sources/Nav1DycsRdcpInetServer.hh"

#endif /* NAVSERVER_H_ */
