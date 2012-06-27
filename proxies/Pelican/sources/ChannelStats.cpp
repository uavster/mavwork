/*
 * ChannelStats.cpp
 *
 *  Created on: 04/05/2012
 *      Author: Ignacio Mellado-Bataller
 */

#include "comm/ChannelStats.h"

#define THROUGHPUT_MEASURE_WINDOW_SECONDS	0.5

namespace Comm {

void ChannelStats::updateStats(cvg_bool r_w, cvg_int packetIncrement, Stats *outStats) {
	cvgMutex *statsMutex = r_w ? &recvStatsMutex : &sendStatsMutex;
	Stats *stats = r_w ? &recvStats : &sendStats;
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

void ChannelStats::notifyReceived(cvg_int numPackets) {
	updateStats(true, numPackets, NULL);
}

void ChannelStats::notifySent(cvg_int numPackets) {
	updateStats(false, numPackets, NULL);
}

}
