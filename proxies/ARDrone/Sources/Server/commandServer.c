/*
 * server.c
 *
 *  Created on: 12/08/2011
 *      Author: Ignacio Mellado
 *      Notes:	This is inside ARDrone's framework, so we use plain C for portability reasons
 */

#define FEEDBACK_SERVER_MAX_CLIENTS			16
#define READ_BUFFER_SIZE					1024
#define SOCKET_REOPEN_GRACE_TIME_S			0.5

#define RECV_TIMEOUT										0.5
#define COMMAND_SERVER_DEAD_CLIENTS_CLEANUP_PERIOD			0.5

// Drone configuration at take-off
#define VIDEO_ENABLE				1
#define DRONE_VISION_ENABLE			1

#include "commandServer.h"
#include <VP_Com/vp_com.h>
#include <VP_Com/vp_com_error.h>
#include <VP_Com/vp_com_socket.h>
#include <VP_Os/vp_os_print.h>
#include <Soft/Lib/ardrone_tool/UI/ardrone_input.h>
#include "Protocol.h"
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <Soft/Lib/ardrone_tool/ardrone_tool_configuration.h>
#include "ClientList.h"

volatile uint8_t dependentThreadsFinished = 0;
volatile uint8_t commandServer_sharedInit = 0;	// This flag indicates if the shared resources were already initialized by this thread
TClientList *feedback_clientList;
TClient *channelOwner = NULL;
int8_t currentDroneMode = -1;

uint8_t commandServer_sharedResourcesInitStatus() {
	return commandServer_sharedInit;
}

void commandServer_notifyDependentThreadFinished() {
	dependentThreadsFinished++;
}

TClientList *commandServer_getFeedbackClientList() {
	return feedback_clientList;
}

extern bool_t ardrone_tool_exit();
extern C_RESULT signal_exit();

extern vp_com_t globalCom;
vp_com_socket_t commandServerSocket;

void processPacket(CommandPacket *p);

Read commandReader;
Write commandWriter;

TClient *commandServer_addNewClient(struct in_addr *remoteAddress, uint16_t commandPort, uint16_t feedbackPort, uint64_t initialPacketIndex) {
	TClient *client;
	client = clientList_addClient(feedback_clientList);
	if (client == NULL) return NULL;
	client->remoteAddress.s_addr = remoteAddress->s_addr;
	client->commandPort = commandPort;
	client->lastPacketIndex = initialPacketIndex;
	/* Init client socket */
	vp_os_memset((int8_t *)&client->socket, 0, sizeof(client->socket));
	client->socket.type = VP_COM_CLIENT;
	client->socket.protocol = VP_COM_UDP;
	client->socket.block = VP_COM_DEFAULT;
	client->socket.port = feedbackPort;
	strcpy(client->socket.serverHost, inet_ntoa(*remoteAddress));
	/* WARNING: The ARDrone SDK has a bug that keeps us from creating a client UDP socket without binding
	 * it to the client port. If we use the proxy on the same computer as the client, both proccesses try to
	 * bind the port and one of them fails. This is why we create the socket manually.
	 */
	/* We use the generic communications interface to open the socket */
	/*if (vp_com_open(&globalCom, &client->socket, &client->socketReader, &client->socketWriter) != VP_COM_OK) {
	    commandServer_removeClient(feedback_clientList, client);
		return NULL;
	}
	*/
	client->socket.priv = (void *)socket(AF_INET, SOCK_DGRAM, 0);
	client->socket.scn = remoteAddress->s_addr;
	client->socketReader = (Read)vp_com_read_udp_socket;
	client->socketWriter = (Write)vp_com_write_udp_socket;
	/*
	 * The above lines are to fix an SDK bug. When Parrot fixes it, the lines should be replaced by the
	 * call to vp_com_open()
	 */
	return client;
}

uint64_t getPacketIndex(CommandPacket *p) {
	return ((uint64_t)p->indexL | ((uint64_t)p->indexH << 32));
}

void setDroneSafeState() {
	if (currentDroneMode != -1) {
		/* The application did not finish, the loop was exited because of an error */
		/* If the ARDrone was moving, keep it hovering */
		if (currentDroneMode == MOVE)
			ardrone_at_set_progress_cmd(0, 0, 0, 0, 0);
		// If the ARDrone was taking off, make it land
		else if (currentDroneMode == TAKEOFF)
			ardrone_tool_set_ui_pad_start(0);
	}
}

