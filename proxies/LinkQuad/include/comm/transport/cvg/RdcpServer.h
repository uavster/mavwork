/*
 * RdcpServer.h
 *
 *  Created on: 04/05/2012
 *      Author: Ignacio Mellado-Bataller
 */

#ifndef RDCPSERVER_H_
#define RDCPSERVER_H_

#include "../IServer.h"
#include <map>
#include "RdcpSocket.h"

namespace Comm {

template<class _UnconnDatTransportT, class _PortAddressT> class RdcpServer :
	public virtual IServer, public virtual cvgThread, public virtual EventListener {
private:
	cvg_int clientQueueLength;
	_UnconnDatTransportT socket;
	cvg_bool isOpen;

	typedef std::map<_PortAddressT, cvg_long> PendingMap;
	typedef std::map<_PortAddressT, RdcpSocket<_UnconnDatTransportT, _PortAddressT> *> SocketMap;

	PendingMap pendingAccepts;
	SocketMap acceptedSockets;

	cvgMutex mutex;
	cvgCondition acceptSemaphore;
	_PortAddressT listenAddress;

	DynamicBuffer<char> defaultSendPayload;
	cvg_double sendBeaconPeriod, recvBeaconPeriod;

/*	class SocketEraser : public virtual cvgThread {
	protected:
		void run();
		void deleteSockets();
	private:
		cvgMutex mutex;
		SocketMap toBeDeleted;
		RdcpServer<_UnconnDatTransportT, _PortAddressT> *server;
	public:
		SocketEraser() : cvgThread("SocketEraser") {}
		void markSocketToDie(RdcpSocket<_UnconnDatTransportT, _PortAddressT> *s);
		void stop();
		inline void setServer(RdcpServer<_UnconnDatTransportT, _PortAddressT> *s) { server = s; }
	};
	SocketEraser socketEraser;
*/

protected:
	virtual void run();
	virtual void gotEvent(EventGenerator &source, const Event &e);

public:
	RdcpServer();
	virtual ~RdcpServer();

	void setNewClientBeaconPeriods(cvg_double recvBeaconPeriod, cvg_double sendBeaconPeriod);
	void setNewClientDefaultSendPayload(const Buffer &defSendPayload);

	// IServer interface
	virtual void open(const IAddress &address, cvg_int clientQueueLength = -1);
	virtual void close();
	virtual RdcpSocket<_UnconnDatTransportT, _PortAddressT> *accept(cvg_double timeout = -1.0);
};

}

#include "../../../../sources/RdcpServer.hh"

#endif /* RDCPSERVER_H_ */
