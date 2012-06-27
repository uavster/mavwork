/*
 * ChannelEventListener.cpp
 *
 *  Created on: 22/08/2011
 *      Author: Ignacio Mellado
 */

#include <ChannelEventListener.h>
#include <Channel.h>

namespace DroneProxy {
namespace Comm {

void ChannelEventListener::stateChanged(Channel *channel, Channel::State oldState, Channel::State newState) {
}

void ChannelEventListener::gotData(Channel *channel, void *data) {
}

void ChannelEventListener::sentData(Channel *channel, void *data) {
}

}
}
