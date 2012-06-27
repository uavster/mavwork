/*
 * DycsClient.hh
 *
 *  Created on: 17/05/2012
 *      Author: Ignacio Mellado-Bataller
 */
#include <iostream>
#define template_def template<class _TransportServerT, class _TransportSocketT>
#define DycsServerConn_ DycsServerConn<_TransportServerT, _TransportSocketT>

namespace Comm {

template_def DycsServerConn_::DycsServerConn(_TransportSocketT *socket, DycsServer<_TransportServerT, _TransportSocketT> *server) {
	this->socket = socket;
	this->server = server;
	requestedAccessMode = Proto::Dycs::UNKNOWN_MODE;
	requestedPriority = 0;
	socket->addEventListener(this);
}

template_def DycsServerConn_::~DycsServerConn() {
	socket->removeEventListener(this);
}

template_def void DycsServerConn_::gotEvent(EventGenerator &source, const Event &e) {
	notifyEvent(e);
}

template_def cvg_ulong DycsServerConn_::readOrPeek(cvg_bool read_peek, void *inBuffer, cvg_ulong lengthBytes, cvg_ulong offsetBytes) {
	char auxBuffer[lengthBytes + sizeof(Proto::Dycs::DycsClientToServerHeader)];
	cvg_ulong received = read_peek ? socket->read(auxBuffer, sizeof(auxBuffer)) : socket->peek(auxBuffer, sizeof(auxBuffer));
	if (received == sizeof(auxBuffer)) {
		Proto::Dycs::DycsAccessMode requestedAccessMode = (Proto::Dycs::DycsAccessMode)((Proto::Dycs::DycsClientToServerHeader *)auxBuffer)->requestedAccessMode;
		cvg_int requestedPriority = (Proto::Dycs::DycsAccessMode)((Proto::Dycs::DycsClientToServerHeader *)auxBuffer)->requestedPriority;
		this->requestedAccessMode = requestedAccessMode;
		this->requestedPriority =  requestedPriority;
		Proto::Dycs::DycsAccessMode mode = server->notifyAccessModeRequest(this, requestedAccessMode, requestedPriority);
		if (mode != Proto::Dycs::COMMANDER) return 0;
		memcpy(&((char *)inBuffer)[offsetBytes], &auxBuffer[sizeof(Proto::Dycs::DycsClientToServerHeader)], lengthBytes);
		return lengthBytes;
	} else if (received > sizeof(Proto::Dycs::DycsClientToServerHeader) &&
				received < sizeof(auxBuffer)) {
		return received - sizeof(Proto::Dycs::DycsClientToServerHeader);
	} else return 0;
}

template_def cvg_ulong DycsServerConn_::read(void *inBuffer, cvg_ulong lengthBytes, cvg_ulong offsetBytes) {
	return readOrPeek(true, inBuffer, lengthBytes, offsetBytes);
}

template_def cvg_ulong DycsServerConn_::peek(void *inBuffer, cvg_ulong lengthBytes, cvg_ulong offsetBytes) {
	return readOrPeek(false, inBuffer, lengthBytes, offsetBytes);
}

template_def cvg_ulong DycsServerConn_::write(const void *outBuffer, cvg_ulong lengthBytes, cvg_ulong offsetBytes) {
	char auxBuffer[lengthBytes + sizeof(Proto::Dycs::DycsServertToClientHeader)];
	((Proto::Dycs::DycsServertToClientHeader *)auxBuffer)->grantedAccessMode = server->isCommander(this) ? Proto::Dycs::COMMANDER : Proto::Dycs::LISTENER;
	memcpy(&auxBuffer[sizeof(Proto::Dycs::DycsServertToClientHeader)], &((char *)outBuffer)[offsetBytes], lengthBytes);
	cvg_ulong written = socket->write(auxBuffer, sizeof(auxBuffer));
	if (written == sizeof(auxBuffer)) return lengthBytes;
	else if (written > sizeof(Proto::Dycs::DycsServertToClientHeader)) return written - sizeof(Proto::Dycs::DycsServertToClientHeader);
	else return 0;
}

template_def void DycsServerConn_::connect(const IAddress &addr) {
	throw cvgException("[DycsServerConn] This is a server socket and cannot be manually connected with connect()");
}

template_def void DycsServerConn_::disconnect() {
	socket->disconnect();
}

template_def void DycsServerConn_::setTimeouts(cvg_double readTimeout, cvg_double writeTimeout) {
	socket->setTimeouts(readTimeout, writeTimeout);
}

template_def void DycsServerConn_::getTimeouts(cvg_double *readTimeout, cvg_double *writeTimeout) {
	socket->getTimeouts(readTimeout, writeTimeout);
}

template_def cvg_bool DycsServerConn_::getLocalAddress(IAddress &addr) {
	return socket->getLocalAddress(addr);
}

template_def cvg_bool DycsServerConn_::getRemoteAddress(IAddress &addr) {
	return socket->getRemoteAddress(addr);
}

template_def IConnectedSocket::ConnectionState DycsServerConn_::getConnectionState() {
	return socket->getConnectionState();
}

}

#undef template_def
#undef DycsServerConn_