void commandServer_removeClient(TClient *client) {
	vp_com_close_socket(&client->socket);
	clientList_removeClient(feedback_clientList, client);
	/* If the client is the channel owner, free it */
	if (client == channelOwner) {
		/* There is no channel owner: go to a safe state */
		setDroneSafeState();
		channelOwner = NULL;
		PRINT("[commandServer] The drone has been set to a safe state because the commander client was removed\n");
	}
}

uint8_t checkClientAccess(struct in_addr *remoteAddress, uint16_t commandPort, uint8_t receptionError, int32_t newFeedbackPort, uint64_t packetIndex, bool_t *outOfSeq, bool_t *isNewClient) {
	TClient *client;
	uint8_t result = 0;
	(*outOfSeq) = FALSE;
	/* Is the client registered? */
	clientList_lock(feedback_clientList);
	client = clientList_getClientByInfo(feedback_clientList, remoteAddress, commandPort);
	if (client != NULL) {
		(*isNewClient) = FALSE;
		if (!receptionError) {
			/* Don't do this if the packet is out of sequence */
			if (packetIndex > client->lastPacketIndex) {
				client->feedbackPort = newFeedbackPort;
				client->socket.port = newFeedbackPort;
				client->lastPacketIndex = packetIndex;
				if (client->numBadPackets > 0) client->numBadPackets--;
			} else (*outOfSeq) = TRUE;
		}
		else {
			client->numBadPackets++;
			if (client->numBadPackets > COMMAND_SERVER_NUM_BAD_PACKETS_FOR_DISCONNECTION) {
				PRINT("[commandServer] Client %s:%d sent too many bad packets and it will be forgotten\n", inet_ntoa(client->remoteAddress), client->commandPort);
				/* Too many bad packets: forget this client and force it to reconnect */
				commandServer_removeClient(client);
				client = NULL;
			}
		}
		gettimeofday(&client->lastCommTime, NULL);
	} else (*isNewClient) = TRUE;
	/* Return TRUE if the client is registered and owns the channel */
	result = client != NULL && client == channelOwner;
	clientList_unlock(feedback_clientList);
	return result;
}

struct timeval lastTimeDeadClientsWereCleaned;

void cleanDeadClients() {
	uint32_t iterator;
	TClient *curClient;
	struct timeval currentTime;
	double elapsed;

	gettimeofday(&currentTime, NULL);
	elapsed = (currentTime.tv_sec - lastTimeDeadClientsWereCleaned.tv_sec) + (currentTime.tv_usec - lastTimeDeadClientsWereCleaned.tv_usec) / 1e6;
	if (elapsed < COMMAND_SERVER_DEAD_CLIENTS_CLEANUP_PERIOD) return;

	lastTimeDeadClientsWereCleaned = currentTime;
	clientList_lock(feedback_clientList);
	clientList_startIterator(&iterator);
	while((curClient = clientList_getNextClient(feedback_clientList, &iterator)) != NULL) {
		elapsed = (currentTime.tv_sec - curClient->lastCommTime.tv_sec) + (currentTime.tv_usec - curClient->lastCommTime.tv_usec) / 1e6;
		if (elapsed > COMMAND_SERVER_MAX_TIME_NO_COMMAND) {
			/* The client has been silent for too long: forget it */
			PRINT("[commandServer] Client %s:%d is not sending packets and it will be forgotten\n", inet_ntoa(curClient->remoteAddress), curClient->commandPort);
			commandServer_removeClient(curClient);
		}
	}
	clientList_unlock(feedback_clientList);
}

uint8_t firstTakeOffPacket = TRUE;

