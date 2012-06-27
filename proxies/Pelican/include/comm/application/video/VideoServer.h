/*
 * VideoServer.h
 *
 *  Created on: 22/05/2012
 *      Author: Ignacio Mellado-Bataller
 */

#ifndef VIDEOSERVER_H_
#define VIDEOSERVER_H_

#include "../ManagedServer.h"
#include <list>

#define VIDEOSERVER_DEFAULT_QUALITY		0.6

namespace Comm {

typedef enum { RAW_BGR24 = 0, RAW_RGB24 = 1, JPEG = 2, RAW_D16 = 3 } VideoFormat;

template<class _ServerT, class _FrameT> class VideoServer
	: public virtual ManagedServer<_ServerT> {

private:
	cvgTimer fpsTimer;
	cvgMutex mutex;
	cvgCondition condition;

	struct Client {
		cvgMutex mutex;
		cvgCondition condition;
//		StaticBuffer<_FrameT, 1> sendHeaderBuffer;
		DynamicBuffer<char> sendFrameBuffer;
	};

	StaticBuffer<_FrameT, 1> headerBuffer;
	cvg_ulong frameLength, maxFrameLength;
	DynamicBuffer<char> frameBuffer;
	DynamicBuffer<char> recodingBuffer, recodingBuffer2;
	cvgMutex bufferMutex;

	cvgMutex clientsMutex;
	std::list<Client *> clients;

	VideoFormat outputFormat;
	cvg_double outputQuality;

	class ManServerListener : public virtual EventListener {
	private:
		VideoServer *parent;
	protected:
		virtual void gotEvent(EventGenerator &source, const Event &e);
	public:
		inline void setParent(VideoServer *p) { parent = p; }
	};
	ManServerListener serverEventHandler;

protected:
	virtual void manageClient(IConnectedSocket *socket);
	void processEvent(EventGenerator &source, const Event &e);

public:
	inline VideoServer() { outputFormat = JPEG; outputQuality = VIDEOSERVER_DEFAULT_QUALITY; serverEventHandler.setParent(this); }
	virtual ~VideoServer() {}

	virtual void open(const IAddress &addr, cvg_int maxQueueLength = -1);
	virtual void close();

	inline void setOutputFormat(VideoFormat format, cvg_double quality = VIDEOSERVER_DEFAULT_QUALITY) { outputFormat = format; outputQuality = quality; }
	inline VideoFormat getOutputFormat() { return outputFormat; }
	void feedFrame(const _FrameT *header, VideoFormat inputFormat, const void *frameData, cvg_int frameDataLength, cvg_int width, cvg_int height, cvg_uint *actualSize);
};

}

#include "../../../../sources/VideoServer.hh"

#endif /* VIDEOSERVER_H_ */
