/*
 * CamUeye.h
 *
 *  Created on: 22/05/2012
 *      Author: Ignacio Mellado-Bataller
 */

#ifndef CAMUEYE_H_
#define CAMUEYE_H_

#include "../IVideoSource.h"
#include <highgui.h>

//#define CAMUEYE_TEST_MODE

namespace Video {

class CamUeye_driver;

class CamUeye : public virtual IVideoSource {
private:
	CamUeye_driver *driver;
	cvg_int width, height;
#ifdef CAMUEYE_TEST_MODE
	IplImage *im;
#endif

public:
	CamUeye();

	virtual void open();
	virtual void close();

	virtual cvg_int getWidth();
	virtual cvg_int getHeight();
	virtual cvg_int getFrameLength();
	virtual VideoFormat getFormat();
	virtual cvg_ulong getTicksPerSecond();

	virtual cvg_bool captureFrame(void *frameBuffer, cvg_ulong *timestamp = NULL);
};

}

#endif /* CAMUEYE_H_ */
