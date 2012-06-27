/*
 * VideoChannel.cpp
 *
 *  Created on: 03/09/2011
 *      Author: Ignacio Mellado
 */

#include "VideoChannel.h"
#include <arpa/inet.h>
#include <string.h>
#include <atlante.h>
#include <math.h>
#include <netinet/tcp.h>

namespace DroneProxy {
namespace Comm {

using namespace DroneProxy::Timing;

#define DEFAULT_BUFFER_INITIAL_WIDTH	640
#define DEFAULT_BUFFER_INITIAL_HEIGHT	480
#define MAX_POSSIBLE_NUM_PLANES			3
#define MAX_POSSIBLE_BYTES_PER_PLANE	2

#define GET_WORST_CASE_LENGTH(width, height)	(width * height * MAX_POSSIBLE_NUM_PLANES * MAX_POSSIBLE_BYTES_PER_PLANE)
#define DEFAULT_BUFFER_LENGTH					GET_WORST_CASE_LENGTH(DEFAULT_BUFFER_INITIAL_WIDTH, DEFAULT_BUFFER_INITIAL_HEIGHT)

#define BUFFER_EXTRA_ALLOC_FACTOR						1.0

VideoChannel::VideoChannel()
	: cvgThread("videoChannel") {
	openLog("VideoChannel");
	setChannelName("Video");
	videoSocket = -1;
	frameBuffer = NULL;
	auxFrameBuffer = NULL;
	frameBufferLength = 0;
	gotFirstPacket = false;
	frameBufferMutex = new DroneProxy::Threading::Mutex(false);
}

VideoChannel::~VideoChannel() {
	close();
	delete frameBufferMutex;
	closeLog();
}

void VideoChannel::open(cvgString host, cvg_int port) {
	close();
	setCurrentState(CONNECTING);
	try {
		Channel::open(host, port);

		videoSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (videoSocket == -1)
			throw cvgException("[VideoChannel] Error opening socket");
		// Set linger time
		linger lingerData;
		lingerData.l_onoff = 1;
		lingerData.l_linger = CVG_LITERAL_INT(VIDEO_CHANNEL_DEFAULT_LINGER_SECONDS);
		setsockopt(videoSocket, SOL_SOCKET, SO_LINGER, &lingerData, sizeof(linger));

		// Disable Nagle's algorithm
		int flag = 1;
		if (setsockopt(videoSocket, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(int)) == -1)
			throw cvgException("[TcpSocket] Cannot disable Nagle algorithm");

/*		int sockBuffSize = 3 * 8000; //1e6 * 0.4 / 8;
		if (setsockopt(videoSocket, SOL_SOCKET, SO_SNDBUF, &sockBuffSize, sizeof(int)) == -1)
			throw cvgException("[TcpSocket] Cannot set TCP send window size");
		if (setsockopt(videoSocket, SOL_SOCKET, SO_RCVBUF, &sockBuffSize, sizeof(int)) == -1)
			throw cvgException("[TcpSocket] Cannot set TCP reception window size");
*/
		// Set socket timeouts so recv does not block permanently and the worker thread may be closed
		timeval sockTimeout;
		sockTimeout.tv_sec = floor(VIDEO_CHANNEL_MAX_FRAME_ATOMIC_SECONDS);
		sockTimeout.tv_usec = (long)(VIDEO_CHANNEL_MAX_FRAME_ATOMIC_SECONDS * 1e6 - sockTimeout.tv_sec * 1e6);
		setsockopt(videoSocket, SOL_SOCKET, SO_RCVTIMEO, &sockTimeout, sizeof(timeval));
//		setsockopt(videoSocket, SOL_SOCKET, SO_SNDTIMEO, &sockTimeout, sizeof(timeval));
		// Connect to server
		sockaddr_in socketAddress;
		bzero((char *)&socketAddress, sizeof(socketAddress));
		socketAddress.sin_family = AF_INET;
		socketAddress.sin_port = htons(port);
		socketAddress.sin_addr.s_addr = inet_addr(host.c_str());
		if (connect(videoSocket, (sockaddr *)&socketAddress, sizeof(sockaddr_in)) != 0) {
			throw cvgException("[VideoChannel] Cannot connect to server");
		}
		frameBuffer = new cvg_char[DEFAULT_BUFFER_LENGTH];
		auxFrameBuffer = new cvg_char[DEFAULT_BUFFER_LENGTH];
		frameBufferLength = DEFAULT_BUFFER_LENGTH;
		memset(&frameInfo, 0, sizeof(VideoFrameHeader));
		setCurrentState(CONNECTED);
		start();
	} catch(cvgException e) {
		close();
		setCurrentState(FAULT);
		throw e;
	}
}

void VideoChannel::close() {
	// Signal the thread to stop
	selfStop();

	// Close the socket, so a pending recv() is stopped immediately
	// No new recv's will be issued since the thread is signaled to stop
	setCurrentState(DISCONNECTING);
	if (videoSocket != -1) {
		shutdown(videoSocket, SHUT_RDWR);
		::close(videoSocket);
		videoSocket = -1;
	}

	// Wait for the thread to actually stop
	cvg_bool excThrown = false;
	cvgException closeExc;
	try {
		stop();
	} catch(cvgException e) {
		// Save the exception if it does not stop on time and continue closing other things
		excThrown = true;
		closeExc = e;
	}

	setCurrentState(DISCONNECTED);

	Channel::close();

	if (frameBuffer != NULL) {
		delete [] frameBuffer;
		frameBuffer = NULL;
		frameBufferLength = 0;
	}

	if (auxFrameBuffer != NULL) {
		delete [] auxFrameBuffer;
		auxFrameBuffer = NULL;
	}

	gotFirstPacket = false;
	if (excThrown) throw closeExc;
}

cvg_bool VideoChannel::syncFrame(void *buffer, cvg_uint length) {
	cvg_char *fInfo = (cvg_char *)buffer;
	unsigned short expectedSignature = htons(PROTOCOL_VIDEOFRAME_SIGNATURE);
	if (!readData(fInfo, length)) return false;

	cvg_int i = 0;
	cvg_bool found = false;
	while(i < length - 1) {
		if (*(unsigned short *)&fInfo[i] == expectedSignature) {
			found = true;
			break;
		}
		i++;
	}
	if (found && i > 0) {
		for (cvg_int j = 0; j < length - i; j++) fInfo[j] = fInfo[i + j];
		found = readData(&fInfo[length - i], i);
	}
	return found;
}

void VideoChannel::run() {
	if (getCurrentState() != CONNECTED) { usleep(100000); return; }

	try {

		VideoFrameHeader fInfo;
		// Read frame header
		if (syncFrame(&fInfo, sizeof(VideoFrameHeader))) {
			syncTimer.restart(false);
			VideoFrameHeader_ntoh(&fInfo);
			if (fInfo.signature == PROTOCOL_VIDEOFRAME_SIGNATURE) {
				// If frame doesn't fit, reallocate space
				if (fInfo.videoData.dataLength > frameBufferLength &&
					fInfo.videoData.dataLength <= CVG_LITERAL_INT(VIDEO_CHANNEL_MAX_ALLOWED_FRAME_LENGTH)) {
					if (frameBufferMutex->lock()) {
						frameBufferLength = (cvg_int)(fInfo.videoData.dataLength * (1.0 + BUFFER_EXTRA_ALLOC_FACTOR));
						if (frameBuffer != NULL) delete [] frameBuffer;
						frameBuffer = new cvg_char[frameBufferLength];
						if (auxFrameBuffer != NULL) delete [] auxFrameBuffer;
						auxFrameBuffer = new cvg_char[frameBufferLength];
						memcpy(&frameInfo, &fInfo, sizeof(VideoFrameHeader));
						frameBufferMutex->unlock();
					}
				}
				if (fInfo.videoData.dataLength > CVG_LITERAL_INT(VIDEO_CHANNEL_MAX_ALLOWED_FRAME_LENGTH)) {
					log(cvgString("Frame is too long: ") + fInfo.videoData.dataLength + " bytes");
					//setCurrentState(FAULT);
				} else {
					// Receive frame data in auxiliary buffer
					if (readData(auxFrameBuffer, fInfo.videoData.dataLength)) {
						if (frameBufferMutex->lock()) {
							try {
								// Copy frame data to "official" buffer
								memcpy(&frameInfo, &fInfo, sizeof(VideoFrameHeader));
								memcpy(frameBuffer, auxFrameBuffer, fInfo.videoData.dataLength);
								gotFirstPacket = true;
								lastFrameTime.restart(false);
								notifyReceivedData(this);
							} catch(cvgException e) {
								// Consumers could throw an exception. Need to unlock the mutex if it happens.
								frameBufferMutex->unlock();
								throw e;
							}
							frameBufferMutex->unlock();
						}
					} else {
						log("Cannot read frame data");
						setCurrentState(FAULT);
					}
				}
			}
		} else {
			if (syncTimer.getElapsedSeconds() > VIDEO_CHANNEL_MAX_INTERSYNC_SECONDS) {
				log("Unable to find SYNC");
				setCurrentState(FAULT);
			}
			usleep(10);
		}
		/*if (gotFirstPacket) {
			if (lastFrameTime.getElapsedSeconds() > VIDEO_CHANNEL_MAX_INTERFRAME_SECONDS) {*/
//				setCurrentState(FAULT);
/*			}
		} else usleep(10000);*/
	} catch(cvgException e) {
		log(e.getMessage());
	}
}

cvg_bool VideoChannel::readData(void *buffer, cvg_uint length) {
	cvg_uint remaining = length;
	cvg_uint offset = 0;

//	Timer recvStartTime;
	while(remaining > 0) {
		ssize_t received = recv(videoSocket, &((cvg_char *)buffer)[offset], remaining, MSG_NOSIGNAL);
		if (received <= 0) return false;
		offset += received;
		remaining -= received;
//		if (recvStartTime.getElapsedSeconds() >= timeout) return false;
	}
	return true;
}

}
}
