/*
 * FeedbackChannel.h
 *
 *  Created on: 22/08/2011
 *      Author: Ignacio Mellado
 */

#ifndef FEEDBACKCHANNEL_H_
#define FEEDBACKCHANNEL_H_

#define FEEDBACK_CHANNEL_MAX_SILENT_TIME 	0.5f

#include "Channel.h"
#include <arpa/inet.h>
#include "Protocol.h"
#include "Mutex.h"
#include "LogProducer.h"
#include "ControlChannel.h"

namespace DroneProxy {
namespace Comm {

class FeedbackChannel : public Channel, public virtual cvgThread, public virtual DroneProxy::Logs::LogProducer {
	friend class ChannelManager;
private:
	sockaddr_in socketAddress;
	int feedbackSocket;
	cvg_bool gotFirstPacket;
	DroneProxy::Threading::Mutex packetAccessMutex;
	FeedbackPacket feedbackPacket;
	cvg_int localCommandPort;
	cvg_ulong lastPacketIndex;
	ControlChannel *controlChannel;

protected:
	inline void setControlPort(cvg_int port) { localCommandPort = port; }
	inline void setControlChannel(ControlChannel *controlChannel) { this->controlChannel = controlChannel; }
	virtual void run();

public:
	FeedbackChannel();
	virtual ~FeedbackChannel();

	inline virtual void open(cvgString host, cvg_int basePort) { open(host, basePort, 20); }
	void open(cvgString host, cvg_int basePort, cvg_int portSpan);
	virtual void close();

	void getDroneInfo(FeedbackPacket *info);
};

}
}

#endif /* FEEDBACKCHANNEL_H_ */
