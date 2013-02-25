/*
 * TcpSocket.cpp
 *
 *  Created on: 27/04/2012
 *      Author: Ignacio Mellado-Bataller
 */

#include <iostream>
#include "comm/transport/inet/TcpSocket.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/tcp.h>

#define TCPSOCKET_DEFAULT_LINGER_SECONDS			1
#define TCPSOCKET_DEFAULT_MAX_READWRITE_SECONDS		1

namespace Comm {

TcpSocket::TcpSocket() {
	descriptor = -1;
	setConnectionState(DISCONNECTED);
	readTimeout = writeTimeout = TCPSOCKET_DEFAULT_MAX_READWRITE_SECONDS;
}

TcpSocket::~TcpSocket() {
	disconnect();
}

void TcpSocket::connect(const IAddress &addr) {
	const InetAddress *iaddr = dynamic_cast<const InetAddress *>(&addr);
	if (iaddr == NULL) throw cvgException("[TcpSocket] The address passed to connect() is not of type InetAddress");

	disconnect();
	setConnectionState(CONNECTING);

	try {
		descriptor = socket(AF_INET, SOCK_STREAM, 0);
		if (descriptor == -1)
			throw cvgException("[TcpSocket] Unable to create socket descriptor");

		// Set linger time: time before the socket is reset on a connection termination
		linger lingerData;
		lingerData.l_onoff = 1;
		lingerData.l_linger = CVG_LITERAL_INT(TCPSOCKET_DEFAULT_LINGER_SECONDS);
		setsockopt(descriptor, SOL_SOCKET, SO_LINGER, &lingerData, sizeof(linger));

		// Set socket timeouts
		setTimeouts(readTimeout, writeTimeout);

		// Disable Nagle's algorithm
		int flag = 1;
		if (setsockopt(descriptor, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(int)) == -1)
			throw cvgException("[TcpSocket] Cannot disable Nagle algorithm");

/*		int sockBuffSize = 3 * 8000; //1e6 * 0.4 / 8;
		if (setsockopt(descriptor, SOL_SOCKET, SO_SNDBUF, &sockBuffSize, sizeof(int)) == -1)
			throw cvgException("[TcpSocket] Cannot set TCP send window size");
		if (setsockopt(descriptor, SOL_SOCKET, SO_RCVBUF, &sockBuffSize, sizeof(int)) == -1)
			throw cvgException("[TcpSocket] Cannot set TCP reception window size");
*/
		// Connect to server
		if (::connect(descriptor, (sockaddr *)iaddr->toAddr(), sizeof(sockaddr_in)) != 0)
			throw cvgException("[TcpSocket] Cannot connect to server");

		setConnectionState(CONNECTED);

	} catch (cvgException &e) {
		disconnect();
		setConnectionState(FAULT);
		throw e;
	}
}

void TcpSocket::disconnect() {
	setConnectionState(DISCONNECTING);
	if (descriptor >= 0) {
		shutdown(descriptor, SHUT_RDWR);
		close(descriptor);
		descriptor = -1;
	}
	setConnectionState(DISCONNECTED);
}

cvg_ulong TcpSocket::read(void *inBuffer, cvg_ulong lengthBytes, cvg_ulong offsetBytes) {
	ssize_t bytes = ::recv(descriptor, &((char *)inBuffer)[offsetBytes], lengthBytes, MSG_NOSIGNAL);
	if (bytes <= 0) { setConnectionState(FAULT); return 0; }
	else return bytes;
}

cvg_ulong TcpSocket::write(const void *outBuffer, cvg_ulong lengthBytes, cvg_ulong offsetBytes) {
	ssize_t bytes = ::send(descriptor, &((char *)outBuffer)[offsetBytes], lengthBytes, MSG_NOSIGNAL);
	if (bytes <= 0) { setConnectionState(FAULT); return 0; }
	else return bytes;
}

cvg_ulong TcpSocket::peek(void *inBuffer, cvg_ulong lengthBytes, cvg_ulong offsetBytes) {
	ssize_t bytes = ::recv(descriptor, &((char *)inBuffer)[offsetBytes], lengthBytes, MSG_PEEK | MSG_NOSIGNAL);
	if (bytes <= 0) { setConnectionState(FAULT); return 0; }
	else return bytes;
}

void TcpSocket::setTimeouts(cvg_double readTimeout, cvg_double writeTimeout) {
	timeval sockReadTimeout;
	sockReadTimeout.tv_sec = floor(readTimeout);
	sockReadTimeout.tv_usec = (long)(readTimeout * 1e6 - sockReadTimeout.tv_sec * 1e6);
	timeval sockWriteTimeout;
	sockWriteTimeout.tv_sec = floor(writeTimeout);
	sockWriteTimeout.tv_usec = (long)(writeTimeout * 1e6 - sockWriteTimeout.tv_sec * 1e6);

	if (setsockopt(descriptor, SOL_SOCKET, SO_RCVTIMEO, &sockReadTimeout, sizeof(timeval)) != 0)
		throw cvgException("[TcpSocket] Error setting read timeout");
	this->readTimeout = readTimeout;

	if (setsockopt(descriptor, SOL_SOCKET, SO_SNDTIMEO, &sockWriteTimeout, sizeof(timeval)) != 0)
		throw cvgException("[TcpSocket] Error setting write timeout");
	this->writeTimeout = writeTimeout;
}

void TcpSocket::getTimeouts(cvg_double *readTimeout, cvg_double *writeTimeout) {
	(*readTimeout) = this->readTimeout;
	(*writeTimeout) = this->writeTimeout;
}

cvg_bool TcpSocket::getLocalAddress(IAddress &addr) {
	InetAddress *iaddr = dynamic_cast<InetAddress *>(&addr);
	if (iaddr == NULL)
		throw cvgException("[TcpSocket] The address provided to getLocalAddress() is not of type InetAddress");

	struct sockaddr_in addrIn;
	socklen_t addrLen = sizeof(struct sockaddr_in);
	if (getsockname(descriptor, (struct sockaddr *)&addrIn, &addrLen) != 0)
		throw cvgException("[TcpSocket] Unable to get local address");
	iaddr->fromAddr(addrIn);

	return true;
}

cvg_bool TcpSocket::getRemoteAddress(IAddress &addr) {
	InetAddress *iaddr = dynamic_cast<InetAddress *>(&addr);
	if (iaddr == NULL)
		throw cvgException("[TcpSocket] The address provided to getRemoteAddress() is not of type InetAddress");

	struct sockaddr_in addrIn;
	socklen_t addrLen = sizeof(struct sockaddr_in);
	if (getpeername(descriptor, (struct sockaddr *)&addrIn, &addrLen) != 0)
		throw cvgException("[TcpSocket] Unable to get remote address");
	iaddr->fromAddr(addrIn);

}

void TcpSocket::getBuffers(cvg_int *readBuffLen, cvg_int *writeBuffLen) {
	int buffSize;
	socklen_t length = sizeof(int);
	if (getsockopt(descriptor, SOL_SOCKET, SO_RCVBUF, &buffSize, &length) != 0)
		throw cvgException("[UdpSocket] Unable to get receive buffer size");
	*readBuffLen = buffSize;

	length = sizeof(int);
	if (getsockopt(descriptor, SOL_SOCKET, SO_SNDBUF, &buffSize, &length) != 0)
		throw cvgException("[UdpSocket] Unable to get send buffer size");
	*writeBuffLen = buffSize;
}

void TcpSocket::setBuffers(cvg_int readBuffLen, cvg_int writeBuffLen) {
	int buffSize = readBuffLen;
	socklen_t length = sizeof(int);
	if (setsockopt(descriptor, SOL_SOCKET, SO_RCVBUF, &buffSize, sizeof(int)) != 0)
		throw cvgException("[UdpSocket] Unable to set receive buffer size");

	buffSize = writeBuffLen;
	length = sizeof(int);
	if (setsockopt(descriptor, SOL_SOCKET, SO_SNDBUF, &buffSize, sizeof(int)) != 0)
		throw cvgException("[UdpSocket] Unable to set send buffer size");
}

}
