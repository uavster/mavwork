/*
 * HokuyoCam.h
 *
 *  Created on: 19/06/2012
 *      Author: Ignacio Mellado-Bataller
 */

#ifndef HOKUYOCAM_H_
#define HOKUYOCAM_H

#include "Hokuyo.h"
#include "../IVideoSource.h"

namespace Video {

class HokuyoCam : public virtual IVideoSource {
public:
	HokuyoCam();

	virtual void open();
	virtual void close();

	virtual cvg_int getWidth();
	virtual cvg_int getHeight();
	virtual cvg_int getFrameLength();
	virtual VideoFormat getFormat();
	virtual cvg_ulong getTicksPerSecond();

	virtual cvg_bool captureFrame(void *frameBuffer, cvg_ulong *timestamp = NULL);

private:
	Hokuyo sensor;
	Hokuyo::SampleBuffer *buffer;
	cvg_int numBins;
};

}

#endif /* HOKUYOCAM_H_ */
