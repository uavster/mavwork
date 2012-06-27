/*
 * VideoProcessor.h
 *
 *  Created on: 04/09/2011
 *      Author: Ignacio Mellado
 */

#ifndef VIDEOPROCESSOR_H_
#define VIDEOPROCESSOR_H_

#include <atlante.h>
#include "Mutex.h"
#include "Condition.h"
#include "ChannelEventListener.h"
#include "Protocol.h"
#include "VideoChannel.h"
#include "LogProducer.h"

namespace Processing {

typedef void (*FrameProcessingCallback)(void *data, cvg_int cameraId, cvg_ulong timeCode, VideoFormat format, cvg_uint width, cvg_uint height, cvg_char *frameData);

class VideoProcessor : public virtual cvgThread, public DroneProxy::Comm::ChannelEventListener, public DroneProxy::Logs::LogProducer  {
private:
	VideoFrameHeader frameInfo;
	cvg_char *frameBuffer;
	cvg_int frameBufferLength;
	cvg_char *decodedBuffer;
	DroneProxy::Threading::Condition bufferCondition;
	DroneProxy::Comm::VideoChannel *videoChannel;
	FrameProcessingCallback frameProcessingCallback;
	void *callbackData;
	cvg_bool finish;
	cvg_int id;

protected:
	virtual void run();
	virtual void gotData(DroneProxy::Comm::Channel *channel, void *data);
	virtual void stateChanged(DroneProxy::Comm::Channel *channel, DroneProxy::Comm::Channel::State oldState, DroneProxy::Comm::Channel::State newState);

public:
	VideoProcessor(cvg_int id);
	virtual ~VideoProcessor();

	void create(DroneProxy::Comm::VideoChannel *vCh);
	void destroy();
	void process();
	inline void setProcessingCallback(FrameProcessingCallback fpc, void *data) { callbackData = data; frameProcessingCallback = fpc; }
};

}

#endif /* VideoProcessor_H_ */
