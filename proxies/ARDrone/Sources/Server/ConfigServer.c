/*
 * ConfigServer.c
 *
 *  Created on: 14/01/2012
 *      Author: Ignacio Mellado
 */

#include "ConfigServer.h"
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
#include "ConfigManager.h"

#define CONFIGSERVER_MAX_CLIENTS					16
#define CONFIGSERVER_ACCEPT_TIMEOUT					0.2
#define CONFIGSERVER_MAX_ATOMIC_SEND_SECONDS		0.5
#define CONFIGSERVER_MAX_ATOMIC_RECV_SECONDS		0.5
#define CONFIGSERVER_CLIENT_DATA_BUFFER_SIZE		1024

TClientList *configServer_clientList = NULL;
extern vp_com_t globalCom;	/* Global comm interface */
extern bool_t ardrone_tool_exit();
vp_com_socket_t	configServerSocket;
volatile bool_t configServerStarted = FALSE;

C_RESULT configServer_init() {
	configServer_clientList = clientList_create(CONFIGSERVER_MAX_CLIENTS);

	/* Create server socket */
	configServerSocket.type = VP_COM_SERVER;
	configServerSocket.protocol = VP_COM_TCP;
	configServerSocket.block = VP_COM_DONTWAIT;
	configServerSocket.is_multicast = 0;
	configServerSocket.port = CONFIG_SERVER_PORT;
/*	if (FAILED(vp_com_open(&globalCom, &configServerSocket, NULL, NULL))) {
		PRINT("[ConfigServer] Unable to open server socket\n");
		return C_FAIL;
	}*/
	/* We create the server socket manually, as we need to set the reuse flag before binding it and Parrot's SDK doesn't allow that */
	bool_t error = TRUE;
	configServerSocket.priv = (void *)socket(AF_INET, SOCK_STREAM, 0);
	if ((int)configServerSocket.priv >= 0) {
		struct sockaddr_in serverAddress;
		/* Try reusing the address */
		int s = 1;
		setsockopt((int)configServerSocket.priv, SOL_SOCKET, SO_REUSEADDR, &s, sizeof(int));
		/* Bind to address and port */
		bzero((char *)&serverAddress, sizeof(serverAddress));
		serverAddress.sin_family = AF_INET;
		serverAddress.sin_addr.s_addr = INADDR_ANY;
		serverAddress.sin_port = htons(configServerSocket.port);
	    if (bind((int)configServerSocket.priv, (struct sockaddr *) &serverAddress, sizeof(struct sockaddr_in)) >= 0)
		{
	    	error = FALSE;
		} else close((int)configServerSocket.priv);
	}
	if (error) {
		clientList_destroy(configServer_clientList);
		configServer_clientList = NULL;
		PRINT("[ConfigServer] Unable to open server socket\n");
		return C_FAIL;
	}

	/* Set server socket timeout */
	struct timeval tm;
	tm.tv_sec = 0;
	tm.tv_usec = CONFIGSERVER_ACCEPT_TIMEOUT * 1e6;
	setsockopt((int32_t)configServerSocket.priv, SOL_SOCKET, SO_RCVTIMEO, &tm, sizeof(tm));

	configServerStarted = TRUE;

	return C_OK;
}

void configServer_closeClient(TClient *client) {
	if (client->socket.priv != NULL) {
		shutdown((int)client->socket.priv, SHUT_RDWR);
		vp_com_close(&globalCom, &client->socket);
	}
}

void configServer_destroy() {
	uint32_t iterator;
	TClient *curClient;

	if (!configServerStarted) return;
	configServerStarted = FALSE;

	/* Do not accept more clients */
	vp_com_close(&globalCom, &configServerSocket);

	if (configServer_clientList != NULL) {
		/* Close all client sockets */
		clientList_startIterator(&iterator);
		while((curClient = clientList_getNextClient(configServer_clientList, &iterator)) != NULL) {
			configServer_closeClient(curClient);
		}
		clientList_destroy(configServer_clientList);
		configServer_clientList = NULL;
	}
}

