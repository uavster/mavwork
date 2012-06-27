#include <ardrone_tool/Navdata/ardrone_navdata_client.h>

#include <ardrone_api.h>
#include <Navdata/navdata.h>
#include <control_states.h>
#include <stdio.h>
#include <VP_Com/vp_com.h>
#include <VP_Com/vp_com_socket.h>
#include <Server/commandServer.h>
#include <VP_Os/vp_os_print.h>
#include <VP_Com/vp_com_error.h>
#include <Server/Protocol.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <unistd.h>
#include <errno.h>
#include "ClientList.h"

/* --- Interface with commandServer --- */
extern uint8_t commandServer_sharedResourcesInitStatus();
extern void commandServer_notifyDependentThreadFinished();
extern TClientList *commandServer_getFeedbackClientList();
extern void commandServer_removeClient(TClient *client);
/* ------------------------------------ */
extern C_RESULT signal_exit();

#define FEEDBACK_SAMPLE_FREQ_DIVIDER		6
#define FEEDBACK_CHANNEL_FREQ				25

#define ARDRONE_CTRL_STATE_DEFAULT	0
#define ARDRONE_CTRL_STATE_INIT		1
#define ARDRONE_CTRL_STATE_LANDED	2
#define ARDRONE_CTRL_STATE_FLYING	3
#define ARDRONE_CTRL_STATE_HOVERING	4
#define ARDRONE_CTRL_STATE_TEST		5
#define ARDRONE_CTRL_STATE_TAKEOFF	6
#define ARDRONE_CTRL_STATE_GOTOFIX	7
#define ARDRONE_CTRL_STATE_LANDING	8

char ardroneCtrlStateToFeedbackState(int st) {
	switch(st) {
	case ARDRONE_CTRL_STATE_DEFAULT:
	case ARDRONE_CTRL_STATE_LANDED:
		return LANDED;
	case ARDRONE_CTRL_STATE_FLYING:
		return FLYING;
	case ARDRONE_CTRL_STATE_HOVERING:
		return HOVERING;
	case ARDRONE_CTRL_STATE_TAKEOFF:
		return TAKINGOFF;
	case ARDRONE_CTRL_STATE_LANDING:
		return LANDING;
	case ARDRONE_CTRL_STATE_GOTOFIX:
		return EMERGENCY;
	case ARDRONE_CTRL_STATE_INIT:
	case ARDRONE_CTRL_STATE_TEST:
	default:
		return UNKNOWN;
	}
}

extern bool_t ardrone_tool_exit();

extern vp_com_t globalCom;
vp_com_socket_t navdataSocket;

volatile bool_t samplingThreadFinished = TRUE;

/* Initialization of local variables before event loop  */
inline C_RESULT demo_navdata_client_init( void* data )
{
	samplingThreadFinished = FALSE;
	return C_OK;
}

char localBrainHost[64] = "";

pthread_mutex_t packetMutex;
volatile uint8_t newPacket;
pthread_cond_t packetCondition;
FeedbackPacket feedbackPacket, tmpPacket;
uint64_t currentPacketIndex = 0;

volatile char packetMutexInitialized = 0;

struct timeval lastInfoUpdate;
int32_t	sampleClockDivider = 0;

