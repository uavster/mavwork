/*
 * IVideoSource.h
 *
 *  Created on: 22/05/2012
 *      Author: Ignacio Mellado-Bataller
 */

#ifndef IVIDEOSOURCE_H_
#define IVIDEOSOURCE_H_

#include <atlante.h>
#include "../base/Event.h"

namespace Video {

class IVideoSource {
public:
	typedef enum { RAW_BGR24 = 0, RAW_RGB24 = 1, JPEG = 2, RAW_D16 = 3, RAW_BAYER8 = 4, ANY_FORMAT = 100 } VideoFormat;

	virtual void open() = 0;
	virtual void close() = 0;

	virtual cvg_int getWidth() = 0;
	virtual cvg_int getHeight() = 0;
	virtual cvg_int getFrameLength() = 0;
	virtual VideoFormat getFormat() = 0;
	virtual cvg_ulong getTicksPerSecond() = 0;

	// This method blocks until a new image is captured or an error occurs
	virtual cvg_bool captureFrame(void *frameBuffer, cvg_ulong *timestamp = NULL) = 0;
};

class GotVideoFrameEvent : public virtual Event {
public:
	inline GotVideoFrameEvent() {
		frameData = NULL;
	}

	inline GotVideoFrameEvent(void *frameData, cvg_int width, cvg_int height, cvg_int frameLength, char format, cvg_ulong timestamp, cvg_ulong ticksPerSecond) {
		this->frameData = frameData;
		this->width = width;
		this->height = height;
		this->frameLength = frameLength;
		this->format = format;
		this->timestamp = timestamp;
		this->ticksPerSecond = ticksPerSecond;
	}

	void *frameData;
	cvg_int width, height, frameLength;
	cvg_ulong timestamp, ticksPerSecond;
	char format;
};

}

#endif /* IVIDEOSOURCE_H_ */
