/*
 * ClientList.c
 *
 *  Created on: 09/01/2012
 *      Author: Ignacio Mellado
 *      Notes: Currently, the list is implemented as an array and the iteration functions walk it until a full
 *      position is found. After some client disconnection, the array gets fragmented and iteration and look-up
 *      become inefficient. This should be fixed in future versions with a hash linked list.
 */

#include "ClientList.h"

TClientList *clientList_create(int32_t maxClients) {
	TClientList *cl = vp_os_malloc(sizeof(TClientList));
	cl->maxClients = maxClients;
	cl->numClients = 0;
	cl->clients = vp_os_malloc(maxClients * sizeof(TClient));
	clientList_removeAllClients(cl);
	vp_os_mutex_init(&cl->mutex);
	return cl;
}

void clientList_destroy(TClientList *cl) {
	vp_os_free(cl->clients);
	vp_os_mutex_destroy(&cl->mutex);
	vp_os_free(cl);
}

void clientList_lock(TClientList *cl) {
	vp_os_mutex_lock(&cl->mutex);
}

void clientList_unlock(TClientList *cl) {
	vp_os_mutex_unlock(&cl->mutex);
}

TClient *clientList_addClient(TClientList *cl) {
	int i;
	for (i = 0; i < cl->maxClients; i++) {
		if (!cl->clients[i].usedSlot) {
			cl->clients[i].usedSlot = TRUE;
			cl->numClients++;
			return &cl->clients[i];
		}
	}
	return NULL;
}

void clientList_removeClient(TClientList *cl, TClient *client) {
	if (client == NULL) return;
	if (client->usedSlot) {
		client->usedSlot = FALSE;
		cl->numClients--;
	}
}

void clientList_removeAllClients(TClientList *cl) {
	int i = 0;
	for (i = 0; i < cl->maxClients; i++)
		clientList_removeClient(cl, &cl->clients[i]);
}

void clientList_startIterator(uint32_t *iteratorIndex) {
	(*iteratorIndex) = 0;
}

TClient *clientList_getNextClient(TClientList *cl, uint32_t *iteratorIndex) {
	while((*iteratorIndex) < cl->maxClients) {
		uint32_t it = (*iteratorIndex)++;
		if (cl->clients[it].usedSlot) return &cl->clients[it];
	}
	return NULL;
}

TClient *clientList_getClientByInfo(TClientList *cl, struct in_addr *remoteAddress, uint16_t commandPort) {
	uint32_t iterator;
	TClient *curClient;
	clientList_startIterator(&iterator);
	while((curClient = clientList_getNextClient(cl, &iterator)) != NULL) {
		if (curClient->remoteAddress.s_addr == remoteAddress->s_addr &&
			curClient->commandPort == commandPort)
			return curClient;
	}
	return NULL;
}

uint32_t clientList_getNumClients(TClientList *cl) {
	return cl->numClients;
}

