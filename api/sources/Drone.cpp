/*
 * Drone.cpp
 *
 *  Created on: 05/09/2011
 *      Author: Ignacio Mellado
 */
#include <stdio.h>

#include <Drone.h>

#define DRONE_CONTROL_PORT			11235
#define DRONE_CONTROL_FREQ			60.0

#define DRONE_FEEDBACK_IP			"0.0.0.0"
#define DRONE_FEEDBACK_BASE_PORT	11237
#define DRONE_FEEDBACK_PORT_SPAN	20

#define DRONE_VIDEO_PORT			11236

#define DRONE_CONFIG_PORT			11234

#define DRONE_RANGER_PORT			11238

namespace DroneProxy {

Drone::Drone() {
	openLog("Drone");
	opened = false;
	commEventListener.setDrone(this);
	mustTrim = false;
	needConfigUpdate = false;
	videoProcessor = NULL;
}

Drone::~Drone() {
	close();
	closeLog();
}

void Drone::open(const cvgString &host, AccessMode accessMode, cvg_uchar priority) {
	close();
	opened = true;
	this->host = host;
	cvgString vidHosts[] = { host, host };
	cvg_int vidPorts[] = { DRONE_VIDEO_PORT, DRONE_RANGER_PORT };
	channelManager.config(	host, CVG_LITERAL_INT(DRONE_CONTROL_PORT), DRONE_CONTROL_FREQ,
							DRONE_FEEDBACK_IP, CVG_LITERAL_INT(DRONE_FEEDBACK_BASE_PORT), CVG_LITERAL_INT(DRONE_FEEDBACK_PORT_SPAN),
							host, CVG_LITERAL_INT(DRONE_CONFIG_PORT),
							2, vidHosts, vidPorts
							);
	channelManager.create(accessMode, priority);
	channelManager.getFeedbackChannel().addEventListener(&commEventListener);
	channelManager.getConfigChannel().addEventListener(&commEventListener);

	videoProcessor = new Processing::VideoProcessor *[channelManager.getNumVideoChannels()];
	for (cvg_int i = 0; i < channelManager.getNumVideoChannels(); i++) {
		videoProcessor[i] = new Processing::VideoProcessor(i);
		videoProcessor[i]->create(&channelManager.getVideoChannel(i));
		videoProcessor[i]->setProcessingCallback(&processFrame, this);
	}

	channelManager.start();
}

void Drone::close() {
	channelManager.getConfigChannel().removeEventListener(&commEventListener);
	channelManager.getFeedbackChannel().removeEventListener(&commEventListener);
	if (videoProcessor != NULL) {
		for (cvg_int i = 0; i < channelManager.getNumVideoChannels(); i++) {
			videoProcessor[i]->destroy();
			delete videoProcessor[i];
		}
		delete [] videoProcessor;
		videoProcessor = NULL;
	}
	channelManager.destroy();
	opened = false;
}

void Drone::processFrame(void *instance, cvg_int cameraId, cvg_ulong timeCode, VideoFormat format, cvg_uint width, cvg_uint height, cvg_char *frameData) {
	((Drone *)instance)->processVideoFrame(cameraId, timeCode, format, width, height, frameData);
}

void Drone::processVideoFrame(cvg_int cameraId, cvg_ulong timeCode, VideoFormat format, cvg_uint width, cvg_uint height, cvg_char *frameData) {
}

void Drone::processFeedback(FeedbackData *feedbackData) {
}

void Drone::trim() {
	if (!lastRecvFbackMutex.lock()) throw cvgException("[Drone::trim] Unable to lock last received feedback data");
	memcpy(&trimData, &lastReceivedFeedback, sizeof(FeedbackData));
	mustTrim = true;
	lastRecvFbackMutex.unlock();
}

void Drone::CommEventListener::gotData(Comm::Channel *channel, void *data) {
	if (channel == &drone->getChannelManager().getFeedbackChannel()) {
		// Feedback channel event
		if (drone->lastRecvFbackMutex.lock()) {
			// Restore user-defined configuration if needed
			if (((FeedbackData *)data)->grantedAccessMode == COMMANDER &&			// IF the access mode is Commander AND (
				(
					drone->needConfigUpdate ||		// a config update was triggered by a communication-with-proxy recovery OR
					drone->lastReceivedFeedback.grantedAccessMode != COMMANDER ||	// Commander access was just gained OR
					(drone->lastReceivedFeedback.commWithDrone == 0 && ((FeedbackData *)data)->commWithDrone != 0) // the communication with the drone has been just recovered)
				)
				)
			{
				try {
					drone->setConfigParams();
				} catch(cvgException e) {
					drone->log(e.getMessage());
				}
			}
			drone->needConfigUpdate = false;

			// Store last received info
			memcpy(&drone->lastReceivedFeedback, data, sizeof(FeedbackData));
			// If there'd been a trim request, take offset off the Euler angles
			if (drone->mustTrim) {
				((FeedbackData *)data)->yaw -= drone->trimData.yaw;
				((FeedbackData *)data)->pitch -= drone->trimData.pitch;
				((FeedbackData *)data)->roll -= drone->trimData.roll;
			}
			drone->lastRecvFbackMutex.unlock();
		}
		drone->processFeedback((FeedbackData *)data);
	}
}

void Drone::CommEventListener::stateChanged(Comm::Channel *channel, Comm::Channel::State oldState, Comm::Channel::State newState) {
	if (channel == &drone->getChannelManager().getConfigChannel()) {
		// Restore user-defined configuration if needed
		if (oldState != Comm::Channel::CONNECTED && newState == Comm::Channel::CONNECTED) {
			// gotData() will be called right after stateChanged() and we need the config to get updated
			drone->needConfigUpdate = true;
		}
	}
}

void Drone::setConfigParams() {
}

}

