/*
 * Vid1TcpInetServer.h
 *
 *  Created on: 22/05/2012
 *      Author: Ignacio Mellado-Bataller
 */

#define FPS_LIMIT			10

#include "comm/Vid1TcpInetServer.h"

#define template_def		//template<class _VideoT>
#define Vid1TcpInetServer_	Vid1TcpInetServer//<_VideoT>

#include "comm/application/video/Vid1Protocol.h"
#include "cam/IVideoSource.h"
#include <string.h>

namespace Comm {

template_def Vid1TcpInetServer_::Vid1TcpInetServer() {
	frameFeeder.setParent(this);
}

template_def void Vid1TcpInetServer_::open(const IAddress &addr, cvg_int maxQueueLength) {
	VideoServer_base::open(addr, maxQueueLength);
	frameFeeder.start();
	VideoServer_base::addEventListener(&frameFeeder);
}

template_def void Vid1TcpInetServer_::close() {
	VideoServer_base::removeEventListener(&frameFeeder);
	frameFeeder.stop();
	VideoServer_base::close();
}

template_def void Vid1TcpInetServer_::FrameFeeder::gotEvent(EventGenerator &source, const Event &event) {
	parent->processEvent(source, event);
}

template_def void Vid1TcpInetServer_::processEvent(EventGenerator &source, const Event &event) {
	const Video::GotVideoFrameEvent *gvfe = dynamic_cast<const Video::GotVideoFrameEvent *>(&event);
	if (gvfe != NULL) {
		if (fpsTimer.getElapsedSeconds() <= 1.0 / FPS_LIMIT) return;
		fpsTimer.restart();

		if (getNumClients() == 0) return;

//		cvg_ulong timeCode = 1e6 * timer.getElapsedSeconds();

		if (!frameMutex.lock()) throw cvgException("[Vid1TcpInetServer] Cannot lock frame mutex");
		header.signature = PROTOCOL_VIDEOFRAME_SIGNATURE;
		header.videoData.dataLength = gvfe->frameLength;
		header.videoData.encoding = gvfe->format;
		header.videoData.width = gvfe->width;
		header.videoData.height = gvfe->height;
		header.videoData.timeCodeH = (gvfe->timestamp >> 32) & 0xFFFFFFFF;
		header.videoData.timeCodeL = gvfe->timestamp & 0xFFFFFFFF;
		frameBuffer.copy(gvfe->frameData, gvfe->frameLength);
//std::cout << "Frame -> frameCondition.signal(): " << (1e-6 * (cvgTimer::getSystemSeconds() * 1e6 - gvfe->timestamp)) << " s" << std::endl;
		frameCondition.signal();
		frameMutex.unlock();
	}
}

template_def void Vid1TcpInetServer_::FrameFeeder::run() {
	if (!parent->frameMutex.lock()) throw cvgException("[Vid1TcpInetServer] Cannot lock frame mutex");
	cvg_bool timeout = !parent->frameCondition.wait(&parent->frameMutex, 0.25);
//cvg_ulong ftime = (((cvg_ulong)parent->header.videoData.timeCodeH) << 32) | (cvg_ulong)parent->header.videoData.timeCodeL;
//std::cout << "Frame -> frame feeder: " << (1e-6 * (cvgTimer::getSystemSeconds() * 1e6 - ftime)) << " s" << std::endl;
	Proto::Vid1::Vid1FrameHeader tmpHeader;
	if (!timeout) {
		memcpy(&tmpHeader, &parent->header, sizeof(Proto::Vid1::Vid1FrameHeader));
		parent->frameBuffer2 = parent->frameBuffer;
	}
	parent->frameMutex.unlock();

	if (timeout) return;

	VideoFormat inputFormat = (VideoFormat)tmpHeader.videoData.encoding;
	tmpHeader.videoData.encoding = parent->getOutputFormat();
	parent->feedFrame(	&tmpHeader, inputFormat, &parent->frameBuffer2,
						tmpHeader.videoData.dataLength,
						tmpHeader.videoData.width,
						tmpHeader.videoData.height,
						&tmpHeader.videoData.dataLength,
						&tmpHeader.videoData.width,
						&tmpHeader.videoData.height);
}

}

#undef Vid1TcpInetServer_
#undef template_def



