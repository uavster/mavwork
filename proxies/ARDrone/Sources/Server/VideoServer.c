/*
 * VideoServer.c
 *
 *  Created on: 31/08/2011
 *      Author: Ignacio Mellado
 */

#define VIDEOSERVER_MAXCLIENTS		16
#define ACCEPT_TIMEOUT				0.2
#define MAX_FRAME_WAIT_SECONDS		0.2
#define MAX_ATOMIC_SEND_SECONDS		0.5
#define MAX_SEND_FRAME_SECONDS		0.5

#include <VideoServer.h>

#define VIDEOSERVER_DEFAULT_FRAME_WIDTH		QVGA_WIDTH
#define VIDEOSERVER_DEFAULT_FRAME_HEIGHT	QVGA_HEIGHT
#define VIDEOSERVER_DEFAULT_FRAME_BPP		24
#define VIDEOSERVER_DEFAULT_OUTPUT_ENCODING	JPEG //RAW_BGR24

#include <VP_Api/vp_api_thread_helper.h>
#include <ardrone_api.h>
#include <VP_Com/vp_com.h>
#include <VP_Com/vp_com_socket.h>
#include <VP_Com/vp_com_error.h>
#include <VP_Os/vp_os_print.h>
#include <sys/socket.h>
#include <Protocol.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <errno.h>
#include "ClientList.h"
#include <unistd.h>
#include "VideoTranscoder.h"

TClientList *videoServer_clientList;

/* This function is not defined in any header file */
/*extern C_RESULT vp_com_client_open_socket(vp_com_socket_t* server_socket, vp_com_socket_t* client_socket);*/

extern vp_com_t globalCom;
extern bool_t ardrone_tool_exit();

void closeClient(TClient *client) {
	if (client->socket.priv != NULL) {
		shutdown((int)client->socket.priv, SHUT_RDWR);
		vp_com_close(&globalCom, &client->socket);
	}
}

vp_com_socket_t	videoServerSocket;

int8_t *frameBuffer = NULL;
vp_os_mutex_t frameBufferMutex, settingsMutex;
vp_os_cond_t frameBufferCond;

int32_t videoServer_frameWidth = VIDEOSERVER_DEFAULT_FRAME_WIDTH;
int32_t videoServer_frameHeight = VIDEOSERVER_DEFAULT_FRAME_HEIGHT;
int32_t videoServer_frameBpp = VIDEOSERVER_DEFAULT_FRAME_BPP;
int32_t videoServer_framePacketLength;
VideoFormat videoServer_frameSourceEncoding = RAW_BGR24;
VideoFormat videoServer_frameOutputEncoding = VIDEOSERVER_DEFAULT_OUTPUT_ENCODING;

int8_t *availVideoBuffer = NULL;

volatile bool_t videoServerStarted = FALSE;

void videoServer_setOutputEncoding(VideoFormat outputEncoding) {
	vp_os_mutex_lock(&settingsMutex);
	videoServer_frameOutputEncoding = outputEncoding;
	vp_os_mutex_unlock(&settingsMutex);
}

void videoServer_destroy() {
	uint32_t iterator;
	TClient *curClient;

	/* Do not accept more clients */
	vp_com_close(&globalCom, &videoServerSocket);

	/* Close all client sockets */
	if (videoServer_clientList != NULL) {
		clientList_startIterator(&iterator);
		while((curClient = clientList_getNextClient(videoServer_clientList, &iterator)) != NULL) {
			closeClient(curClient);
		}
		clientList_destroy(videoServer_clientList);
		videoServer_clientList = NULL;
	}

	/* Free resources */
	vp_os_cond_destroy(&frameBufferCond);
	vp_os_mutex_destroy(&frameBufferMutex);
	vp_os_free(frameBuffer);

	videoTranscoder_destroy();
	vp_os_mutex_destroy(&settingsMutex);

	videoServerStarted = FALSE;
}

void videoServer_endBroadcaster() {
	/* Signal broadcaster thread */
	vp_os_mutex_lock(&frameBufferMutex);
	availVideoBuffer = NULL;
	vp_os_cond_signal(&frameBufferCond);
	vp_os_mutex_unlock(&frameBufferMutex);
}

