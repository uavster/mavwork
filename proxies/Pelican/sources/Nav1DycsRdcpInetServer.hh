/*
 * Nav1DycsRdcpInetServer.cpp
 *
 *  Created on: 21/05/2012
 *      Author: Ignacio Mellado-Bataller
 */

#include "comm/Nav1DycsRdcpInetServer.h"

#define SEND_BEACON_FREQ	25.0
#define RECV_BEACON_FREQ	25.0
#define STATE_POLLING_FREQ	60.0

#define template_def				template<class _AutopilotT>
#define Nav1DycsRdcpInetServer_		Nav1DycsRdcpInetServer<_AutopilotT>

namespace Comm {

template_def Nav1DycsRdcpInetServer_::Nav1DycsRdcpInetServer() {
	stateReceiver.setParent(this);
#ifdef TEST_STATE_SOURCE
	testAutopilot.setParent(this);
#endif

	getTransportServer().setNewClientBeaconPeriods(1/RECV_BEACON_FREQ, 1/SEND_BEACON_FREQ);
	StaticBuffer<char, sizeof(Comm::Proto::Dycs::DycsClientToServerHeader) + sizeof(Mav::StateInfo)> connBuffer;
	bzero(&connBuffer, connBuffer.sizeBytes());
	((Comm::Proto::Dycs::DycsClientToServerHeader *)&connBuffer)->requestedAccessMode = Mav::UNKNOWN_MODE;
	getTransportServer().setNewClientDefaultSendPayload(connBuffer);
}

template_def void Nav1DycsRdcpInetServer_::open(const Comm::InetAddress &addr) {
	Nav1DycsRdcpInetServer_base::open(addr);
#ifndef TEST_STATE_SOURCE
	autopilot.setStatePollingFrequency(STATE_POLLING_FREQ);
	autopilot.addEventListener(&stateReceiver);
	autopilot.open();
	addEventListener(&autopilot);
#else
	testAutopilot.start();
#endif
}

template_def void Nav1DycsRdcpInetServer_::close() {
#ifndef TEST_STATE_SOURCE
	removeAllEventListeners();
	autopilot.removeEventListener(&stateReceiver);
	autopilot.close();
#else
	testAutopilot.stop();
#endif
	Nav1DycsRdcpInetServer_base::close();
}

template_def void Nav1DycsRdcpInetServer_::gotCommandData(const Mav::CommandData &newCommands) {
#ifndef TEST_STATE_SOURCE
//	cvg_ulong timeCode = (((cvg_ulong)newCommands.timeCodeH) << 32) | ((cvg_ulong)newCommands.timeCodeL);
//	std::cout << "t:" << timeCode << " mode:" << (cvg_int)newCommands.flyingMode << " pitch:" << newCommands.theta << " roll:" << newCommands.phi << " yaw:" << newCommands.yaw << " thrust:" << newCommands.gaz << std::endl;
	autopilot.setCommandData(newCommands);
#else
	cvg_ulong timeCode = (((cvg_ulong)newCommands.timeCodeH) << 32) | ((cvg_ulong)newCommands.timeCodeL);
	std::cout << "t:" << timeCode << " mode:" << (cvg_int)newCommands.flyingMode << " pitch:" << newCommands.theta << " roll:" << newCommands.phi << " yaw:" << newCommands.yaw << " thrust:" << newCommands.gaz << std::endl;
#endif
}

template_def void Nav1DycsRdcpInetServer_::StateInfoReceiver::gotEvent(EventGenerator &source, const Event &e) {
	const Mav::IAsyncAutopilot::GotStateInfoEvent *gse = dynamic_cast<const Mav::IAsyncAutopilot::GotStateInfoEvent *>(&e);
	if (gse != NULL) {
		parent->notifyStateInfo(gse->stateInfo);
	}
}

}

#undef template_def
#undef Nav1DycsRdcpInetServer_

#undef STATE_POLLING_FREQ
#undef SEND_BEACON_FREQ
#undef RECV_BEACON_FREQ