/* Receving navdata during the event loop */
inline C_RESULT demo_navdata_client_process( const navdata_unpacked_t* const navdata )
{
	struct timeval tv;

	sampleClockDivider++;
	if (sampleClockDivider < FEEDBACK_SAMPLE_FREQ_DIVIDER) return C_OK;
	sampleClockDivider = 0;

	gettimeofday(&tv, NULL);

	if (packetMutexInitialized && pthread_mutex_lock(&packetMutex) == 0) {
		uint64_t timeCode;
		const navdata_demo_t*nd = &navdata->navdata_demo;

		timeCode = (uint64_t)tv.tv_sec * 1e6 + (uint64_t)tv.tv_usec;
		feedbackPacket.feedbackData.timeCodeH = (unsigned int)(timeCode >> 32);
		feedbackPacket.feedbackData.timeCodeL = (unsigned int)(timeCode & 0xFFFFFFFF);
		feedbackPacket.feedbackData.commWithDrone = 1;
		feedbackPacket.feedbackData.droneMode = ardroneCtrlStateToFeedbackState(nd->ctrl_state >> 16);
		feedbackPacket.feedbackData.nativeDroneState = nd->ctrl_state;
		feedbackPacket.feedbackData.batteryLevel = nd->vbat_flying_percentage / 100.0f;
		feedbackPacket.feedbackData.roll = nd->phi / 1000.0f;
		feedbackPacket.feedbackData.pitch = nd->theta / 1000.0f;
		feedbackPacket.feedbackData.yaw = nd->psi / 1000.0f;
		feedbackPacket.feedbackData.altitude = nd->altitude / 1000.0f;
		feedbackPacket.feedbackData.speedX = nd->vx / 1000.0f;
		feedbackPacket.feedbackData.speedY = nd->vy / 1000.0f;
		feedbackPacket.feedbackData.speedYaw = nd->vz / 1000.0f;
		FeedbackPacket_hton(&feedbackPacket);

		gettimeofday(&lastInfoUpdate, NULL);

		newPacket = 1;
		pthread_cond_signal(&packetCondition);

		pthread_mutex_unlock(&packetMutex);
	}

	return C_OK;
}

void resetPacket() {
	vp_os_memset((int8_t *)&feedbackPacket, 0, sizeof(feedbackPacket));
	FeedbackPacket_hton(&feedbackPacket);
}

