/*
 * RdcpServer.hh
 *
 *  Created on: 05/05/2012
 *      Author: Ignacio Mellado-Bataller
 */

#include "comm/transport/cvg/RdcpProtocol.h"
#include "data/Buffer.h"
#include "comm/transport/IConnectedSocket.h"

namespace Comm {

template<class _UnconnDatTransportT, class _PortAddressT> RdcpServer<_UnconnDatTransportT, _PortAddressT>::RdcpServer() : cvgThread("RdcpServer") {
	isOpen = false;
//	socketEraser.setServer(this);
}

template<class _UnconnDatTransportT, class _PortAddressT> RdcpServer<_UnconnDatTransportT, _PortAddressT>::~RdcpServer() {
	close();
}

template<class _UnconnDatTransportT, class _PortAddressT> void RdcpServer<_UnconnDatTransportT, _PortAddressT>::open(const IAddress &address, cvg_int clientQueueLength) {
	if (isOpen) return;
	this->clientQueueLength = clientQueueLength;
	const _PortAddressT *pa = dynamic_cast<const _PortAddressT *>(&address);
	if (pa == NULL) throw cvgException(cvgString("[RdcpServer] Address supplied to open is not of type ") + typeid(_PortAddressT).name());
	listenAddress = *pa;
	socket.setAddresses(true, address, false, DummyAddress(""));
	isOpen = true;
//	socketEraser.start();
	start();
}

template<class _UnconnDatTransportT, class _PortAddressT> void RdcpServer<_UnconnDatTransportT, _PortAddressT>::close() {
	if (!isOpen) return;
	isOpen = false;

	stop();
	// Signal the accept condition to unlock any waiting thread
	if (mutex.lock()) {
		acceptSemaphore.signal();
		mutex.unlock();
	}

//	socketEraser.stop();

	while(acceptedSockets.size() > 0) {
		RdcpSocket<_UnconnDatTransportT, _PortAddressT> *s = acceptedSockets.begin()->second;
		s->removeEventListener(this);
		s->disconnect();
		acceptedSockets.erase(acceptedSockets.begin());
		// The application must delete the socket
//		delete s;
	}
}

template<class _UnconnDatTransportT, class _PortAddressT> RdcpSocket<_UnconnDatTransportT, _PortAddressT> *RdcpServer<_UnconnDatTransportT, _PortAddressT>::accept(cvg_double timeoutSeconds) {
	RdcpSocket<_UnconnDatTransportT, _PortAddressT> *newSocket = NULL;
	// Check if there are any pending connections
	if (!mutex.lock()) throw cvgException("[RdcpServer] Cannot lock resources in accept()");
	try {
		if (!isOpen) throw cvgException("[RdcpServer] accept() was called before opening the server");
		cvg_bool timeout = false;
		if (timeoutSeconds >= 0.0) timeout = !acceptSemaphore.wait(&mutex, timeoutSeconds);
		else acceptSemaphore.wait(&mutex);

		_PortAddressT connectionAddress, otherEndAddress, localAddress;

		if (!timeout && pendingAccepts.size() > 0) {
			// There are pending connections
			connectionAddress = pendingAccepts.begin()->first;
			cvg_long remotePort = pendingAccepts.begin()->second;
			otherEndAddress = connectionAddress;
			otherEndAddress.setPort(remotePort);
			localAddress = listenAddress;
			localAddress.setPort(localAddress.anyPort());

			newSocket = new RdcpSocket<_UnconnDatTransportT, _PortAddressT>;
			pendingAccepts.erase(pendingAccepts.begin());
			acceptedSockets[connectionAddress] = newSocket;
		}
		mutex.unlock();

		if (newSocket != NULL) {
			newSocket->setBeaconPeriods(recvBeaconPeriod, sendBeaconPeriod);
			newSocket->setSenderPayload(defaultSendPayload);
			newSocket->addEventListener(this);
			newSocket->connect(localAddress, otherEndAddress, connectionAddress, false);
		}
	} catch (cvgException &e) {
		mutex.unlock();
		throw e;
	}

	return newSocket;
}

template<class _UnconnDatTransportT, class _PortAddressT> void RdcpServer<_UnconnDatTransportT, _PortAddressT>::run() {
	// Check if a new client arrived
	StaticBuffer<Proto::Rdcp::RdcpHeader, 1> header;
	_PortAddressT address;
	cvg_ulong bytes = ((ISocket &)socket).read(address, header);
	if (bytes != header.sizeBytes()) return;

	// Check packet validity
	Proto::Rdcp::RdcpHeader_hton((Proto::Rdcp::RdcpHeader *)&header);
	if (header[0].signature != RDCPHEADER_CLIENT_TO_SERVER_SIGNATURE) return;

	// The packet is valid, check if the client is already known
	if (!mutex.lock()) return;
	if (!(acceptedSockets.find(address) != acceptedSockets.end())) {
		if (!(pendingAccepts.find(address) != pendingAccepts.end()) &&
			(clientQueueLength < 0 || pendingAccepts.size() < clientQueueLength)) {
			// The sender is unknown and pending connections are below the maximum
 			pendingAccepts[address] = header[0].responsePort;
 			acceptSemaphore.signal();
		}
	}
	mutex.unlock();
}

template<class _UnconnDatTransportT, class _PortAddressT> void RdcpServer<_UnconnDatTransportT, _PortAddressT>::setNewClientBeaconPeriods(cvg_double recvBeaconPeriod, cvg_double sendBeaconPeriod) {
	if (!mutex.lock()) throw cvgException("[RdcpServer] Cannot lock mutex in setNewClientBeaconPeriods()");
	this->recvBeaconPeriod = recvBeaconPeriod;
	this->sendBeaconPeriod = sendBeaconPeriod;
	mutex.unlock();
}

template<class _UnconnDatTransportT, class _PortAddressT> void RdcpServer<_UnconnDatTransportT, _PortAddressT>::setNewClientDefaultSendPayload(const Buffer &defSendPayload) {
	if (!mutex.lock()) throw cvgException("[RdcpServer] Cannot lock mutex in setNewClientSendPayloadSize()");
	this->defaultSendPayload = defSendPayload;
	mutex.unlock();
}

template<class _UnconnDatTransportT, class _PortAddressT> void RdcpServer<_UnconnDatTransportT, _PortAddressT>::gotEvent(EventGenerator &source, const Event &e) {
	const IConnectedSocket::ConnectionStateChangedEvent *sce = dynamic_cast<const IConnectedSocket::ConnectionStateChangedEvent *>(&e);
	if (sce == NULL) return;
	if (!mutex.lock()) throw cvgException("[RdcpServer] Cannot lock resources in accept()");
	try {
		if (sce->newState != sce->oldState &&
			(sce->newState == IConnectedSocket::DISCONNECTED || sce->newState == IConnectedSocket::FAULT)
			) {
			// If a socket changes to a DISCONNECTED or FAULT state, remove it from the accepted socket list
			RdcpSocket<_UnconnDatTransportT, _PortAddressT> *rdcpSocket = dynamic_cast<RdcpSocket<_UnconnDatTransportT, _PortAddressT> *>(sce->socket);
			if (rdcpSocket != NULL)
				acceptedSockets.erase(rdcpSocket->sender.getRemoteSendAddress());

//			socketEraser.markSocketToDie(rdcpSocket);
		}
	} catch(cvgException &e) {
	} catch(std::exception &e) {
	}
	mutex.unlock();
}

/*template<class _UnconnDatTransportT, class _PortAddressT> void RdcpServer<_UnconnDatTransportT, _PortAddressT>::SocketEraser::markSocketToDie(RdcpSocket<_UnconnDatTransportT, _PortAddressT> *s) {
	if (mutex.lock()) {
		if (!(toBeDeleted.find(s->getId()) != toBeDeleted.end())) {
			toBeDeleted[s->getId()] = s;
		}
		mutex.unlock();
	}
}

template<class _UnconnDatTransportT, class _PortAddressT> void RdcpServer<_UnconnDatTransportT, _PortAddressT>::SocketEraser::deleteSockets() {
	if (mutex.lock()) {
		while(toBeDeleted.size() > 0) {
			RdcpSocket<_UnconnDatTransportT, _PortAddressT> *s = toBeDeleted.begin()->second;
			s->removeEventListener(server);
			s->disconnect();
			toBeDeleted.erase(toBeDeleted.begin());
			delete s;
		}
		mutex.unlock();
	}
}

template<class _UnconnDatTransportT, class _PortAddressT> void RdcpServer<_UnconnDatTransportT, _PortAddressT>::SocketEraser::run() {
	usleep(100000);
	deleteSockets();
}

template<class _UnconnDatTransportT, class _PortAddressT> void RdcpServer<_UnconnDatTransportT, _PortAddressT>::SocketEraser::stop() {
	cvgThread::stop();
	deleteSockets();
}
*/
}
