/*
 * ControlChannel.h
 *
 *  Created on: 15/08/2011
 *      Author: nacho
 */

#ifndef CONTROLCHANNEL_H_
#define CONTROLCHANNEL_H_

#include <atlante.h>
#include "Protocol.h"
#include <arpa/inet.h>
#include "Channel.h"
#include "Mutex.h"
#include "Condition.h"
#include "LogProducer.h"

namespace DroneProxy {
namespace Comm {

class ControlChannel : public virtual cvgThread, public DroneProxy::Comm::Channel, public virtual DroneProxy::Logs::LogProducer {
	friend class ChannelManager;
	friend class FeedbackChannel;
private:
	DroneProxy::Threading::Condition packetReadyCondition;
	sockaddr_in socketAddress;
	int controlSocket;
	DroneProxy::Threading::Mutex packetAccessMutex;
	CommandPacket commandPacket;
	cvg_double loopWaitTime;
	volatile AccessMode grantedAccessMode;
	cvg_ulong packetIndex;	// Even at 1000 packets/s, this would overflow in more than 500 million years
	volatile cvg_ushort remoteControlPort;

//	FlyingMode lastCommandedFlyingMode;
//	volatile cvg_bool updateCommandedFlyingModeAfterRecovery;

protected:
	void resetCommandPacket();
	virtual void run();

	void setFeedbackPort(cvg_ushort fbPort);
	void setRemoteControlPort(cvg_ushort rcPort);
	void setGrantedAccessMode(AccessMode grantedAccess);
	cvg_int getLocalPort();

public:
	ControlChannel();
	virtual ~ControlChannel();

	void open(cvgString host, cvg_int port, AccessMode access, cvg_char requestedPriority, cvg_float resendFreq);
	virtual void close();
	inline AccessMode getGrantedAccessMode() { return grantedAccessMode; }

	void setFlyingMode(FlyingMode m);
	FlyingMode getFlyingMode();

	enum ControlProperty { PHI = 1, THETA = 2, GAZ = 4, YAW = 8 };
	void setControlData(cvg_int changes, cvg_float phi, cvg_float theta, cvg_float gaz, cvg_float yaw);
	void setControlData(cvg_float phi, cvg_float theta, cvg_float gaz, cvg_float yaw);
	void getControlData(CommandPacket *cmdPacket);
};

}
}

#endif /* CONTROLCHANNEL_H_ */