#define STATE_WAITING_REQUEST		0
#define STATE_RECEIVING_REQUEST		1
#define STATE_RECEIVING_PARAM_DATA	2
#define STATE_SENDING_RESPONSE		3
#define STATE_SENDING_PARAM_DATA	4
#define STATE_ERROR					5

DEFINE_THREAD_ROUTINE(configServer_receptionist, data) {
	while(!ardrone_tool_exit()) {
		vp_com_socket_t clientSocket;
		if (SUCCEED(vp_com_wait_connections(&globalCom, &configServerSocket, &clientSocket, 5))) {
			clientList_lock(configServer_clientList);
			TClient *newClient = clientList_addClient(configServer_clientList);
			if (newClient != NULL) {
				struct timeval tm;
				vp_os_memcpy(&newClient->socket, &clientSocket, sizeof(vp_com_socket_t));
				newClient->state = STATE_WAITING_REQUEST;
				/* Set client socket timeout */
				tm.tv_sec = 0;
				tm.tv_usec = CONFIGSERVER_MAX_ATOMIC_SEND_SECONDS * 1e6;
				setsockopt((int32_t)newClient->socket.priv, SOL_SOCKET, SO_SNDTIMEO, &tm, sizeof(tm));
				tm.tv_sec = 0;
				tm.tv_usec = CONFIGSERVER_MAX_ATOMIC_RECV_SECONDS * 1e6;
				setsockopt((int32_t)newClient->socket.priv, SOL_SOCKET, SO_RCVTIMEO, &tm, sizeof(tm));
				newClient->dataBuffer = vp_os_malloc(CONFIGSERVER_CLIENT_DATA_BUFFER_SIZE);
				PRINT("[ConfigServer] New client connection\n");
			}
			clientList_unlock(configServer_clientList);
		}
	}
	THREAD_RETURN(0);
}

#define IO_OK				0
#define IO_CLOSED			1
#define IO_ERROR			2

int32_t readData(TClient *client, void *buffer, uint32_t *index, uint32_t *remainingLength) {
	int32_t received;
	int32_t result = IO_OK;
	/* With the SDK, we cannot recv without signals, so we do it manually */
	received = recv((int)client->socket.priv, &(((int8_t *)buffer)[*index]), (*remainingLength), MSG_NOSIGNAL | MSG_DONTWAIT);
	if (received < 0) {
		if (errno != EAGAIN && errno != EWOULDBLOCK) result = IO_ERROR;
	} else {
		if (received == 0) result = IO_CLOSED;
		else {
			(*index) += received;
			(*remainingLength) -= received;
		}
	}
	return result;
}

int32_t writeData(TClient *client, void *buffer, uint32_t *index, uint32_t *remainingLength) {
	int32_t written;
	int32_t result = IO_OK;
	/* With the SDK, we cannot recv without signals, so we do it manually */
	written = send((int)client->socket.priv, &(((int8_t *)buffer)[*index]), (*remainingLength), MSG_NOSIGNAL | MSG_DONTWAIT);
	if (written < 0) {
		if (errno != EAGAIN && errno != EWOULDBLOCK) result = IO_ERROR;
	} else {
		(*index) += written;
		(*remainingLength) -= written;
	}
	return result;
}

uint32_t processWriteRequest(TClient *client) {
	/* We have the full request in the client structure */
	C_RESULT result = configManager_setParamFromNetwork(client->request.info.paramId, client->dataBuffer, client->request.info.dataLength);
	/* Setup index, remainingLength and request for the ACK packet */
	client->request.info.dataLength = 0;
	client->request.info.mode = result == C_OK ? CONFIG_ACK : CONFIG_NACK;
	ConfigHeader_hton(&client->request);
	client->bufferIndex = 0;
	client->remainingData = sizeof(ConfigHeader);
	return STATE_SENDING_RESPONSE;
}

