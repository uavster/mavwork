/*
 * AsyncVideoTranscoder.cpp
 *
 *  Created on: 19/10/2012
 *      Author: Ignacio Mellado-Bataller
 */

#include <iostream>
#include "cam/AsyncVideoTranscoder.h"

namespace Video {

void AsyncVideoTranscoder::setOutputFeatures(IVideoSource::VideoFormat dstFormat, cvg_int dstWidth, cvg_int dstHeight) {
	if (!frameMutex.lock()) throw cvgException("[AsyncVideoTranscoder::setOutputFeatures] Unable to lock frame buffer");
	dstFeatures.format = dstFormat;
	dstFeatures.width = dstWidth;
	dstFeatures.height = dstHeight;
	outputFeaturesSet = true;
	frameMutex.unlock();
}

void AsyncVideoTranscoder::feedFrame(const GotVideoFrameEvent &e) {
	feedFrame((IVideoSource::VideoFormat)e.format, e.width, e.height, e.frameData, e.timestamp, e.ticksPerSecond);
}

void AsyncVideoTranscoder::feedFrame(IVideoSource::VideoFormat srcFormat, cvg_int srcWidth, cvg_int srcHeight, const void *srcData, cvg_ulong srcTimeCode, cvg_ulong srcTicksPerSec) {
	if (!frameMutex.lock()) throw cvgException("[AsyncVideoTranscoder::feedFrame] Unable to lock frame buffer");
	try {
		srcFeatures.format = srcFormat;
		srcFeatures.width = srcWidth;
		srcFeatures.height = srcHeight;
		srcFeatures.timestamp = srcTimeCode;
		srcFeatures.ticksPerSecond = srcTicksPerSec;
		srcFeatures.frameLength = transcoder.getFrameSizeInBytes(srcFormat, srcWidth, srcHeight);
		inputBuffer.copy(srcData, srcFeatures.frameLength);
//std::cout << "Frame " << srcTimeCode << " with format " << transcoder.getFormatName(srcFormat) << " signaled to the transcoder" << std::endl;
		frameCondition.signal();
		frameMutex.unlock();
	} catch (cvgException &e) {
		frameMutex.unlock();
		throw e;
	}
}

void AsyncVideoTranscoder::run() {
	try {
		if (!frameMutex.lock()) throw cvgException("[AsyncVideoTranscoder::run] Unable to lock frame buffer");
		// Check if setOutputFeatures() was called first...
		if (outputFeaturesSet) {
			// ... It was. Wait for a new frame to transcode
			cvg_bool gotFrame = frameCondition.wait(&frameMutex, 0.25);
			if (gotFrame) {
				// Got a frame; copy to an auxiliary buffer to fast unlock the mutex
				auxBuffer = inputBuffer;
				auxSrcFeatures = srcFeatures;
				auxDstFeatures = dstFeatures;
			}
			frameMutex.unlock();

			if (gotFrame) {
				// Got a frame; transcode it
				if (auxDstFeatures.width == ASYNCVIDEOTRANSCODER_WIDTH_ANY) auxDstFeatures.width = auxSrcFeatures.width;
				if (auxDstFeatures.height == ASYNCVIDEOTRANSCODER_HEIGHT_ANY) auxDstFeatures.height = auxSrcFeatures.height;
				auxDstFeatures.frameLength = transcoder.getFrameSizeInBytes((IVideoSource::VideoFormat)auxDstFeatures.format, auxDstFeatures.width, auxDstFeatures.height);
				outputBuffer.resize(auxDstFeatures.frameLength);
				auxDstFeatures.frameData = &outputBuffer;
				transcoder.transcodeFrame((IVideoSource::VideoFormat)auxSrcFeatures.format, auxSrcFeatures.width, auxSrcFeatures.height, &auxBuffer, (IVideoSource::VideoFormat)auxDstFeatures.format, auxDstFeatures.width, auxDstFeatures.height, &outputBuffer);
//std::cout << "Frame " << auxSrcFeatures.timestamp << " transcoded from " << transcoder.getFormatName((IVideoSource::VideoFormat)auxSrcFeatures.format).c_str() << " to " << transcoder.getFormatName((IVideoSource::VideoFormat)dstFeatures.format).c_str() << std::endl;
				// Notify all listeners with a copy of the features (so nobody can modify it)
				GotVideoFrameEvent e = GotVideoFrameEvent(auxDstFeatures);
				// Keep the original timing information
				e.timestamp = auxSrcFeatures.timestamp;
				e.ticksPerSecond = auxSrcFeatures.ticksPerSecond;
				notifyEvent(e);
			}
		} else {
			frameMutex.unlock();
			usleep(100000); // Not doing anything because no defined output format
		}
	} catch (cvgException &e) {
		frameMutex.unlock();
		std::cout << "[EXCEPTION] [AsyncVideoTranscoder::run()] " << e.getMessage() << std::endl;
	}
}

void AsyncVideoTranscoder::gotEvent(EventGenerator &source, const Event &e) {
	const GotVideoFrameEvent *gvfe = dynamic_cast<const GotVideoFrameEvent *>(&e);
	if (gvfe != NULL) {
		feedFrame(*gvfe);
	}
}

}
