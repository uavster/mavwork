/*
 * Channel.h
 *
 *  Created on: 22/08/2011
 *      Author: Ignacio Mellado
 */

#ifndef CHANNEL_H_
#define CHANNEL_H_

#include <atlante.h>
#include <list>
#include <sys/time.h>
#include "Mutex.h"
#include "Timer.h"

namespace DroneProxy {
namespace Comm {

class ChannelEventListener;

class Channel {
public:
	enum State { DISCONNECTED, CONNECTING, CONNECTED, DISCONNECTING, FAULT, UNKNOWN };
	class Statistics {
		friend class Channel;
	private:
		cvg_uint throughputPacketCount;
		DroneProxy::Timing::Timer lastThroughputUpdateTime;
	protected:
		inline void reset() {
			throughputPacketCount = 0;
			throughput = 0.0f;
			jitter = 0.0f;
			lastThroughputUpdateTime.restart(false);
		}
	public:
		cvg_float	throughput;
		cvg_float	jitter;

		inline Statistics() { reset(); }
		inline ~Statistics() { }
		inline Statistics &operator = (Statistics &s) {
			throughput = s.throughput;
			jitter = s.jitter;
		}

	};

private:
	std::list<ChannelEventListener *> eventListeners;
	volatile State currentState;
	cvgString host;
	cvg_int port;

	DroneProxy::Threading::Mutex recvStatsMutex;
	Statistics recvStats;
	DroneProxy::Threading::Mutex sendStatsMutex;
	Statistics sendStats;

	cvgString name;

protected:
	inline void setChannelName(const char *name) { this->name = name; }
	void notifyStateChange(State oldState, State newState);
	void notifyReceivedData(void *data);
	void notifySentData(void *data);
	void updateStats(cvg_bool r_w, cvg_int packetIncrement, Statistics *throughput);

public:
	Channel();
	virtual ~Channel();

	virtual void open(cvgString host, cvg_int port);
	virtual void close();
	void addEventListener(ChannelEventListener *csl);
	inline void removeEventListener(ChannelEventListener *csl) { eventListeners.remove(csl); }
	inline void removeAllEventListeners() { eventListeners.clear(); }
	void setCurrentState(State state);
	inline State getCurrentState() { return currentState; }
	static cvgString stateToString(State s);
	inline Statistics getRecvStatistics() { Statistics st; updateStats(true, 0, &st); return st; }
	inline Statistics getSendStatistics() { Statistics st; updateStats(false, 0, &st); return st; }
	inline cvgString &getHost() { return host; }
	inline cvg_int getPort() { return port; }
	inline cvgString getChannelName() { return name; }
};

}
}

#endif /* CHANNEL_H_ */