uint32_t processReadRequest(TClient *client) {
	/* We have the full request in the client structure */
	/* Setup index, remainingLength, request and dataBuffer for the WRITE packet with the param value */
	uint32_t actualLength;
	C_RESULT result;
	result = configManager_getParamForNetwork(client->request.info.paramId, client->dataBuffer, CONFIGSERVER_CLIENT_DATA_BUFFER_SIZE, &actualLength);
	client->request.info.mode = result == C_OK ? CONFIG_WRITE : CONFIG_NACK;
	client->request.info.dataLength = actualLength;
	ConfigHeader_hton(&client->request);
	client->bufferIndex = 0;
	client->remainingData = sizeof(ConfigHeader);
	return STATE_SENDING_RESPONSE;
}

uint32_t processRequest(TClient *client) {
	ConfigHeader_ntoh(&client->request);
	if (client->request.signature != PROTOCOL_CONFIG_SIGNATURE) return STATE_ERROR;
	switch(client->request.info.mode) {
	case CONFIG_WRITE:
		client->remainingData = client->request.info.dataLength;
		/* If data is too long, disconnect the client */
		if (client->remainingData > CONFIGSERVER_CLIENT_DATA_BUFFER_SIZE) return STATE_ERROR;
		client->bufferIndex = 0;
		return STATE_RECEIVING_PARAM_DATA;
	case CONFIG_READ:
		return processReadRequest(client);
	default:
		return STATE_ERROR;
	}
}

int32_t manageClient(TClient *client) {
	int32_t result;
	switch(client->state) {
	case STATE_WAITING_REQUEST:
		client->bufferIndex = 0;
		client->remainingData = sizeof(ConfigHeader);
		result = readData(client, &client->request, &client->bufferIndex, &client->remainingData);
		if (result != IO_OK) return result;
		if (client->remainingData == 0) client->state = processRequest(client);
		else client->state = STATE_RECEIVING_REQUEST;
		if (client->state == STATE_ERROR) return C_FAIL;
		break;
	case STATE_RECEIVING_REQUEST:
		result = readData(client, &client->request, &client->bufferIndex, &client->remainingData);
		if (result != IO_OK) return result;
		if (client->remainingData == 0) client->state = processRequest(client);
		if (client->state == STATE_ERROR) return C_FAIL;
		break;
	case STATE_RECEIVING_PARAM_DATA:
		result = readData(client, client->dataBuffer, &client->bufferIndex, &client->remainingData);
		if (result != IO_OK) return result;
		if (client->remainingData == 0) client->state = processWriteRequest(client);
		if (client->state == STATE_ERROR) return C_FAIL;
		break;
	case STATE_SENDING_RESPONSE:
		result = writeData(client, &client->request, &client->bufferIndex, &client->remainingData);
		if (result != IO_OK) return result;
		if (client->remainingData == 0) {
			ConfigHeader_ntoh(&client->request);
			if (client->request.info.mode == CONFIG_ACK || client->request.info.mode == CONFIG_NACK)
				client->state = STATE_WAITING_REQUEST;
			else {
				client->remainingData = client->request.info.dataLength;
				client->bufferIndex = 0;
				client->state = STATE_SENDING_PARAM_DATA;
			}
		}
		break;
	case STATE_SENDING_PARAM_DATA:
		result = writeData(client, client->dataBuffer, &client->bufferIndex, &client->remainingData);
		if (result != IO_OK) return result;
		if (client->remainingData == 0) client->state = STATE_WAITING_REQUEST;
		break;
	}
	return IO_OK;
}

DEFINE_THREAD_ROUTINE(configServer_roomService, data) {
	TClient *client;
	uint32_t iterator;
	while(!ardrone_tool_exit()) {
		clientList_startIterator(&iterator);
		while((client = clientList_getNextClient(configServer_clientList, &iterator)) != NULL) {
			bool_t removeClient = FALSE;
			switch(manageClient(client)) {
				case IO_ERROR:
					PRINT("[ConfigServer] There was an error with client %d and it will be closed\n", (int)client->socket.priv);
					removeClient = TRUE;
					break;
				case IO_CLOSED:
					PRINT("[ConfigServer] The client %d disconnected\n", (int)client->socket.priv);
					removeClient = TRUE;
					break;
			}
			if (removeClient) {
				configServer_closeClient(client);
				clientList_removeClient(configServer_clientList, client);
			}
		}
		usleep(50000);
	}
	THREAD_RETURN(0);
}