DEFINE_THREAD_ROUTINE(server, data) {
	uint8_t initError = 0;
	int32_t maxFinalWaitCounts = 80;

	feedback_clientList = clientList_create(FEEDBACK_SERVER_MAX_CLIENTS);
	// If there was an error initializing the resources, tell dependent threads through sharedInit
	// TODO: See TODO section at the start of the feedback channel thread
	if (initError) { commandServer_sharedInit = 2; signal_exit(); THREAD_RETURN(-1); }
	commandServer_sharedInit = 1;	// Mark shared resources as initialized, so dependent threads can start normally

	CommandPacket inputPacket, tmpPacket;
	while(!ardrone_tool_exit()) {
		/* Init server socket */
		vp_os_memset((int8_t *)&commandServerSocket, 0, sizeof(commandServerSocket));
		commandServerSocket.type = VP_COM_SERVER;
		commandServerSocket.protocol = VP_COM_UDP;
//		commandServerSocket.block = VP_COM_DONTWAIT;
		commandServerSocket.port = COMMAND_SERVER_PORT;
		strcpy(commandServerSocket.serverHost, "0.0.0.0");
		/* We use the generic communications interface to open the socket */
		if (vp_com_open(&globalCom, &commandServerSocket, &commandReader, &commandWriter) != VP_COM_OK)
			PRINT("[commandServer] Unable to start command socket\n");
		else {
			/* Set server socket timeout */
			struct timeval tm;
			tm.tv_sec = 0;
			tm.tv_usec = RECV_TIMEOUT * 1e6;
			setsockopt((int32_t)commandServerSocket.priv, SOL_SOCKET, SO_RCVTIMEO, &tm, sizeof(tm));

			channelOwner = NULL;	/* The channel has just started and it has no owner yet */
			while(!ardrone_tool_exit()) {
				bool_t outOfSeq, isNewClient;
				/* Read the socket using the socket interface directly (no equivalent in generic interface) */
				int32_t readBytes = sizeof(CommandPacket);
				if (commandReader(&commandServerSocket, (int8_t *)&tmpPacket, &readBytes) != VP_COM_OK)
					break;
				if (readBytes > 0) {
					struct in_addr remoteAddress;
					uint8_t receptionError = 1, accessGranted = 0;
//printf("Got something\n");
					/* Something was received. Check if it is a valid packet */
					if (readBytes == sizeof(CommandPacket)) {
						CommandPacket_ntoh(&tmpPacket);
//printf("size ok: 0x%x\n", inputPacket.signature);
						if (tmpPacket.signature == PROTOCOL_COMMAND_SIGNATURE) {
							/* It looks like a valid packet */
							memcpy(&inputPacket, &tmpPacket, sizeof(inputPacket));
							receptionError = 0;
						}
					}
					/* checkClientAccess returns FALSE if the client is not registered or is not the channel owner. */
					remoteAddress.s_addr = commandServerSocket.scn;
					accessGranted = checkClientAccess(&remoteAddress, commandServerSocket.port, receptionError, inputPacket.feedbackPort, getPacketIndex(&inputPacket), &outOfSeq, &isNewClient);
					if (!receptionError && !outOfSeq &&									/* IF the packet is valid and into sequence AND ( */
						(accessGranted || inputPacket.controlData.flyingMode == INIT) 	/* The client is the commander OR it is connecting */
						) {
						/* Process the command if it is valid and the client is granted access or it is new */
						processPacket(&inputPacket);
						/* If a non-TAKEOFF packet from the owner is received, the next TAKEOFF packet will communicate
						 * with the AR.Drone API. Otherwise, only the first TAKEOFF packet does it. If this measure is
						 * not taken, the AR.Drone doesn't respond to further commands after take-off.
						 */
						if (accessGranted && inputPacket.controlData.flyingMode != TAKEOFF)
							firstTakeOffPacket = TRUE;
					}
				}
				/* Monitor how long since last time each client sent something */
				cleanDeadClients();
			}
			if (!ardrone_tool_exit()) {
				// Something bad happened. Everything will be reset.
				// Removing all clients from the feedback list
				clientList_lock(feedback_clientList);
				clientList_removeAllClients(feedback_clientList);
				clientList_unlock(feedback_clientList);
			}
			vp_com_close(&globalCom, &commandServerSocket);
		}
		if (!ardrone_tool_exit()) {
			setDroneSafeState();

			/* Wait some time to retry opening the socket */
			PRINT("[commandServer] The command reception failed: reopening socket in %f seconds\n", SOCKET_REOPEN_GRACE_TIME_S);
			usleep((unsigned int)(SOCKET_REOPEN_GRACE_TIME_S * 1e6));
		}
	}

	// We must wait until all dependent threads end, before freeing shared resources
	while(dependentThreadsFinished < 1) {
		maxFinalWaitCounts--;
		if (maxFinalWaitCounts == 0) {
			PRINT("[commandServer] The navdata thread doesn't seem to end. Shared resources will be closed.\n");
			break;
		}
		usleep(50000);
	}
	clientList_destroy(feedback_clientList);

	THREAD_RETURN(0);
}