DEFINE_THREAD_ROUTINE(feedbackClient, data) {
	/* TODO: We now control the thread launch code. This wait would not be needed if the resource initialization was
	 * done before launching control and feedback channel threads in ardrone_testing_tool, instead of doing it in the
	 * command channel thread. However, this works too.
	 */
	// Wait until the commandServer thread has initialized the shared resources
	int maxWaitLoops = 80;
	uint8_t sris;
	while((sris = commandServer_sharedResourcesInitStatus()) == 0) {
		maxWaitLoops--;
		if (maxWaitLoops == 0) {
			PRINT("[navdata] FATAL ERROR: The commandServer thread doesn't seem to start\n");
			signal_exit();
			THREAD_RETURN(-1);
		}
		usleep(50000);
	}
	// If shared resources were not initialized correctly by the commandServer thread, close this thread
	if (sris == 2) { THREAD_RETURN(-1); }

	resetPacket();
	if (pthread_mutex_init(&packetMutex, NULL) != 0) {
		PRINT("[navdata] FATAL ERROR creating packetMutex");
		THREAD_RETURN(-1);
	}
	newPacket = 0;
	if (pthread_cond_init(&packetCondition, NULL) != 0) {
		PRINT("[navdata] FATA ERROR creating packetCondition");
		THREAD_RETURN(-1);
	}
	packetMutexInitialized = 1;
	while(!ardrone_tool_exit()) {

		if (pthread_mutex_lock(&packetMutex) == 0) {
			struct timeval tv;
			struct timespec ts;
			int res = 0;

			if (feedbackPacket.feedbackData.commWithDrone) {
				// If too long since last update from the drone, mark faulty connection
				struct timeval currentTime;
				gettimeofday(&currentTime, NULL);
				double elapsed = (currentTime.tv_sec - lastInfoUpdate.tv_sec) + (currentTime.tv_usec - lastInfoUpdate.tv_usec) / 1e6;
				if (elapsed > NAVDATA_MAX_TIME_NO_DRONE_UPDATES)
					feedbackPacket.feedbackData.commWithDrone = 0;
			}

			// Setup wait timeout
			gettimeofday(&tv, NULL);
			uint32_t timeoutSeconds = (uint32_t)floor(1.0 / FEEDBACK_CHANNEL_FREQ);
			ts.tv_sec = tv.tv_sec + timeoutSeconds;
			ts.tv_nsec = 1000 * tv.tv_usec + (uint64_t)(1000000000L * ((1.0 / FEEDBACK_CHANNEL_FREQ) - timeoutSeconds));
			if (ts.tv_nsec >= 1000000000L) {
				ts.tv_sec++;
				ts.tv_nsec -= 1000000000L;
			}
			// Wait for a data change to be signaled from the other thread, or continue after the timeout
			while(!newPacket && res != ETIMEDOUT) {
				res = pthread_cond_timedwait(&packetCondition, &packetMutex, &ts);
			}
			if (!newPacket) {
				struct timeval tv;
				gettimeofday(&tv, NULL);
				//uint64_t timeCode = (uint64_t)tv.tv_sec * 1e6 + (uint64_t)tv.tv_usec;
				feedbackPacket.feedbackData.timeCodeH = 0; //htonl((unsigned int)(timeCode >> 32));
				feedbackPacket.feedbackData.timeCodeL = 0; //htonl((unsigned int)(timeCode & 0xFFFFFFFF));
			}
			/* Copy data to end using feedbackPacket as soon as possible */
			memcpy(&tmpPacket, &feedbackPacket, sizeof(FeedbackPacket));
			newPacket = 0;
			pthread_mutex_unlock(&packetMutex);

			{
				uint32_t iterator;
				TClient *client;
				int32_t length = sizeof(FeedbackPacket);

				/*						struct sockaddr_in socketAddress;
				socketAddress.sin_family = AF_INET;
				socketAddress.sin_port = htons(navdataSocket.port);
				socketAddress.sin_addr.s_addr = navdataSocket.scn;*/

				tmpPacket.indexH = htonl((uint32_t)(currentPacketIndex >> 32));
				tmpPacket.indexL = htonl((uint32_t)(currentPacketIndex & 0xFFFFFFFF));
				currentPacketIndex++;
				/* Send data to all clients in feedback list */
				clientList_lock(commandServer_getFeedbackClientList());
				clientList_startIterator(&iterator);
				while((client = clientList_getNextClient(commandServer_getFeedbackClientList(), &iterator)) != NULL) {
					tmpPacket.feedbackData.grantedAccessMode = client->isOwner ? COMMANDER : LISTENER;
					tmpPacket.commandPort = htons(client->commandPort);
//					if (sendto((int)client->socket.priv, &tmpPacket, length, MSG_NOSIGNAL, (struct sockaddr *)&socketAddress, sizeof(struct sockaddr_in)) != length) {
					if (client->socketWriter(&client->socket, (int8_t *)&tmpPacket, &length) != VP_COM_OK ||
						length != sizeof(FeedbackPacket)) {
						PRINT("[navdata] Unable to send feedback data to client. It will be removed.\n");
						commandServer_removeClient(client);
					}
				}
				clientList_unlock(commandServer_getFeedbackClientList());
			}
		}
	}
	int32_t maxWaitCount = 80;
	packetMutexInitialized = 0;
	while(!samplingThreadFinished && maxWaitCount > 0) {
		usleep(50000);
		maxWaitCount--;
	}
	if (!samplingThreadFinished) PRINT("[navdata] The sampling thread didn't seem to finish. Exiting anyway.\n");
	pthread_cond_destroy(&packetCondition);
	pthread_mutex_destroy(&packetMutex);

	commandServer_notifyDependentThreadFinished();

	THREAD_RETURN(0);
}

/* Relinquish the local resources after the event loop exit */
inline C_RESULT demo_navdata_client_release( void )
{
	samplingThreadFinished = TRUE;
	return C_OK;
}

/* Registering to navdata client */
BEGIN_NAVDATA_HANDLER_TABLE
  NAVDATA_HANDLER_TABLE_ENTRY(demo_navdata_client_init, demo_navdata_client_process, demo_navdata_client_release, NULL)
END_NAVDATA_HANDLER_TABLE

