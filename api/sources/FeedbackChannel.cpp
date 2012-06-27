/*
 * FeedbackChannel.cpp
 *
 *  Created on: 22/08/2011
 *      Author: Ignacio Mellado
 */

#include <FeedbackChannel.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <Protocol.h>
#include <stdio.h>
#include <math.h>

namespace DroneProxy {
namespace Comm {

FeedbackChannel::FeedbackChannel()
	: cvgThread("feedbackChannel") {
	openLog("FeedbackChannel");
	setChannelName("Feedback");
	feedbackSocket = -1;
	localCommandPort = -1;
	gotFirstPacket = false;
}

FeedbackChannel::~FeedbackChannel() {
	close();
	closeLog();
}

void FeedbackChannel::open(cvgString host, cvg_int port, cvg_int portSpan) {
	close();
	setCurrentState(CONNECTING);
	try {
		feedbackSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
		if (feedbackSocket == -1)
			throw cvgException("[FeedbackChannel] Error opening socket");

		// Find a port in the range that's not being used
		cvg_uint i;
		for (i = 0; i < portSpan; i++) {
			Channel::open(host, port + i);
			// Set socket timeouts so recv does not block permanently and the worker thread may be closed
			timeval sockTimeout;
			sockTimeout.tv_sec = floor(FEEDBACK_CHANNEL_MAX_SILENT_TIME);
			sockTimeout.tv_usec = (int)(FEEDBACK_CHANNEL_MAX_SILENT_TIME * 1e6 - sockTimeout.tv_sec * 1e6);
			//setsockopt(feedbackSocket, SOL_SOCKET, SO_SNDTIMEO, &sockTimeout, sizeof(timeval));
			setsockopt(feedbackSocket, SOL_SOCKET, SO_RCVTIMEO, &sockTimeout, sizeof(timeval));
			// Init address structures
			bzero((char *)&socketAddress, sizeof(socketAddress));
			socketAddress.sin_family = AF_INET;
			socketAddress.sin_port = htons(getPort());
			socketAddress.sin_addr.s_addr = inet_addr(getHost().c_str());
			// Bind state socket to port
			if (bind(feedbackSocket, (struct sockaddr *)&socketAddress, sizeof(sockaddr_in)) == 0)
				break;
			// If the port is in use, try the next one in the span
		}
		if (i == portSpan)
			throw cvgException(cvgString("[FeedbackChannel] Unable to find a free port to bind to in the range [") +  port + ", " + (port + portSpan) + "]");

		// Start channel thread
		start();

	} catch(cvgException e) {
		close();
		setCurrentState(FAULT);
		throw e;
	}
}

void FeedbackChannel::close() {
	stop();
	setCurrentState(DISCONNECTING);
	if (feedbackSocket != -1) {
		shutdown(feedbackSocket, SHUT_RDWR);
		::close(feedbackSocket);
		feedbackSocket = -1;
	}
	gotFirstPacket = false;
	setCurrentState(DISCONNECTED);
	Channel::close();
}

void FeedbackChannel::run() {
	try {
		if (getCurrentState() != CONNECTING && getCurrentState() != CONNECTED) { usleep(100000); return; }

		cvg_bool reconnectOnError = true;

		socklen_t slen = sizeof(sockaddr_in);
		cvg_bool error = false;
		ssize_t received = 0;
		FeedbackPacket recvBuffer;
		received = recvfrom(feedbackSocket, &recvBuffer, sizeof(FeedbackPacket), 0, (struct sockaddr *)&socketAddress, &slen);
		if (received == sizeof(FeedbackPacket)) {
			FeedbackPacket_ntoh(&recvBuffer);
			if ((recvBuffer.signature & 0x7FFF) == (PROTOCOL_FEEDBACK_SIGNATURE & 0x7FFF)) {
				// If the packet is not for this client, discard it
				if (recvBuffer.commandPort == localCommandPort) {
					if (packetAccessMutex.lock()) {
						cvg_bool outOfSeq = false;
						cvg_ulong currentPacketIndex = ((((cvg_ulong)recvBuffer.indexH) << 32) | ((cvg_ulong)recvBuffer.indexL & 0xFFFFFFFF));

						if (gotFirstPacket) {
							// Check if the packet is out of sequence
							outOfSeq = currentPacketIndex <= lastPacketIndex;
						} else {
							if (!(recvBuffer.signature & 0x8000))
								controlChannel->setRemoteControlPort(socketAddress.sin_port);
						}
						gotFirstPacket = true;
						lastPacketIndex = currentPacketIndex;

						if (!outOfSeq) {
							memcpy(&feedbackPacket, &recvBuffer, sizeof(FeedbackPacket));
						}
						packetAccessMutex.unlock();
					} else { error = true; reconnectOnError = false; }
				} else { error = true; reconnectOnError = false; }
			} else error = true;
		} else error = true;

		if (!error) {
			setCurrentState(CONNECTED);
			notifyReceivedData(&feedbackPacket.feedbackData);
		} else {
			if (gotFirstPacket && reconnectOnError) setCurrentState(FAULT);
		}

		if (received <= 0) usleep(10000);
	} catch(cvgException e) {
		log(e.getMessage());
		usleep(10000);
	}
}

void FeedbackChannel::getDroneInfo(FeedbackPacket *info) {
	if (packetAccessMutex.lock()) {
		memcpy(info, &feedbackPacket, sizeof(FeedbackPacket));
		packetAccessMutex.unlock();
	}
}

}
}

