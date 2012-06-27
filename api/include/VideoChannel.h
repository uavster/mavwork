/*
 * VideoChannel.h
 *
 *  Created on: 03/09/2011
 *      Author: Ignacio Mellado
 */

#ifndef VIDEOCHANNEL_H_
#define VIDEOCHANNEL_H_

#define VIDEO_CHANNEL_MAX_FRAME_ATOMIC_SECONDS 		0.07
#define VIDEO_CHANNEL_MAX_INTERSYNC_SECONDS			1.0
#define VIDEO_CHANNEL_MAX_INTERFRAME_SECONDS		0.1
#define VIDEO_CHANNEL_DEFAULT_LINGER_SECONDS		1
#define VIDEO_CHANNEL_MAX_ALLOWED_FRAME_LENGTH		(20 * 1024 * 1024)

#include <atlante.h>
#include "Channel.h"
#include "Protocol.h"
#include "Mutex.h"
#include "Timer.h"
#include "Mutex.h"
#include "LogProducer.h"

namespace DroneProxy {
namespace Comm {

class VideoChannel : public Channel, public virtual cvgThread, public DroneProxy::Logs::LogProducer {
private:
	int videoSocket;
	cvg_bool gotFirstPacket;

	cvg_int frameBufferLength;
	cvg_char *frameBuffer, *auxFrameBuffer;
	DroneProxy::Timing::Timer lastFrameTime;
	DroneProxy::Threading::Mutex *frameBufferMutex;
	VideoFrameHeader frameInfo;

	DroneProxy::Timing::Timer syncTimer;

protected:
	cvg_bool syncFrame(void *buffer, cvg_uint length);
	cvg_bool readData(void *buffer, cvg_uint length);
	virtual void run();

public:
	VideoChannel();
	virtual ~VideoChannel();

	virtual void open(cvgString host, cvg_int port);
	virtual void close();

	inline DroneProxy::Threading::Mutex *getFrameBufferMutex() { return frameBufferMutex; }
	inline cvg_char *getFrameBuffer() { return frameBuffer; }
	inline VideoFrameHeader *getFrameInfo() { return &frameInfo; }
};

}
}

#endif /* VIDEOCHANNEL_H_ */
