/*
 * Channel.cpp
 *
 *  Created on: 22/08/2011
 *      Author: Ignacio Mellado
 */

#include <Channel.h>
#include <ChannelEventListener.h>

#define THROUGHPUT_MEASURE_WINDOW_SECONDS	0.5

using namespace std;

namespace DroneProxy {
namespace Comm {

Channel::Channel() {
	currentState = DISCONNECTED;
}

Channel::~Channel() {
	close();
}

void Channel::open(cvgString host, cvg_int port) {
	this->host = host;
	this->port = port;
	recvStats.reset();
	sendStats.reset();
}

void Channel::close() {
}

void Channel::addEventListener(ChannelEventListener *cel) {
	list<ChannelEventListener *>::iterator i;
	for(i = eventListeners.begin(); i != eventListeners.end(); i++)
		if ((*i) == cel) return;

	eventListeners.push_back(cel);
	cel->stateChanged(this, UNKNOWN, currentState);
}

void Channel::notifyStateChange(State oldState, State newState) {
	list<ChannelEventListener *>::iterator i;
	for(i = eventListeners.begin(); i != eventListeners.end(); i++)
		(*i)->stateChanged(this, oldState, newState);
}

void Channel::updateStats(cvg_bool r_w, cvg_int packetIncrement, Statistics *outStats) {
	DroneProxy::Threading::Mutex *statsMutex = r_w ? &recvStatsMutex : &sendStatsMutex;
	Statistics *stats = r_w ? &recvStats : &sendStats;
	if (statsMutex->lock()) {
		stats->throughputPacketCount += packetIncrement;
		// Throughput calculation
		cvg_double elapsed = stats->lastThroughputUpdateTime.getElapsedSeconds();
		if (elapsed >= THROUGHPUT_MEASURE_WINDOW_SECONDS) {
			stats->throughput = (cvg_float)(stats->throughputPacketCount / elapsed);
			stats->lastThroughputUpdateTime.restart();
			stats->throughputPacketCount = 0;
		}
		if (outStats != NULL) (*outStats) = (*stats);
		statsMutex->unlock();
	}
}

void Channel::notifyReceivedData(void *data) {
	updateStats(true, 1, NULL);

	list<ChannelEventListener *>::iterator i;
	for(i = eventListeners.begin(); i != eventListeners.end(); i++)
		(*i)->gotData(this, data);
}

void Channel::notifySentData(void *data) {
	updateStats(false, 1, NULL);

	list<ChannelEventListener *>::iterator i;
	for(i = eventListeners.begin(); i != eventListeners.end(); i++)
		(*i)->sentData(this, data);
}

void Channel::setCurrentState(State newState) {
	if (currentState != newState) {
		State oldState = currentState;
		currentState = newState;
		notifyStateChange(oldState, newState);
	}
}

cvgString Channel::stateToString(State st) {
	cvgString str;
	switch(st) {
	case Channel::DISCONNECTED: str = "DISCONNECTED"; break;
	case Channel::CONNECTING: str = "CONNECTING"; break;
	case Channel::CONNECTED: str = "CONNECTED"; break;
	case Channel::DISCONNECTING: str = "DISCONNECTING"; break;
	case Channel::FAULT: str = "FAULT"; break;
	default:
	case Channel::UNKNOWN: str = "UNKNOWN"; break;
	}
	return str;
}

}
}
