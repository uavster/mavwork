/*
 * ChannelStats.h
 *
 *  Created on: 04/05/2012
 *      Author: Ignacio Mellado-Bataller
 */

#ifndef CHANNELSTATS_H_
#define CHANNELSTATS_H_

#include <atlante.h>

namespace Comm {

class ChannelStats {
public:

	class Stats {
		friend class ChannelStats;
	private:
		cvg_uint throughputPacketCount;
		cvgTimer lastThroughputUpdateTime;
	protected:
		inline void reset() {
			throughputPacketCount = throughput = jitter = 0.0f;
			lastThroughputUpdateTime.restart(false);
		}
	public:
		cvg_float	throughput;
		cvg_float	jitter;

		inline Stats() { reset(); }
		inline Stats &operator = (Stats &s) {
			throughput = s.throughput;
			jitter = s.jitter;
			return (*this);
		}
	};

	inline Stats getRecvStatistics() { Stats st; updateStats(true, 0, &st); return st; }
	inline Stats getSendStatistics() { Stats st; updateStats(false, 0, &st); return st; }

	void notifyReceived(cvg_int numPackets = 1);
	void notifySent(cvg_int numPackets = 1);

private:
	cvgMutex recvStatsMutex;
	Stats recvStats;
	cvgMutex sendStatsMutex;
	Stats sendStats;

protected:
	void updateStats(cvg_bool r_w, cvg_int packetIncrement, Stats *throughput);
};

}

#endif /* CHANNELSTATS_H_ */
