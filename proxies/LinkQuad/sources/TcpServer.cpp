/*
 * TcpServer.cpp
 *
 *  Created on: 22/05/2012
 *      Author: Ignacio Mellado-Bataller
 */

#include "comm/transport/inet/TcpServer.h"
#include <math.h>

#define ACCEPT_TIMEOUT	0.2

namespace Comm {

TcpServer::TcpServer() {
	descriptor = -1;
}

void TcpServer::open(const IAddress &address, cvg_int maxQueueLength) {
	const InetAddress *iaddr = dynamic_cast<const InetAddress *>(&address);
	if (iaddr == NULL) throw cvgException("[TcpSocket] The address passed to connect() is not of type InetAddress");

	try {
		descriptor = socket(AF_INET, SOCK_STREAM, 0);
		if (descriptor == -1)
			throw cvgException("[TcpServer] Unable to create socket descriptor");

		// Try reusing the address
		int s = 1;
		if (setsockopt(descriptor, SOL_SOCKET, SO_REUSEADDR, &s, sizeof(int)) != 0)
			throw cvgException("[TcpServer] Error enabling address reuse");

		// Bind to address and port
	    if (bind(descriptor, (struct sockaddr *)iaddr->toAddr(), sizeof(struct sockaddr_in)) < 0)
	    	throw cvgException(cvgString("[TcpServer] Cannot bind socket to ") + iaddr->toString());

	    if (listen(descriptor, maxQueueLength) != 0)
	    	throw cvgException("[TcpServer] Cannot listen()");

		// Set server socket timeout
		struct timeval tm;
		tm.tv_sec = floor(ACCEPT_TIMEOUT);
		tm.tv_usec = (long)(ACCEPT_TIMEOUT * 1e6 - tm.tv_sec * 1e6);
		if (setsockopt(descriptor, SOL_SOCKET, SO_RCVTIMEO, &tm, sizeof(tm)) != 0)
			throw cvgException("[TcpServer] Cannot set server socket timeout");

	} catch (cvgException &e) {
		close();
	}
}

void TcpServer::close() {
	if (descriptor >= 0) {
		::close(descriptor);
		descriptor = -1;
	}
}

TcpSocket *TcpServer::accept(cvg_double timeout) {
	TcpSocket *newSocket = NULL;
	struct sockaddr_in addr;
	socklen_t lenAddr = sizeof(struct sockaddr_in);
	int newDesc = ::accept(descriptor, (struct sockaddr *)&addr, &lenAddr);
	if (newDesc >= 0) {
		newSocket = new TcpSocket();
		newSocket->descriptor = newDesc;
		newSocket->setConnectionState(IConnectedSocket::CONNECTED);
	}
	return newSocket;
}

}
