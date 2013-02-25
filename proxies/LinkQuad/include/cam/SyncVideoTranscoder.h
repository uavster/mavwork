/*
 * SyncVideoTranscoder.h
 *
 *  Created on: 18/10/2012
 *      Author: Ignacio Mellado-Bataller
 */

#ifndef SYNCVIDEOTRANSCODER_H_
#define SYNCVIDEOTRANSCODER_H_

#include <atlante.h>
#include <cv.h>
#include "IVideoSource.h"

namespace Video {

class SyncVideoTranscoder {
public:
	SyncVideoTranscoder();
	~SyncVideoTranscoder();

	static cvg_bool canTranscode(IVideoSource::VideoFormat sourceFormat, IVideoSource::VideoFormat destFormat);
	void transcodeFrame(IVideoSource::VideoFormat sourceFormat, cvg_int sourceWidth, cvg_int sourceHeight, void *sourceData, IVideoSource::VideoFormat destFormat, cvg_int destWidth, cvg_int destHeight, void *destData);
	static cvgString getFormatName(IVideoSource::VideoFormat format);
	static cvg_int getFrameSizeInBytes(IVideoSource::VideoFormat format, cvg_int width, cvg_int height);

protected:
	void setupIplImages(IVideoSource::VideoFormat sourceFormat, cvg_int sourceWidth, cvg_int sourceHeight, void *sourceData, IVideoSource::VideoFormat destFormat, cvg_int destWidth, cvg_int destHeight, void *destData);
	static void getFormatDepthAndPlanes(IVideoSource::VideoFormat format, int *iplDepth, int *depth, int *planes);
private:
	IplImage *srcIplImage, *midIplImage, *dstIplImage;
};

}

#endif /* SYNCVIDEOTRANSCODER_H_ */