C_RESULT videoServer_init() {
	videoServer_clientList = clientList_create(VIDEOSERVER_MAXCLIENTS);

	/* Build frame packet buffer */
	videoServer_framePacketLength = sizeof(VideoFrameHeader) + videoServer_frameWidth * videoServer_frameHeight * (videoServer_frameBpp / 8);
	frameBuffer = vp_os_malloc(videoServer_framePacketLength);

	vp_os_mutex_init(&frameBufferMutex);
	vp_os_cond_init(&frameBufferCond, &frameBufferMutex);
	/* Create server socket */
	videoServerSocket.type = VP_COM_SERVER;
	videoServerSocket.protocol = VP_COM_TCP;
	videoServerSocket.block = VP_COM_DONTWAIT;
	videoServerSocket.is_multicast = 0;
	videoServerSocket.port = VIDEO_SERVER_PORT;
/*	if (FAILED(vp_com_open(&globalCom, &videoServerSocket, NULL, NULL))) {
		vp_os_cond_destroy(&frameBufferCond);
		vp_os_mutex_destroy(&frameBufferMutex);
		vp_os_free(frameBuffer);
		vp_os_mutex_destroy(&clientListMutex);
		PRINT("[VideoServer] Unable to open server socket\n");
		return C_FAIL;
	}*/
	/* We create the server socket manually, as we need to set the reuse flag before binding it and Parrot's SDK doesn't allow that */
	bool_t error = TRUE;
	videoServerSocket.priv = (void *)socket(AF_INET, SOCK_STREAM, 0);
	if ((int)videoServerSocket.priv >= 0) {
		struct sockaddr_in serverAddress;
		/* Try reusing the address */
		int s = 1;
		setsockopt((int)videoServerSocket.priv, SOL_SOCKET, SO_REUSEADDR, &s, sizeof(int));
		/* Bind to address and port */
		bzero((char *)&serverAddress, sizeof(serverAddress));
		serverAddress.sin_family = AF_INET;
		serverAddress.sin_addr.s_addr = INADDR_ANY;
		serverAddress.sin_port = htons(videoServerSocket.port);
	    if (bind((int)videoServerSocket.priv, (struct sockaddr *) &serverAddress, sizeof(struct sockaddr_in)) >= 0)
		{
	    	error = FALSE;
		} else close((int)videoServerSocket.priv);
	}
	if (error) {
		vp_os_cond_destroy(&frameBufferCond);
		vp_os_mutex_destroy(&frameBufferMutex);
		vp_os_free(frameBuffer);
		if (videoServer_clientList != NULL) {
			clientList_destroy(videoServer_clientList);
			videoServer_clientList = NULL;
		}
		PRINT("[VideoServer] Unable to open server socket\n");
		return C_FAIL;
	}

	/* Set server socket timeout */
	struct timeval tm;
	tm.tv_sec = 0;
	tm.tv_usec = ACCEPT_TIMEOUT * 1e6;
	setsockopt((int32_t)videoServerSocket.priv, SOL_SOCKET, SO_RCVTIMEO, &tm, sizeof(tm));

	videoTranscoder_init();

	videoServerStarted = TRUE;
	vp_os_mutex_init(&settingsMutex);

	return C_OK;
}

bool_t videoServer_isStarted() {
	return videoServerStarted;
}

void videoServer_lockFrameBuffer() {
	vp_os_mutex_lock(&frameBufferMutex);
}

void videoServer_unlockFrameBuffer() {
	vp_os_mutex_unlock(&frameBufferMutex);
}

uint64_t videoServer_lastFrameTimeCode;

void videoServer_feedFrame_BGR24(uint64_t timeCode, int8_t *buffer) {
	availVideoBuffer = buffer;
	videoServer_lastFrameTimeCode = timeCode;
	vp_os_cond_signal(&frameBufferCond);
}

int8_t *videoServer_waitForNewFrame() {
	/* Wait for a new available frame */
	vp_os_mutex_lock(&frameBufferMutex);
	//C_RESULT res = vp_os_cond_timed_wait(&frameBufferCond, MAX_FRAME_WAIT_SECONDS * 1000);
	//if (SUCCEED(res) && availVideoBuffer != NULL)
	vp_os_cond_wait(&frameBufferCond);
	if (availVideoBuffer != NULL) {
		((VideoFrameHeader *)frameBuffer)->videoData.timeCodeH = (unsigned int)(videoServer_lastFrameTimeCode >> 32);
		((VideoFrameHeader *)frameBuffer)->videoData.timeCodeL = (unsigned int)(videoServer_lastFrameTimeCode & 0xFFFFFFFF);
		((VideoFrameHeader *)frameBuffer)->videoData.encoding = videoServer_frameSourceEncoding;
		((VideoFrameHeader *)frameBuffer)->videoData.width = videoServer_frameWidth;
		((VideoFrameHeader *)frameBuffer)->videoData.height = videoServer_frameHeight;
		((VideoFrameHeader *)frameBuffer)->videoData.dataLength = videoServer_framePacketLength - sizeof(VideoFrameHeader);
		vp_os_memcpy(&frameBuffer[sizeof(VideoFrameHeader)], availVideoBuffer, videoServer_framePacketLength - sizeof(VideoFrameHeader));
		vp_os_mutex_unlock(&frameBufferMutex);
		availVideoBuffer = NULL;
		return frameBuffer;
	}
	vp_os_mutex_unlock(&frameBufferMutex);
	return NULL;
}

