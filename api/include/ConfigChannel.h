/*
 * ConfigChannel.h
 *
 *  Created on: 15/01/2012
 *      Author: Ignacio Mellado
 */

#ifndef CONFIGCHANNEL_H_
#define CONFIGCHANNEL_H_

#define CONFIG_CHANNEL_DEFAULT_LINGER_SECONDS		1
#define CONFIG_CHANNEL_MAX_OPERATION_SECONDS		1.0

#include <atlante.h>
#include "Protocol.h"
#include <arpa/inet.h>
#include "Channel.h"
#include "Mutex.h"
#include "Condition.h"
#include "LogProducer.h"

namespace DroneProxy {
namespace Comm {

class ConfigChannel : public DroneProxy::Comm::Channel, public virtual DroneProxy::Logs::LogProducer {
private:
	int configSocket;

protected:
	void sendBuffer(const void *buffer, cvg_int length, cvg_bool partial = false);
	void recvBuffer(void *buffer, cvg_int length, cvg_bool justPurge = false);

public:
	ConfigChannel();
	virtual ~ConfigChannel();

	virtual void open(cvgString host, cvg_int port);
	virtual void close();

	void writeParam(cvg_int id, const cvg_char *buffer, cvg_int length);
	void writeParam(cvg_int id, cvg_float value);
	void writeParam(cvg_int id, cvg_int value);
	void writeParam(cvg_int id, const cvgString &value);

	void readParamArray(cvg_int id, cvg_char *buffer, cvg_int length);
	cvg_float readParamFloat(cvg_int id);
	cvg_int readParamInt(cvg_int id);
	cvgString readParamString(cvg_int id, cvg_int length);
};

}
}

#endif /* CONFIGCHANNEL_H_ */
