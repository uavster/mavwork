/*
 * ChannelEventListener.h
 *
 *  Created on: 22/08/2011
 *      Author: nacho
 */

#ifndef CHANNELEVENTLISTENER_H_
#define CHANNELEVENTLISTENER_H_

#include "Channel.h"

namespace DroneProxy {
namespace Comm {

class ChannelEventListener {
public:
	virtual void stateChanged(Channel *channel, Channel::State oldState, Channel::State newState);
	virtual void gotData(Channel *channel, void *data);
	virtual void sentData(Channel *channel, void *data);
};

}
}

#endif /* CHANNELEVENTLISTENER_H_ */
