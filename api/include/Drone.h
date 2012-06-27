/*
 * Drone.h
 *
 *  Created on: 05/09/2011
 *      Author: Ignacio Mellado
 */

#ifndef DRONE_H_
#define DRONE_H_

#include "ChannelManager.h"
#include "VideoProcessor.h"
#include "LogProducer.h"

typedef FeedbackPacket::feedbackData__ FeedbackData;

namespace DroneProxy {

class Drone : public Logs::LogProducer {
private:
	Comm::ChannelManager channelManager;
	Processing::VideoProcessor **videoProcessor;
	cvg_bool opened;
	cvgString host;
	cvg_bool needConfigUpdate;

	Threading::Mutex lastRecvFbackMutex;
	FeedbackData lastReceivedFeedback;
	FeedbackData trimData;
	cvg_bool mustTrim;

	// It is better not to expose the ChannelListener interface with the Drone class
	// We use an internal custom listener instead
	class CommEventListener : public virtual Comm::ChannelEventListener {
	private:
		Drone *drone;
	public:
		inline void setDrone(Drone *drone) { this->drone = drone; }
		virtual void gotData(Comm::Channel *channel, void *data);
		virtual void stateChanged(Comm::Channel *channel, Comm::Channel::State oldState, Comm::Channel::State newState);
	};
	CommEventListener commEventListener;

protected:
	static void processFrame(void *instance, cvg_int cameraId, cvg_ulong timeCode, VideoFormat format, cvg_uint width, cvg_uint height, cvg_char *frameData);

	// Override this method in the child class to process video data
	virtual void processVideoFrame(cvg_int cameraId, cvg_ulong timeCode, VideoFormat format, cvg_uint width, cvg_uint height, cvg_char *frameData);

	// Override this method in the child class to process feedback data
	virtual void processFeedback(FeedbackData *feedbackData);

	// OVerride this method with the configuration write commands
	virtual void setConfigParams();

public:
	Drone();
	virtual ~Drone();

	void open(const cvgString &host, AccessMode accessMode = COMMANDER, cvg_uchar priority = 0);
	void close();

	inline cvg_bool isOpen() { return opened; }
	inline cvgString getHost() { return host; }

	// Information about connections state
	inline cvg_bool connToProxy() { return channelManager.connToProxy(); }
	inline cvg_bool connToDrone() { return channelManager.connToDrone(); }

	// Control mode access
	inline FlyingMode getControlMode() { return channelManager.getControlMode(); }
	inline cvg_bool setControlMode(FlyingMode m) { return channelManager.setControlMode(m); }

	// Control data access
	inline cvg_bool setControlData(cvg_int changes, cvg_float phi, cvg_float theta, cvg_float gaz, cvg_float yawSpeed) {
		return channelManager.setControlData(changes, phi, theta, gaz, yawSpeed);
	}
	inline cvg_bool setControlData(cvg_float phi, cvg_float theta, cvg_float gaz, cvg_float yawSpeed) {
		return channelManager.setControlData(phi, theta, gaz, yawSpeed);
	}

	// Logs
	inline void addLogConsumer(Logs::LogConsumer *lc) {
		channelManager.addLogConsumer(lc);
		LogProducer::addLogConsumer(lc);
	}
	inline void removeLogConsumer(Logs::LogConsumer *lc) {
		channelManager.removeLogConsumer(lc);
		LogProducer::removeLogConsumer(lc);
	}

	// Config
	inline void writeParam(cvg_int id, const cvg_char *buffer, cvg_int length) { channelManager.getConfigChannel().writeParam(id, buffer, length); }
	inline void writeParam(cvg_int id, cvg_float value) { channelManager.getConfigChannel().writeParam(id, value); }
	inline void writeParam(cvg_int id, cvg_int value) { channelManager.getConfigChannel().writeParam(id, value); }
	inline void writeParam(cvg_int id, const cvgString &value) { channelManager.getConfigChannel().writeParam(id, value); }

	inline void readParamArray(cvg_int id, cvg_char *buffer, cvg_int length) { channelManager.getConfigChannel().readParamArray(id, buffer, length); }
	inline cvg_float readParamFloat(cvg_int id) { return channelManager.getConfigChannel().readParamFloat(id); }
	inline cvg_int readParamInt(cvg_int id) { return channelManager.getConfigChannel().readParamInt(id); }
	inline cvgString readParamString(cvg_int id, cvg_int length) { return channelManager.getConfigChannel().readParamString(id, length); }

	void trim();

	// This inner objects provide advanced low-level functionalities
	inline Comm::ChannelManager &getChannelManager() { return channelManager; }
	inline Processing::VideoProcessor &getVideoProcessor(cvg_int i) { return *videoProcessor[i]; }
};

}

#endif /* DRONE_H_ */
