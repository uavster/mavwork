/*
 * ChannelManager.h
 *
 *  Created on: 22/08/2011
 *      Author: Ignacio Mellado
 */

#ifndef CHANNELMANAGER_H_
#define CHANNELMANAGER_H_

#define CHANNEL_RESET_GRACE_TIME	0.0

#include <atlante.h>
#include "ControlChannel.h"
#include "FeedbackChannel.h"
#include "VideoChannel.h"
#include "ConfigChannel.h"
#include "ChannelEventListener.h"
#include <stdarg.h>
#include "LogProducer.h"

namespace DroneProxy {
namespace Comm {

class ChannelManager : public virtual cvgThread, public virtual ChannelEventListener, public DroneProxy::Logs::LogProducer, public DroneProxy::Logs::LogConsumer {
private:
	ControlChannel	controlChannel;
	FeedbackChannel	feedbackChannel;
	VideoChannel 	**videoChannel;
	ConfigChannel	configChannel;

	cvg_bool	*vidChannelResetting;
	DroneProxy::Timing::Timer	*vidChannelResetStartTime;

	cvgString 	controlHost;
	cvg_int 	controlPort;
	cvg_float 	controlFreq;

	cvgString 	feedbackHost;
	cvg_int 	feedbackBasePort;
	cvg_int		feedbackPortSpan;

	cvg_int numVidHosts;
	cvgString	*videoHost;
	cvg_int		*videoPort;

	cvgString	configHost;
	cvg_int		configPort;

	enum NumChannels { NUM_CHANNELS = 3 };
	Channel		*channels[NUM_CHANNELS];
	cvg_bool	channelResetting[NUM_CHANNELS];
	DroneProxy::Timing::Timer	channelResetStartTime[NUM_CHANNELS];

	AccessMode requiredAccessMode;
	cvg_uchar requiredPriority;

	cvg_bool created;

	FlyingMode lastCommandedFlyingMode;

protected:
	virtual void run();
	virtual void stateChanged(Channel *channel, Channel::State oldState, Channel::State newState);
	virtual void gotData(Channel *channel, void *data);
	virtual void logConsume(LogProducer *producer, const cvgString &str);

public:
	ChannelManager();
	virtual ~ChannelManager();

	void config(	cvgString ctrlHost, cvg_int ctrlPort, cvg_float ctrlFreq,
					cvgString fbHost, cvg_int fbBasePort, cvg_int fbPortSpan,
					cvgString confHost, cvg_int confPort,
					cvg_int numVidHosts, cvgString *vidHosts, cvg_int *vidPorts
				);
	void create(AccessMode accessMode = COMMANDER, cvg_uchar priority = 0);
	void destroy();

	inline ControlChannel &getControlChannel() { return controlChannel; }
	inline FeedbackChannel &getFeedbackChannel() { return feedbackChannel; }
	inline VideoChannel &getVideoChannel(cvg_int i) { return *videoChannel[i]; }
	inline ConfigChannel &getConfigChannel() { return configChannel; }
	inline cvg_int getNumVideoChannels() { return numVidHosts; }

	cvg_bool connToProxy();
	cvg_bool connToDrone();
	AccessMode getGrantedAccessMode();

	FlyingMode getControlMode();
	cvg_bool setControlMode(FlyingMode m);

	cvg_bool setControlData(cvg_int changes, cvg_float phi, cvg_float theta, cvg_float gaz, cvg_float yaw);
	cvg_bool setControlData(cvg_float phi, cvg_float theta, cvg_float gaz, cvg_float yaw);
};

}
}

#endif /* CHANNELMANAGER_H_ */
