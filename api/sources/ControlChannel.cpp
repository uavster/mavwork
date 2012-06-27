/*
 * ControlChannel.cpp
 *
 *  Created on: 15/08/2011
 *      Author: nacho
 *      Notes:	It uses the portable Atlante library. However, an early
 *      version is used and some features are implemented directly with
 *      calls that are not platform independent, i.e. sockets.
 */
#include <stdio.h>
#include <ControlChannel.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <math.h>

namespace DroneProxy {
namespace Comm {

ControlChannel::ControlChannel()
	: cvgThread("controlChannel") {
	openLog("ControlChannel");
	setChannelName("Control");
	controlSocket = -1;
	commandPacket.feedbackPort = 0;
	packetIndex = 0;
//	lastCommandedFlyingMode = INIT;
//	updateCommandedFlyingModeAfterRecovery = false;
	resetCommandPacket();
}

ControlChannel::~ControlChannel() {
	close();
	closeLog();
}

void ControlChannel::resetCommandPacket() {
	commandPacket.controlData.flyingMode = INIT;
	timeval commandTime;
	gettimeofday(&commandTime, NULL);
	cvg_ulong timeCode = (cvg_ulong)commandTime.tv_sec * 1e6 + (cvg_ulong)commandTime.tv_usec;
	commandPacket.controlData.timeCodeH = (unsigned int)(timeCode >> 32);
	commandPacket.controlData.timeCodeL = (unsigned int)(timeCode & 0xFFFFFFFF);
	commandPacket.controlData.phi = 0.0f;
	commandPacket.controlData.theta = 0.0f;
	commandPacket.controlData.gaz = 0.0f;
	commandPacket.controlData.yaw = 0.0f;
}

void ControlChannel::open(cvgString host, cvg_int port, AccessMode access, cvg_char requestedPriority, cvg_float resendFreq) {
	close();
	setCurrentState(CONNECTING);
	try {
		Channel::open(host, port);
		loopWaitTime = 1.0 / resendFreq;
		controlSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
		if (controlSocket == -1)
			throw cvgException("[ControlChannel] Error opening socket");
		bzero((char *)&socketAddress, sizeof(socketAddress));
		socketAddress.sin_family = AF_INET;
		socketAddress.sin_port = htons(port);
		socketAddress.sin_addr.s_addr = inet_addr(host.c_str());
		remoteControlPort = socketAddress.sin_port;
		resetCommandPacket();
		commandPacket.controlData.requestedAccessMode = access;
		commandPacket.controlData.requestedPriority = requestedPriority;
		grantedAccessMode = UNKNOWN_MODE;
		// This call is not necessary to send() on a UDP socket, but we require the OS to assign a port number now,
		// so other channels can know it before the ControlChannel thread starts to send packets.
/*		struct sockaddr_in sin;
		bzero((char *)&sin, sizeof(sin));
		sin.sin_family = AF_INET;
		sin.sin_port = htons(0);
		sin.sin_addr.s_addr = INADDR_ANY;
		socklen_t len = sizeof(sin);
		if (connect(controlSocket, (struct sockaddr *)&sin, len) == -1)
			throw cvgException("[ControlChannel] Unable to connect UDP socket");*/
		socklen_t len = sizeof(struct sockaddr_in);
		if (connect(controlSocket, (const sockaddr *)&socketAddress, len) == -1)
			throw cvgException("[ControlChannel] Unable to connect UDP socket");
		start();
	} catch(cvgException e) {
		close();
		setCurrentState(FAULT);
		throw e;
	}
}

void ControlChannel::close() {
	stop();
	setCurrentState(DISCONNECTING);
	if (controlSocket != -1) {
		shutdown(controlSocket, SHUT_RDWR);
		::close(controlSocket);
		controlSocket = -1;
	}
	setCurrentState(DISCONNECTED);
	Channel::close();
}

void ControlChannel::setControlData(cvg_int changes, cvg_float phi, cvg_float theta, cvg_float gaz, cvg_float yaw) {
	if (grantedAccessMode != COMMANDER) return;
	// Delay test: the timestamp is added as soon as the method is called
	timeval commandTime;
	gettimeofday(&commandTime, NULL);
	cvg_ulong timeCode = (cvg_ulong)commandTime.tv_sec * 1e6 + (cvg_ulong)commandTime.tv_usec;
	if (packetAccessMutex.lock()) {
		commandPacket.controlData.timeCodeH = (unsigned int)(timeCode >> 32);
		commandPacket.controlData.timeCodeL = (unsigned int)(timeCode & 0xFFFFFFFF);
		if (changes & PHI) commandPacket.controlData.phi = phi;
		if (changes & THETA) commandPacket.controlData.theta = theta;
		if (changes & GAZ) commandPacket.controlData.gaz = gaz;
		if (changes & YAW) commandPacket.controlData.yaw = yaw;
		packetReadyCondition.signal();
		packetAccessMutex.unlock();
	}
}

void ControlChannel::setControlData(cvg_float phi, cvg_float theta, cvg_float gaz, cvg_float yaw) {
	setControlData(CVG_LITERAL_INT(-1), phi, theta, gaz, yaw);
}

void ControlChannel::getControlData(CommandPacket *cmdPacket) {
	if (packetAccessMutex.lock()) {
		memcpy(cmdPacket, &commandPacket, sizeof(CommandPacket));
		packetAccessMutex.unlock();
	}
}

void ControlChannel::setFlyingMode(FlyingMode m) {
	if ((m != INIT && m != IDLE) && grantedAccessMode != COMMANDER) return;
	if (packetAccessMutex.lock()) {
		if (m == HOVER || (m == MOVE && commandPacket.controlData.flyingMode != MOVE)) {
			// If setting move mode or hover mode, reset all parameters
			resetCommandPacket();
		}
		timeval commandTime;
		gettimeofday(&commandTime, NULL);
		cvg_ulong timeCode = (cvg_ulong)commandTime.tv_sec * 1e6 + (cvg_ulong)commandTime.tv_usec;
		commandPacket.controlData.timeCodeH = (unsigned int)(timeCode >> 32);
		commandPacket.controlData.timeCodeL = (unsigned int)(timeCode & 0xFFFFFFFF);
		commandPacket.controlData.flyingMode = m;
//		lastCommandedFlyingMode = m;
		packetReadyCondition.signal();
		packetAccessMutex.unlock();
	}
}

FlyingMode ControlChannel::getFlyingMode() {
	if (!packetAccessMutex.lock())
		throw cvgException("[ControlChannel] Unable to lock packetAccessMutex");
	FlyingMode fm = (FlyingMode)commandPacket.controlData.flyingMode;
	packetAccessMutex.unlock();
	return fm;
}

void ControlChannel::run() {
static DroneProxy::Timing::Timer pTimer;
	try {
		if (getCurrentState() != CONNECTED && getCurrentState() != CONNECTING) { usleep(100000); return; }

		cvg_bool timeout = false;
		if (packetAccessMutex.lock()) {
			timeout = !packetReadyCondition.wait(&packetAccessMutex, loopWaitTime);
			packetAccessMutex.unlock();
		} else usleep(loopWaitTime);

		CommandPacket tmpCmdPacket;
		getControlData(&tmpCmdPacket);
		// Don't send packets until the feedback port is set
		if (tmpCmdPacket.feedbackPort != 0) {
/*			if (timeout) {
//				timeval commandTime;
//				gettimeofday(&commandTime, NULL);
//				cvg_ulong timeCode = (cvg_ulong)commandTime.tv_sec * 1e6 + (cvg_ulong)commandTime.tv_usec;

				tmpCmdPacket.controlData.timeCodeH = 0; //(unsigned int)(timeCode >> 32);
				tmpCmdPacket.controlData.timeCodeL = 0; //(unsigned int)(timeCode & 0xFFFFFFFF);
			}*/
			// Set the current packet index
			tmpCmdPacket.indexH = (unsigned int)(packetIndex >> 32);
			tmpCmdPacket.indexL = (unsigned int)(packetIndex & 0xFFFFFFFF);
			packetIndex++;
			CommandPacket_hton(&tmpCmdPacket);
			(*((volatile cvg_ushort *)&socketAddress.sin_port)) = remoteControlPort;
			ssize_t sent = sendto(controlSocket, &tmpCmdPacket, sizeof(CommandPacket), MSG_NOSIGNAL, (sockaddr *)&socketAddress, sizeof(sockaddr_in));
			if (sent != sizeof(CommandPacket)) {
				setCurrentState(FAULT);
			}
			else {
				setCurrentState(CONNECTED);
				notifySentData(&tmpCmdPacket.controlData);
			}
		}
	} catch(cvgException e) {
		log(e.getMessage());
	}
}

void ControlChannel::setFeedbackPort(cvg_ushort fbPort) {
	if (!packetAccessMutex.lock())
		throw cvgException("[ControlChannel] Unable to lock packetAccessMutex");
	commandPacket.feedbackPort = fbPort;
	packetAccessMutex.unlock();
}

cvg_int ControlChannel::getLocalPort() {
	struct sockaddr_in sin;
	socklen_t len = sizeof(sin);
	if (getsockname(controlSocket, (struct sockaddr *)&sin, &len) == -1)
		throw cvgException("[ControlChannel] Error getting socket local port");
	return ntohs(sin.sin_port);
}

void ControlChannel::setRemoteControlPort(cvg_ushort rcPort) {
	remoteControlPort = rcPort;
}

void ControlChannel::setGrantedAccessMode(AccessMode grantedAccess) {
	/*if (grantedAccess == COMMANDER && grantedAccessMode != COMMANDER) {
		grantedAccessMode = grantedAccess;
		// Recover last flying mode before disconnection
		setFlyingMode(lastCommandedFlyingMode);
	}
	else */
	grantedAccessMode = grantedAccess;
}

}
}