void processPacket(CommandPacket *p) {
	/* Get data from packet and send it to the drone */
	switch(p->controlData.flyingMode) {
		case TAKEOFF:
			if (firstTakeOffPacket) {
				// Recover from any emergency
				ardrone_tool_set_ui_pad_select(0);
				usleep(100000);
				ardrone_at_set_flat_trim();		/* We assume the take-off is from a flat surface */
				usleep(100000);
				// Set configuration parameters
				ardrone_control_config.video_enable = VIDEO_ENABLE != 0;
				ARDRONE_TOOL_CONFIGURATION_ADDEVENT(video_enable, &ardrone_control_config.video_enable, NULL);
				ardrone_control_config.vision_enable = DRONE_VISION_ENABLE != 0;
				ARDRONE_TOOL_CONFIGURATION_ADDEVENT(vision_enable, &ardrone_control_config.vision_enable, NULL);
				// Start take-off sequence
				ardrone_tool_set_ui_pad_start(1);
				firstTakeOffPacket = FALSE;
			}
			break;
		case LAND:
			ardrone_tool_set_ui_pad_start(0);
			break;
		case MOVE:
			ardrone_at_set_progress_cmd(1 << ARDRONE_PROGRESSIVE_CMD_ENABLE,
										p->controlData.phi,
										p->controlData.theta,
										p->controlData.gaz,
										p->controlData.yaw
										);
			break;
		case HOVER:
			ardrone_at_set_progress_cmd(0,
/*										p->controlData.phi,
										p->controlData.theta,
										p->controlData.gaz,
										p->controlData.yaw*/
										0, 0, 0, 0
										);
			break;
		case EMERGENCYSTOP:
			ardrone_tool_set_ui_pad_select(1);
			usleep(200000);
			ardrone_tool_set_ui_pad_select(0);
			usleep(200000);
			break;
		case INIT:
			/* Request exclusive access to the client list */
			clientList_lock(feedback_clientList);
			{
				struct in_addr remoteAddress;
				uint16_t commandPort;
				TClient *client;
				char *newClientHost;
				uint8_t isNewClient = 0;
				/* Get new client information: address, command port and feedback port */
				remoteAddress.s_addr = commandServerSocket.scn;	/* Address */
				commandPort = commandServerSocket.port;			/* Port */
				newClientHost = inet_ntoa(remoteAddress);
				/* Check if client is already in the list */
				client = clientList_getClientByInfo(feedback_clientList, &remoteAddress, commandPort);
				if (client == NULL) {
					isNewClient = 1;
					/* If not, add it to the list */
					client = commandServer_addNewClient(&remoteAddress, commandPort, p->feedbackPort, getPacketIndex(p));
					PRINT("[commandServer] New client from %s:%d (feedback port: %d)", newClientHost, commandPort, p->feedbackPort);
					if (client != NULL) {
						/* Ownership is given only if the new client claims it and there's no previous owner */
						if (p->controlData.requestedAccessMode == COMMANDER && channelOwner == NULL) {
							PRINT(" was added to the list as COMMANDER.\n");
						} else {
							PRINT(" was added to the list as LISTENER.\n");
						}
					} else PRINT(" couldn't be added to the feedback list\n");
				}
				if (client != NULL) {
					/* The client is registered: copy information */
					client->feedbackPort = p->feedbackPort;
					client->socket.port = p->feedbackPort;
					client->numBadPackets = 0;
					gettimeofday(&client->lastCommTime, NULL);
					/* Ownership is given only if the new client claims it and there's no previous owner */
					if (p->controlData.requestedAccessMode == COMMANDER && channelOwner == NULL) {
						client->isOwner = TRUE;
						channelOwner = client;
						if (!isNewClient) PRINT("[commandServer] The ownership of the channel was given to %s:%d\n", newClientHost, commandPort);
					} else {
						/* If the owner is not this client, mark it as listener */
						if (client != channelOwner)
							client->isOwner = FALSE;
					}
				}
			}
			clientList_unlock(feedback_clientList);
			break;
	}
	if (p->controlData.flyingMode != INIT && p->controlData.flyingMode != IDLE)
		currentDroneMode = p->controlData.flyingMode;
}
