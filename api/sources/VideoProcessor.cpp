/*
 * VideoProcessor.cpp
 *
 *  Created on: 04/09/2011
 *      Author: Ignacio Mellado
 */

#include <VideoProcessor.h>
#include <Protocol.h>
#include <string.h>
#include <jpeg.h>

namespace Processing {

using namespace DroneProxy::Comm;

#define DEFAULT_BUFFER_INITIAL_WIDTH	640
#define DEFAULT_BUFFER_INITIAL_HEIGHT	480
#define MAX_POSSIBLE_NUM_PLANES			3
#define MAX_POSSIBLE_BYTES_PER_PLANE	2

#define GET_WORST_CASE_LENGTH(width, height)	(width * height * MAX_POSSIBLE_NUM_PLANES * MAX_POSSIBLE_BYTES_PER_PLANE)
#define DEFAULT_BUFFER_LENGTH					GET_WORST_CASE_LENGTH(DEFAULT_BUFFER_INITIAL_WIDTH, DEFAULT_BUFFER_INITIAL_HEIGHT)
#define FRAME_BUFFER_GROWTH_FACTOR				0.2

VideoProcessor::VideoProcessor(cvg_int id)
	: cvgThread((cvgString("videoProcessor") + id).c_str()) {
	openLog("VideoProcessor");
	frameProcessingCallback = NULL;
	frameBuffer = NULL;
	decodedBuffer = NULL;
	frameBufferLength = 0;
	memset(&frameInfo, 0, sizeof(frameInfo));
	videoChannel = NULL;
	finish = false;
	this->id = id;
}

VideoProcessor::~VideoProcessor() {
	destroy();
	closeLog();
}

void VideoProcessor::create(VideoChannel *vCh) {
	destroy();
	try {
		memset(&frameInfo, 0, sizeof(frameInfo));
		frameBufferLength = DEFAULT_BUFFER_LENGTH;
		frameBuffer = new cvg_char[DEFAULT_BUFFER_LENGTH];
		decodedBuffer = new cvg_char[DEFAULT_BUFFER_LENGTH];
		videoChannel = vCh;
		videoChannel->addEventListener(this);
		// The thread will be started by the state change to CONNECTED on the video channel
	} catch(cvgException e) {
		destroy();
		throw e;
	}
}

void VideoProcessor::destroy() {
	cvg_bool wasStarted = isStarted();
	if (wasStarted) {
		log("Closing video processor...");
		selfStop();
		if (videoChannel->getFrameBufferMutex()->lock()) {
			finish = true;
			bufferCondition.signal();
			videoChannel->getFrameBufferMutex()->unlock();
		}
		try {
			stop();
		} catch(cvgException e) {
			log(e.getMessage());
		}
	}
	if (videoChannel != NULL) {
		videoChannel->removeEventListener(this);
		videoChannel = NULL;
	}
	frameProcessingCallback = NULL;
	if (frameBuffer != NULL) {
		delete [] frameBuffer;
		frameBuffer = NULL;
		frameBufferLength = 0;
	}
	if (decodedBuffer != NULL) {
		delete [] decodedBuffer;
		decodedBuffer = NULL;
	}
	finish = false;
	if (wasStarted)
		log("Video processor closed");
}

void VideoProcessor::run() {
	if (!videoChannel->getFrameBufferMutex()->lock()) { usleep(10000); return; }
	// We own the access to the channel buffer.
	// Copy data to our buffer to lock the channel as short as possible.
	try {
		bufferCondition.wait(videoChannel->getFrameBufferMutex());
		if (!finish) {
			cvg_uint neededLength = videoChannel->getFrameInfo()->videoData.dataLength;
			if (neededLength > frameBufferLength) {
				if (frameBuffer != NULL) delete [] frameBuffer;
				if (decodedBuffer != NULL) delete [] decodedBuffer;
				frameBufferLength = (cvg_int)(neededLength * (1.0 + FRAME_BUFFER_GROWTH_FACTOR));
				frameBuffer = new cvg_char[frameBufferLength];
				decodedBuffer = new cvg_char[frameBufferLength];
			}
			memcpy(&frameInfo, videoChannel->getFrameInfo(), sizeof(frameInfo));
			memcpy(frameBuffer, videoChannel->getFrameBuffer(), neededLength);
		}

		videoChannel->getFrameBufferMutex()->unlock();

		// The frame is now in our buffer and we have time to process it without disturbing the channel.
		if (!finish) process();
	} catch(cvgException e) {
		videoChannel->getFrameBufferMutex()->unlock();
		// Send the exception to the log listeners
		log(e.getMessage());
		usleep(10000);
	}
}

void VideoProcessor::stateChanged(Channel *channel, Channel::State oldState, Channel::State newState) {
	if (channel != videoChannel) return;
	// If the video channel connection is closed, stop our thread because things are gonna become invalid
	if (oldState != Channel::DISCONNECTED && newState == Channel::DISCONNECTED) {
		if (isStarted()) {
			log("The video channel is disconnected. Stopping video processor...");
			selfStop();
			finish = true;
			bufferCondition.signal();
			stop();
			log("Video processor stopped");
		}
	}
	// If the video channel is connected, start our processing thread
	else if (oldState != Channel::CONNECTED && newState == Channel::CONNECTED) {
		log("The video channel is connected. Starting video processor...");
		finish = false;
		start();
		log("Video processor started");
	}
}

void VideoProcessor::gotData(Channel *channel, void *data) {
	if (channel != videoChannel) return;
	bufferCondition.signal();
}

void VideoProcessor::process() {
	if (frameProcessingCallback != NULL) {
		// Decode frame if necessary
		cvg_char *decBuffer = NULL;
		cvg_int width, height;
		switch(frameInfo.videoData.encoding) {
			case RAW_BGR24:
			case RAW_D16:
				decBuffer = frameBuffer;
				break;
			case JPEG:
				jpeg_decompress((unsigned char *)decodedBuffer, (unsigned char *)frameBuffer, frameInfo.videoData.dataLength, &width, &height);
				if (!jpeg_isError && width == frameInfo.videoData.width && height == frameInfo.videoData.height)
					decBuffer = decodedBuffer;
				break;
		}
		// If no appropriate decoder is found, do not continue processing
		if (decBuffer != NULL)
			// Send frame for camera 0
			frameProcessingCallback(callbackData,
									id,
									(((cvg_ulong)frameInfo.videoData.timeCodeH) << 32) | (cvg_ulong)frameInfo.videoData.timeCodeL,
									(VideoFormat)frameInfo.videoData.encoding,
									frameInfo.videoData.width,
									frameInfo.videoData.height,
									decBuffer
									);
	}
}

}
