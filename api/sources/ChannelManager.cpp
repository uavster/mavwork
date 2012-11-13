/*
 * ChannelManager.cpp
 *
 *  Created on: 22/08/2011
 *      Author: Ignacio Mellado
 */

#include <ChannelManager.h>
#include <sys/time.h>

#define DEFAULT_CONTROL_IP				"127.0.0.1"
#define DEFAULT_CONTROL_PORT			11235
#define DEFAULT_CONTROL_FREQ			50.0

#define DEFAULT_FEEDBACK_IP				"0.0.0.0"
#define DEFAULT_FEEDBACK_BASE_PORT		11237
#define DEFAULT_FEEDBACK_PORT_SPAN		20

#define DEFAULT_VIDEO_IP				"127.0.0.1"
#define DEFAULT_VIDEO_PORT				11236

#define DEFAULT_CONFIG_IP				"127.0.0.1"
#define DEFAULT_CONFIG_PORT				11234

#define DEFAULT_RANGER_IP				"127.0.0.1"
#define DEFAULT_RANGER_PORT				11238

namespace DroneProxy {
namespace Comm {

#define CHANNELMNGR_CONFIG_INDEX	0
#define CHANNELMNGR_CONTROL_INDEX	1
#define CHANNELMNGR_FEEDBACK_INDEX	2
#define CHANNELMNGR_VIDEO_INDEX		3

ChannelManager::ChannelManager()
	: cvgThread("channelManager") {
	openLog("ChannelManager");
	controlPort = -1;

	channels[0] = &configChannel;
	channels[1] = &controlChannel;
	channels[2] = &feedbackChannel;
	for (cvg_int i = 0; i < NUM_CHANNELS; i++)
		channelResetting[i] = false;

	numVidHosts = 0;
	videoChannel = NULL;
	vidChannelResetting = NULL;
	vidChannelResetStartTime = NULL;
	this->videoHost = NULL;
	this->videoPort = NULL;

	configChannel.addLogConsumer(this);
	controlChannel.addLogConsumer(this);
	feedbackChannel.addLogConsumer(this);

	feedbackChannel.setControlChannel(&controlChannel);

	controlChannel.addEventListener(this);
	feedbackChannel.addEventListener(this);

	cvgString videoHosts[] = { DEFAULT_VIDEO_IP, DEFAULT_RANGER_IP };
	cvg_int videoPorts[] = { DEFAULT_VIDEO_PORT, DEFAULT_RANGER_PORT };
	config(	DEFAULT_CONTROL_IP, CVG_LITERAL_INT(DEFAULT_CONTROL_PORT), DEFAULT_CONTROL_FREQ,
			DEFAULT_FEEDBACK_IP, CVG_LITERAL_INT(DEFAULT_FEEDBACK_BASE_PORT), CVG_LITERAL_INT(DEFAULT_FEEDBACK_PORT_SPAN),
			DEFAULT_CONFIG_IP, CVG_LITERAL_INT(DEFAULT_CONFIG_PORT),
			2, videoHosts, videoPorts
			);

	created = false;
}

ChannelManager::~ChannelManager() {
	destroy();
	feedbackChannel.removeLogConsumer(this);
	controlChannel.removeLogConsumer(this);
//	for (cvg_int i = 0; i < numVidHosts; i++)
//		videoChannel[i]->removeLogConsumer(this);
	configChannel.removeLogConsumer(this);
	closeLog();

	if (this->videoHost != NULL) { delete [] this->videoHost; this->videoHost = NULL; }
	if (this->videoPort != NULL) { delete [] this->videoPort; this->videoPort = NULL; }
}

void ChannelManager::config(cvgString ctrlHost, cvg_int ctrlPort, cvg_float ctrlFreq,
		cvgString fbHost, cvg_int fbBasePort, cvg_int fbPortSpan,
		cvgString confHost, cvg_int confPort,
		cvg_int numVidHosts, cvgString *vidHosts, cvg_int *vidPorts) {
	controlHost = ctrlHost; controlPort = ctrlPort; controlFreq = ctrlFreq;
	feedbackHost = fbHost; feedbackBasePort = fbBasePort; feedbackPortSpan = fbPortSpan;
	configHost = confHost; configPort = confPort;

	if (this->videoHost != NULL) { delete [] this->videoHost; this->videoHost = NULL; }
	if (this->videoPort != NULL) { delete [] this->videoPort; this->videoPort = NULL; }

	this->videoHost = new cvgString[numVidHosts];
	this->videoPort = new cvg_int[numVidHosts];

	this->numVidHosts = numVidHosts;
	for (cvg_int i = 0; i < numVidHosts; i++) {
		this->videoHost[i] = vidHosts[i];
		this->videoPort[i] = vidPorts[i];
	}
}

void ChannelManager::run() {
	try {
		for (cvg_int i = 0; i < NUM_CHANNELS; i++) {
			if (channels[i]->getCurrentState() == Channel::FAULT) {
				if (!channelResetting[i]) {
					log("The %s channel failed. It will be reopened in %.1f seconds.", channels[i]->getChannelName().c_str(), CHANNEL_RESET_GRACE_TIME);
					channelResetting[i] = true;
					channelResetStartTime[i].restart(false);
				} else {
					if (channelResetStartTime[i].getElapsedSeconds() > CHANNEL_RESET_GRACE_TIME) {
						// close() will set the DISCONNECTED state
						channels[i]->close();
						// If the feedback channel must be restarted, so does the control channel
						if (i == CHANNELMNGR_FEEDBACK_INDEX) channels[CHANNELMNGR_CONTROL_INDEX]->close();
						channelResetting[i] = false;
					}
				}
			}
		}
		for (cvg_int i = 0; i < numVidHosts; i++) {
			if (videoChannel[i]->getCurrentState() == Channel::FAULT) {
				if (!vidChannelResetting[i]) {
					log("The %s channel failed. It will be reopened in %.1f seconds.", videoChannel[i]->getChannelName().c_str(), CHANNEL_RESET_GRACE_TIME);
					vidChannelResetting[i] = true;
					vidChannelResetStartTime[i].restart(false);
				} else {
					if (vidChannelResetStartTime[i].getElapsedSeconds() > CHANNEL_RESET_GRACE_TIME) {
						// close() will set the DISCONNECTED state
						videoChannel[i]->close();
						vidChannelResetting[i] = false;
					}
				}
			}
		}

		// If disconnected, connect configuration channel
		if (configChannel.getCurrentState() == Channel::DISCONNECTED) {
			try {
				log("Opening configuration channel at %s:%d...", configHost.c_str(), configPort);
				configChannel.open(configHost, configPort);
				log("Configuration channel opened.");
			} catch(cvgException e) {
				log("Error opening configuration channel. Cause: %s", e.getMessage().c_str());
			}
		}

		// If disconnected, connect control channel
		if (controlChannel.getCurrentState() == Channel::DISCONNECTED) {
			try {
				log("Starting control channel to %s:%d...", controlHost.c_str(), controlPort);
				controlChannel.open(controlHost, controlPort, requiredAccessMode, requiredPriority, controlFreq);
				feedbackChannel.setControlPort(controlChannel.getLocalPort());
				log("Control channel sending commands...");
			} catch(cvgException e) {
				feedbackChannel.setControlPort(-1);
				log("Error starting control channel. Cause: %s", e.getMessage().c_str());
			}
		}

		// If disconnected, connect feedback channel
		if (feedbackChannel.getCurrentState() == Channel::DISCONNECTED) {
			try {
				log("Starting feedback channel at %s:[%d, %d]...", feedbackHost.c_str(), feedbackBasePort, feedbackBasePort + feedbackPortSpan);
				feedbackChannel.open(feedbackHost, feedbackBasePort, feedbackPortSpan);
				// Notify controlChannel the new feedback port value
				controlChannel.setFeedbackPort(feedbackChannel.getPort());
				log("Feedback channel waiting for data at %s:%d...", feedbackChannel.getHost().c_str(), feedbackChannel.getPort());
			} catch(cvgException e) {
				log("Error starting feedback channel. Cause: %s", e.getMessage().c_str());
			}
		}

		// If disconnected, connect video channel
		for (cvg_int i = 0; i < numVidHosts; i++) {
			if (videoChannel[i]->getCurrentState() == Channel::DISCONNECTED) {
				try {
					log("Opening video channel %d at %s:%d...", i, videoHost[i].c_str(), videoPort[i]);
					videoChannel[i]->open(videoHost[i], videoPort[i]);
					log("Video channel %d opened.", i);
				} catch(cvgException e) {
					log("Error opening video channel %d. Cause: %s", i, e.getMessage().c_str());
				}
			}
		}

	} catch(cvgException e) {
		log("Exception: %s", e.getMessage().c_str());
	}
	usleep(100000);
}

void ChannelManager::create(AccessMode accessMode, cvg_uchar priority) {
	destroy();

	lastCommandedFlyingMode = IDLE;

	requiredAccessMode = accessMode;
	requiredPriority = priority;
	if (controlPort < 0) throw cvgException("[ChannelManager] You must call config() before create()");

	videoChannel = new VideoChannel *[numVidHosts];
	for (cvg_int i = 0; i < numVidHosts; i++) {
		videoChannel[i] = new VideoChannel;
		videoChannel[i]->addLogConsumer(this);
	}
	this->vidChannelResetStartTime = new Timing::Timer[numVidHosts];
	this->vidChannelResetting = new cvg_bool[numVidHosts];
	bzero(this->vidChannelResetting, sizeof(this->vidChannelResetting));

	created = true;
}

void ChannelManager::destroy() {
	if (!created) return;
	log("Closing manager...");
	try {
		stop();
	} catch(cvgException e) {
		log(e.getMessage());
	}
	log("Closing video channels...");
	try {
		if (videoChannel != NULL) {
			for (cvg_int i = 0; i < numVidHosts; i++) {
				videoChannel[i]->close();
				delete videoChannel[i];
			}
			delete [] videoChannel;
			videoChannel = NULL;
		}
		if (vidChannelResetStartTime != NULL) {
			delete [] vidChannelResetStartTime;
			vidChannelResetStartTime = NULL;
		}
		if (vidChannelResetting != NULL) {
			delete [] vidChannelResetting;
			vidChannelResetting = NULL;
		}
 	} catch(cvgException e) {
		log(e.getMessage());
	}
	log("Closing control channel...");
	try {
		controlChannel.close();
	} catch(cvgException e) {
		log(e.getMessage());
	}
	log("Closing feedback channel...");
	try {
		feedbackChannel.close();
	} catch(cvgException e) {
		log(e.getMessage());
	}
	log("Manager closed");
	created = false;
}

void ChannelManager::stateChanged(Channel *channel, Channel::State oldState, Channel::State newState) {
	// This method is executed from any of the channel threads
	if (channel == &feedbackChannel) {
		if (oldState != Channel::CONNECTED && newState == Channel::CONNECTED) {
			// The server received an INIT message and started sending feedback messages.
			// Now, the control channel can start sending command messages.

/*			// Before setting a command channel state, ensure the pose commands are equal to the drone's state,
			// so no steps are sent to the drone. The gaz command stays unchanged.
			FeedbackPacket droneInfo;
			feedbackChannel.getDroneInfo(&droneInfo);
			controlChannel.setControlData(	ControlChannel::PHI | ControlChannel::THETA | ControlChannel::YAW,
											droneInfo.feedbackData.roll,
											droneInfo.feedbackData.pitch,
											0,
											droneInfo.feedbackData.yaw
											);*/
//			controlChannel.setControlData(0.0f, 0.0f, 0.0f, 0.0f);
			AccessMode grantedAccess = getGrantedAccessMode();
			if (grantedAccess == requiredAccessMode) {
				// The granted access is like requested, stop sending INITs. Otherwise, keep on sending them,
				// so we get the desired access mode eventually, when the command channel is free.
				controlChannel.setGrantedAccessMode(grantedAccess);
				controlChannel.setFlyingMode(lastCommandedFlyingMode);
			}
			log("Proxy connection established");
		}
		if (oldState != Channel::FAULT && newState == Channel::FAULT) {
			// The feedback channel failed: set the control channel to init mode again.
			controlChannel.setFlyingMode(INIT);
			// Set config channel in fault state too. This channel acts on demand by the application and does not have
			// any beacon packets. Therefore, a channel loss will only be detected after a read/write of any configuration
			// parameter. Setting the early fault state, we help the channel to come back to life as soon as possible.
			configChannel.setCurrentState(Channel::FAULT);
			log("Proxy connection lost");
		}
	}
}

cvg_bool ChannelManager::connToDrone() {
	if (!connToProxy()) return false;
	FeedbackPacket fp;
	feedbackChannel.getDroneInfo(&fp);
	return fp.feedbackData.commWithDrone != 0;
}

cvg_bool ChannelManager::connToProxy() {
	return 	controlChannel.getCurrentState() == Channel::CONNECTED &&
			feedbackChannel.getCurrentState() == Channel::CONNECTED;
}

FlyingMode ChannelManager::getControlMode() {
	return controlChannel.getFlyingMode();
}

AccessMode ChannelManager::getGrantedAccessMode() {
	if (!connToProxy()) return UNKNOWN_MODE;
	FeedbackPacket fp;
	feedbackChannel.getDroneInfo(&fp);
	return (AccessMode)fp.feedbackData.grantedAccessMode;
}

cvg_bool ChannelManager::setControlMode(FlyingMode m) {
	if (!connToDrone()) return false;
	controlChannel.setFlyingMode(m);
	lastCommandedFlyingMode = m;
	return true;
}

cvg_bool ChannelManager::setControlData(cvg_int changes, cvg_float phi, cvg_float theta, cvg_float gaz, cvg_float yaw) {
	if (!connToDrone()) return false;
	controlChannel.setControlData(changes, phi, theta, gaz, yaw);
	return true;
}

cvg_bool ChannelManager::setControlData(cvg_float phi, cvg_float theta, cvg_float gaz, cvg_float yaw) {
	if (!connToDrone()) return false;
	controlChannel.setControlData(phi, theta, gaz, yaw);
	return true;
}

void ChannelManager::logConsume(LogProducer *producer, const cvgString &str) {
	log(str);
}

void ChannelManager::gotData(Channel *channel, void *data) {
	if (channel == &feedbackChannel) {
		FeedbackPacket::feedbackData__ *fb = (FeedbackPacket::feedbackData__ *)data;
		AccessMode grantedAccess = (AccessMode)fb->grantedAccessMode;
		if (grantedAccess != controlChannel.getGrantedAccessMode()) {
			log(cvgString("Access granted as ") + (grantedAccess == COMMANDER ? "COMMANDER" : "LISTENER"));
		}
		controlChannel.setGrantedAccessMode(grantedAccess);
/*		if (fb->commWithDrone && fb->droneMode == HOVERING) {
			if (controlChannel.getFlyingMode() == TAKEOFF)
				controlChannel.setFlyingMode(HOVER);
		}*/
	}
}

}
}