#include <unistd.h>

void videoServer_transmitFrame(int8_t *frame, int32_t length) {
	uint32_t listIterator;
	TClient *currentClient;
	clientList_lock(videoServer_clientList);

//printf("%d,%d %d %d %d\n", ((VideoFrameHeader *)frame)->videoData.width,((VideoFrameHeader *)frame)->videoData.height, ((VideoFrameHeader *)frame)->videoData.encoding, ((VideoFrameHeader *)frame)->videoData.dataLength, length);
	VideoFrameHeader_hton((VideoFrameHeader *)frame);
//struct timeval start, end;
//gettimeofday(&start, NULL);
	clientList_startIterator(&listIterator);
	while((currentClient = clientList_getNextClient(videoServer_clientList, &listIterator)) != NULL) {
		/* Send frame data to client */
		struct timeval transmissionStart;
		gettimeofday(&transmissionStart, NULL);
		int32_t offset = 0;
		while(offset < length) {
			struct timeval currentTime;
			int32_t sent = length - offset;
			// The socket is sending a signal (probably SIGPIPE) and the SDK does not provide the option for MSG_NOSIGNAL
			// This is why we do it directly with the socket API
//				if (FAILED(vp_com_write_socket(&videoServerClients[i].socket, &frameBuffer[offset], &sent))) {
			sent = send((int)currentClient->socket.priv, &frame[offset], sent, MSG_NOSIGNAL); // | MSG_DONTWAIT);
			if (sent < 0) {
				if (errno == EAGAIN) sent = 0;
				else {
					PRINT("[VideoServer] The client socket %d failed and it will be closed\n", (uint32_t)currentClient->socket.priv);
					closeClient(currentClient);
					clientList_removeClient(videoServer_clientList, currentClient);
					break;
				}
			}
			/* PRINT("Sent: %d, offset: %d\n", sent, offset); */
			offset += sent;
			/* If total time is out, close client */
			gettimeofday(&currentTime, NULL);
			double elapsed = (currentTime.tv_sec - transmissionStart.tv_sec) + (currentTime.tv_usec - transmissionStart.tv_usec) / 1e6;
			if (elapsed > MAX_SEND_FRAME_SECONDS) {
				PRINT("[VideoServer] The client socket %d timed out and it will be closed\n", (uint32_t)currentClient->socket.priv);
				closeClient(currentClient);
				clientList_removeClient(videoServer_clientList, currentClient);
				break;
			}
		}
	}
//gettimeofday(&end, NULL);
//PRINT("Elapsed: %f\n", (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1e6);
	clientList_unlock(videoServer_clientList);
}

/*
 * This thread accepts incoming client connections
 */
DEFINE_THREAD_ROUTINE(videoServer, data) {
	while(!ardrone_tool_exit()) {
		vp_com_socket_t clientSocket;
		if (SUCCEED(vp_com_wait_connections(&globalCom, &videoServerSocket, &clientSocket, 5))) {
			clientList_lock(videoServer_clientList);
			TClient *newClient = clientList_addClient(videoServer_clientList);
			if (newClient != NULL) {
				struct timeval tm;
				vp_os_memcpy(&newClient->socket, &clientSocket, sizeof(vp_com_socket_t));
				/* Set client socket timeout */
				tm.tv_sec = 0;
				tm.tv_usec = MAX_ATOMIC_SEND_SECONDS * 1e6;
				setsockopt((int32_t)newClient->socket.priv, SOL_SOCKET, SO_SNDTIMEO, &tm, sizeof(tm));
				PRINT("[VideoServer] New client connection\n");
			}
			clientList_unlock(videoServer_clientList);
		}
	}
	THREAD_RETURN(0);
}

DEFINE_THREAD_ROUTINE(videoServer_broadcaster, data) {
	while(!ardrone_tool_exit()) {
		int8_t *newFrame = videoServer_waitForNewFrame();
		if (newFrame != NULL) {
			int8_t *encodedFrame = NULL;
			VideoFormat outputEncoding;

			vp_os_mutex_lock(&settingsMutex);
			outputEncoding = videoServer_frameOutputEncoding;
			vp_os_mutex_unlock(&settingsMutex);

			if (videoTranscoder_transcode(newFrame, outputEncoding, (uint8_t **)&encodedFrame)) {
				videoServer_transmitFrame(encodedFrame, ((VideoFrameHeader *)encodedFrame)->videoData.dataLength + sizeof(VideoFrameHeader));
			}
		}
	}
	THREAD_RETURN(0);
}
