/*
 * ClientList.h
 *
 *  Created on: 09/02/2012
 *      Author: nacho
 */

#ifndef CLIENTLIST_H_
#define CLIENTLIST_H_

#include <ardrone_api.h>
#include <VP_Api/vp_api_thread_helper.h>
#include <VP_Com/vp_com.h>
#include <VP_Com/vp_com_socket.h>
#include <arpa/inet.h>
#include "Protocol.h"

typedef struct {
	bool_t				usedSlot;
	/* Used by videoServer (sockets identified by established connection) */
	vp_com_socket_t 	socket;
	Read				socketReader;
	Write				socketWriter;
	/* Used by commandServer (sockets identified by source IP and port) */
	struct in_addr		remoteAddress;
	uint16_t			commandPort;
	/* This flag indicates whether the client owns the command channel (COMMANDER role) */
	uint8_t				isOwner;
	/* Used to track the feedback port of every client. If the received port in a command packet is different,
	 * the feedback server is notified */
	uint16_t			feedbackPort;
	/* Last received packet index. Packets with a lower index will be discarded. */
	uint64_t			lastPacketIndex;
	/* Last time a packet was received from this client */
	struct timeval		lastCommTime;
	/* Number of bad packets sent by the client */
	uint32_t			numBadPackets;
	/* Used by configServer info */
	uint32_t			state;
	ConfigHeader		request;
	void				*dataBuffer;
	uint32_t			bufferIndex;
	uint32_t			remainingData;
} TClient;

typedef struct {
	uint32_t maxClients;
	uint32_t numClients;
	TClient *clients;
	vp_os_mutex_t mutex;
} TClientList;

TClientList *clientList_create(int32_t maxClients);
void clientList_destroy(TClientList *cl);
void clientList_lock(TClientList *cl);
void clientList_unlock(TClientList *cl);
TClient *clientList_addClient(TClientList *cl);
void clientList_removeClient(TClientList *cl, TClient *client);
void clientList_removeAllClients(TClientList *cl);
void clientList_startIterator(uint32_t *iteratorIndex);
TClient *clientList_getNextClient(TClientList *cl, uint32_t *iteratorIndex);
TClient *clientList_getClientByInfo(TClientList *cl, struct in_addr *remoteAddress, uint16_t commandPort);
uint32_t clientList_getNumClients(TClientList *cl);

#endif /* CLIENTLIST_H_ */
