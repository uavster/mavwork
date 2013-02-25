/*
 * DycsServer.h
 *
 *  Created on: 15/05/2012
 *      Author: Ignacio Mellado-Bataller
 */

#ifndef DYCSSERVER_H_
#define DYCSSERVER_H_

#include <atlante.h>
#include "DycsServerConn.h"
#include <list>
#include "DycsProtocol.h"
#include "base/EventListener.h"
#include "../../transport/IServer.h"

namespace Comm {

template<class _TransportServerT, class _TransportSocketT> class DycsServer
	: public virtual IServer {
		template<class, class> friend class DycsServerConn;
private:
	_TransportServerT transportServer;
	DycsServerConn<_TransportServerT, _TransportSocketT> *currentCommander;
	cvg_int currentCommanderPriority;

	cvgMutex mutex;
	std::list<DycsServerConn<_TransportServerT, _TransportSocketT> *> clients;

	class SocketEventReceiver : public virtual EventListener {
	private:
		DycsServer<_TransportServerT, _TransportSocketT> *server;
	protected:
		virtual void gotEvent(EventGenerator &source, const Event &e);
	public:
		inline void setParent(DycsServer<_TransportServerT, _TransportSocketT> *server) { this->server = server; }
	};
	SocketEventReceiver socketEventReceiver;

protected:
	inline void lock() { if (!mutex.lock()) throw cvgException("[DycsServer] Cannot lock mutex"); }
	inline void unlock() { mutex.unlock(); }
	Proto::Dycs::DycsAccessMode notifyAccessModeRequest(DycsServerConn<_TransportServerT, _TransportSocketT> *client, Proto::Dycs::DycsAccessMode requestedAccessMode, cvg_int requestedPriority);
	DycsServerConn<_TransportServerT, _TransportSocketT> *findAnotherCommander();
	cvg_bool isCommander(DycsServerConn<_TransportServerT, _TransportSocketT> *c);

public:
	DycsServer();

	void open(const IAddress &address, cvg_int clientQueueLength = -1);
	void close();

	DycsServerConn<_TransportServerT, _TransportSocketT> *accept(cvg_double timeout = -1.0);

	inline _TransportServerT &getTransportServer() { return transportServer; }
};

}

#include "../../../../sources/DycsServer.hh"

#endif /* DYCSSERVER_H_ */
