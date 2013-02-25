/*
 * Vid1TcpInetServer.h
 *
 *  Created on: 22/05/2012
 *      Author: Ignacio Mellado-Bataller
 */

#ifndef VID1TCPINETSERVER_H_
#define VID1TCPINETSERVER_H_

#include "application/video/VideoServer.h"
#include "transport/inet/TcpServer.h"
#include "application/video/Vid1Protocol.h"
#include <atlante.h>

namespace Comm {

typedef VideoServer<TcpServer, Proto::Vid1::Vid1FrameHeader>	VideoServer_base;

class Vid1TcpInetServer : public virtual VideoServer_base {
private:
	cvgTimer fpsTimer;
	cvgTimer timer;
	Proto::Vid1::Vid1FrameHeader header;
	DynamicBuffer<char> frameBuffer, frameBuffer2;
	cvgMutex frameMutex;
	cvgCondition frameCondition;

	class FrameFeeder : public virtual cvgThread, public virtual EventListener {
	protected:
		virtual void run();
		virtual void gotEvent(EventGenerator &source, const Event &event);
	private:
		Vid1TcpInetServer *parent;
	public:
		inline FrameFeeder() : cvgThread("Vid1TcpInetServer::FrameFeeder") {}
		inline void setParent(Vid1TcpInetServer *p) { parent = p; }
	};
	FrameFeeder frameFeeder;

protected:
	void processEvent(EventGenerator &source, const Event &event);

public:
	Vid1TcpInetServer();

	virtual void open(const IAddress &addr, cvg_int maxQueueLength = -1);
	virtual void close();
};

}

#endif /* VID1TCPINETSERVER_H_ */
