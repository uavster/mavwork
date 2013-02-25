/*
 * AsyncVideoTranscoder.h
 *
 *  Created on: 18/10/2012
 *      Author: Ignacio Mellado-Bataller
 *
 *  Description: Objects of this class receive a video frame, they transcode the frame in the background and
 *  they notify listeners with a GotVideoFrameEvent when the job is done. Others frames that arrive while the
 *  transcoder is busy are droped to minimize the video delay.
 */

#ifndef ASYNCVIDEOTRANSCODER_H_
#define ASYNCVIDEOTRANSCODER_H_

#include <atlante.h>
#include "SyncVideoTranscoder.h"
#include "IVideoSource.h"
#include "data/Buffer.h"
#include "base/EventListener.h"
#include "base/EventGenerator.h"

#define ASYNCVIDEOTRANSCODER_WIDTH_ANY		-1
#define ASYNCVIDEOTRANSCODER_HEIGHT_ANY		-1

namespace Video {

class AsyncVideoTranscoder :
	public virtual cvgThread,
	public virtual EventListener,
	public virtual EventGenerator
{
public:
	inline AsyncVideoTranscoder() : cvgThread("AsyncVideoTranscoder") {
		outputFeaturesSet = false;
	}

	void setOutputFeatures(IVideoSource::VideoFormat dstFormat, cvg_int dstWidth = ASYNCVIDEOTRANSCODER_WIDTH_ANY, cvg_int dstHeight = ASYNCVIDEOTRANSCODER_HEIGHT_ANY);
	void feedFrame(IVideoSource::VideoFormat srcFormat, cvg_int srcWidth, cvg_int srcHeight, const void *srcData, cvg_ulong srcTimeCode, cvg_ulong srcTicksPerSec);
	void feedFrame(const GotVideoFrameEvent &e);

protected:
	virtual void run();
	virtual void gotEvent(EventGenerator &source, const Event &e);

private:
	SyncVideoTranscoder transcoder;
	cvgMutex frameMutex;
	cvgCondition frameCondition;
	DynamicBuffer<char> inputBuffer, outputBuffer, auxBuffer;

	cvg_bool outputFeaturesSet;
	GotVideoFrameEvent dstFeatures, srcFeatures, auxSrcFeatures, auxDstFeatures;
};

}

#endif /* ASYNCVIDEOTRANSCODER_H_ */
