/*
 * UdpSocket.cpp
 *
 *  Created on: 27/04/2012
 *      Author: Ignacio Mellado-Bataller
 */

#include <iostream>
#include "comm/transport/inet/UdpSocket.h"
#include <math.h>

// Min. required IP packet handled by hosts - max IPv4 header - UDP header
#define UDPSOCKET_DEFAULT_MAX_PAYLOAD				(576 - 60 - 8)
#define UDPSOCKET_DEFAULT_MAX_READWRITE_SECONDS		1.0

namespace Comm {

UdpSocket::UdpSocket() {
	maxPayload = UDPSOCKET_DEFAULT_MAX_PAYLOAD;
	descriptor = -1;
	readTimeout = UDPSOCKET_DEFAULT_MAX_READWRITE_SECONDS;
	writeTimeout = UDPSOCKET_DEFAULT_MAX_READWRITE_SECONDS;
}

UdpSocket::~UdpSocket() {
	destroy();
}

void UdpSocket::lock() {
	if (!mutex.lock()) throw cvgException("[UdpSocket] Cannot lock internal mutex");
}

void UdpSocket::unlock() {
	mutex.unlock();
}

void UdpSocket::create() {
	destroy();

	descriptor = socket(AF_INET, SOCK_DGRAM, 0);
	if (descriptor == -1)
		throw cvgException("[UdpServer] Unable to create socket descriptor");

	// Set socket timeouts
	setTimeouts(readTimeout, writeTimeout);
}

void UdpSocket::destroy() {
	if (descriptor >= 0) {
		close(descriptor);
		descriptor = -1;
	}
}

void UdpSocket::setAddresses(cvg_bool recvAddrValid, const IAddress &recv, cvg_bool sendAddrValid, const IAddress &send) {
	lock();

	try {

		enableSend = sendAddrValid;
		enableRecv = recvAddrValid;

		const InetAddress *isend = NULL;
		if (enableSend) {
			isend = dynamic_cast<const InetAddress *>(&send);
			if (isend == NULL) throw cvgException("[UdpSocket] Destination address passed to setAddress() is not of type InetAddress");
		}

		const InetAddress *irecv = NULL;
		if (enableRecv) {
			irecv = dynamic_cast<const InetAddress *>(&recv);
			if (irecv == NULL) throw cvgException("[UdpSocket] Reception address passed to setAddress() is not of type InetAddress");
		}

		destroy();
		if (enableRecv || enableSend)
			create();

		if (enableSend) {
			outAddress = (*isend);
		}
	/*
		// With UDP, connect() only lets receiving packets from the specified address
		struct sockaddr_in sa;
		if (connect(descriptor, (struct sockaddr *)isend->toAddr(sa), sizeof(struct sockaddr_in)) != 0)
			throw cvgException("[UdpSocket] Error connecting socket to " + send.toString());
	*/

		if (enableRecv) {
			if (bind(descriptor, (struct sockaddr *)irecv->toAddr(), sizeof(sockaddr_in)) != 0)
				throw cvgException("[UdpSocket] Error binding socket to " + recv.toString());
		}

	} catch(cvgException &e) {
		unlock();
		throw e;
	}

	unlock();
}

void UdpSocket::setRecvAddress(cvg_bool recvAddrValid, const IAddress &recvAddr) {
	lock();

	try {
		enableRecv = recvAddrValid;
		const InetAddress *irecv = NULL;
		if (enableRecv) {
			irecv = dynamic_cast<const InetAddress *>(&recvAddr);
			if (irecv == NULL) throw cvgException("[UdpSocket] Reception address passed to setRecvAddress() is not of type InetAddress");
		}
		destroy();
		if (enableRecv) {
			create();
			if (bind(descriptor, (struct sockaddr *)irecv->toAddr(), sizeof(sockaddr_in)) != 0)
				throw cvgException("[UdpSocket] Error binding socket to " + recvAddr.toString());
		}
	} catch(cvgException &e) {
		unlock();
		throw e;
	}

	unlock();
}

void UdpSocket::setSendAddress(cvg_bool sendAddrValid, const IAddress &sendAddr) {
	lock();

	try {
		enableSend = sendAddrValid;
		const InetAddress *isend = NULL;
		if (enableSend) {
			isend = dynamic_cast<const InetAddress *>(&sendAddr);
			if (isend == NULL) throw cvgException("[UdpSocket] Destination address passed to setSendAddress() is not of type InetAddress");
		}
		if (descriptor < 0) create();
		if (enableSend) {
			outAddress = (*isend);
		}
	} catch(cvgException &e) {
		unlock();
		throw e;
	}

	unlock();
}

cvg_bool UdpSocket::getLocalAddress(IAddress &addr) {
	lock();
	if (!enableRecv) { unlock(); return false; }
	try {
		InetAddress *iaddr = dynamic_cast<InetAddress *>(&addr);
		if (iaddr == NULL)
			throw cvgException("[UdpSocket] The address provided to getLocalAddress() is not of type InetAddress");

		struct sockaddr_in addrIn;
		socklen_t addrLen = sizeof(struct sockaddr_in);
		if (getsockname(descriptor, (struct sockaddr *)&addrIn, &addrLen) != 0)
			throw cvgException("[UdpSocket] Unable to get local address");
		iaddr->fromAddr(addrIn);

	} catch(cvgException &e) {
		unlock();
		throw e;
	}
	unlock();
	return true;
}

cvg_bool UdpSocket::getRemoteAddress(IAddress &addr) {
	lock();
	if (!enableSend) { unlock(); return false; }
	try {
		InetAddress *iaddr = dynamic_cast<InetAddress *>(&addr);
		if (iaddr == NULL)
			throw cvgException("[UdpSocket] The address provided to getRemoteAddress() is not of type InetAddress");

		(*iaddr) = outAddress;

	} catch(cvgException &e) {
		unlock();
		throw e;
	}
	unlock();
	return true;

}
/*
void UdpSocket::getAddresses(cvg_bool *recvAddrValid, IAddress &recv, cvg_bool *sendAddrValid, IAddress &send) {
	lock();

	try {
		InetAddress *irecv = dynamic_cast<InetAddress *>(&recv);
		if (irecv == NULL)
			throw cvgException("[UdpSocket] The provided reception address is not of type InetAddress");
		InetAddress *isend = dynamic_cast<InetAddress *>(&send);
		if (isend == NULL)
			throw cvgException("[UdpSocket] The provided send address is not of type InetAddress");

		if (enableSend) {
			(*isend) = outAddress;
			(*sendAddrValid) = true;
		}
		else {
			(*sendAddrValid) = false;
		}

		if (enableRecv) {
			struct sockaddr_in addr;
			socklen_t addrLen = sizeof(struct sockaddr_in);
			if (getsockname(descriptor, (struct sockaddr *)&addr, &addrLen) != 0)
				throw cvgException("[UdpSocket] Unable to get reception address");
			irecv->fromAddr(addr);
			(*recvAddrValid) = true;
		} else {
			(*recvAddrValid) = false;
		}
	} catch(cvgException &e) {
		unlock();
		throw e;
	}

	unlock();
}
*/
cvg_ulong UdpSocket::getMaxPayloadSize() {
	lock();
	cvg_ulong res = maxPayload;
	unlock();
	return res;
}

cvg_ulong UdpSocket::readFlags(void *inBuffer, cvg_ulong lengthBytes, cvg_ulong offsetBytes, cvg_int flags) {
	ssize_t bytes;

	if (!enableRecv)
		throw cvgException("[UdpSocket] The socket is not bound to any address. Call setAddresses() first or specify an address in read().");

	bytes = recv(descriptor, &(((char *)inBuffer)[offsetBytes]), lengthBytes, MSG_NOSIGNAL | flags);
	// If a packet larger than the assumed maximum is received, update the assumption
	if (bytes > 0 && bytes > maxPayload) maxPayload = bytes;

	return bytes >= 0 ? (cvg_ulong)bytes : 0;
}

cvg_ulong UdpSocket::read(void *inBuffer, cvg_ulong lengthBytes, cvg_ulong offsetBytes) {
	return readFlags(inBuffer, lengthBytes, offsetBytes, 0);
}

cvg_ulong UdpSocket::peek(void *inBuffer, cvg_ulong lengthBytes, cvg_ulong offsetBytes) {
	return readFlags(inBuffer, lengthBytes, offsetBytes, MSG_PEEK);
}

cvg_ulong UdpSocket::write(const void *outBuffer, cvg_ulong lengthBytes, cvg_ulong offsetBytes) {
	cvg_ulong result;

	lock();
	struct sockaddr_in sa;
	memcpy(&sa, outAddress.toAddr(), sizeof(struct sockaddr_in));
	unlock();

	if (!enableSend)
		throw cvgException("[UdpSocket] The socket has not a default address for write(). Call setAddresses() first or specify an address in write().");

	result = sendto(descriptor, &(((char *)outBuffer)[offsetBytes]), lengthBytes, MSG_NOSIGNAL, (struct sockaddr *)&sa, sizeof(struct sockaddr_in));

	return result;
}

cvg_ulong UdpSocket::readFlags(IAddress &addr, void *inBuffer, cvg_ulong lengthBytes, cvg_ulong offsetBytes, cvg_int flags) {
	ssize_t bytes;

	InetAddress *iaddr = dynamic_cast<InetAddress *>(&addr);
	if (iaddr == NULL) throw cvgException("[UdpSocket] Argument 1 passed to read() is not of type InetAddress");

	if (descriptor < 0) {
		lock();
		create();
		unlock();
	}

	struct sockaddr_in sa;
	bzero(&sa, sizeof(sockaddr_in));
	socklen_t saLen = sizeof(struct sockaddr_in);
	bytes = recvfrom(descriptor, &(((char *)inBuffer)[offsetBytes]), lengthBytes, MSG_NOSIGNAL | flags, (struct sockaddr *)&sa, &saLen);
	if (bytes > 0) {
		iaddr->fromAddr(sa);
		// If a packet larger than the assumed maximum is received, update the assumption
		if (bytes > maxPayload) maxPayload = bytes;
	}

	return bytes >= 0 ? (cvg_ulong)bytes : 0;
}

cvg_ulong UdpSocket::write(const IAddress &addr, const void *outBuffer, cvg_ulong lengthBytes, cvg_ulong offsetBytes) {
	ssize_t bytes;

	const InetAddress *iaddr = dynamic_cast<const InetAddress *>(&addr);
	if (iaddr == NULL) throw cvgException("[UdpSocket] Argument 1 passed to write() is not of type InetAddress");

	if (descriptor < 0) {
		lock();
		create();
		unlock();
	}

	lock();
	struct sockaddr_in sa;
	memcpy(&sa, outAddress.toAddr(), sizeof(struct sockaddr_in));
	unlock();

	bytes = sendto(descriptor, &(((char *)outBuffer)[offsetBytes]), lengthBytes, MSG_NOSIGNAL, (const struct sockaddr *)&sa, sizeof(struct sockaddr_in));

	return bytes >= 0 ? (cvg_ulong)bytes : 0;
}

cvg_ulong UdpSocket::read(IAddress &addr, void *inBuffer, cvg_ulong lengthBytes, cvg_ulong offsetBytes) {
	return readFlags(addr, inBuffer, lengthBytes, offsetBytes, 0);
}

cvg_ulong UdpSocket::peek(IAddress &addr, void *inBuffer, cvg_ulong lengthBytes, cvg_ulong offsetBytes) {
	return readFlags(addr, inBuffer, lengthBytes, offsetBytes, MSG_PEEK);
}

void UdpSocket::setTimeouts(cvg_double readTimeout, cvg_double writeTimeout) {
	lock();

	try {
		if (descriptor < 0) {
			this->readTimeout = readTimeout;
			this->writeTimeout = writeTimeout;
			unlock();
			return;
		}

		timeval sockReadTimeout;
		sockReadTimeout.tv_sec = floor(readTimeout);
		sockReadTimeout.tv_usec = (long)(readTimeout * 1e6 - sockReadTimeout.tv_sec * 1e6);
		timeval sockWriteTimeout;
		sockWriteTimeout.tv_sec = floor(writeTimeout);
		sockWriteTimeout.tv_usec = (long)(writeTimeout * 1e6 - sockWriteTimeout.tv_sec * 1e6);

		if (setsockopt(descriptor, SOL_SOCKET, SO_RCVTIMEO, &sockReadTimeout, sizeof(timeval)) != 0)
			throw cvgException("[UdpSocket] Error setting read timeout");
		this->readTimeout = readTimeout;

		if (setsockopt(descriptor, SOL_SOCKET, SO_SNDTIMEO, &sockWriteTimeout, sizeof(timeval)) != 0)
			throw cvgException("[UdpSocket] Error setting write timeout");
		this->writeTimeout = writeTimeout;
	} catch(cvgException &e) {
		unlock();
		throw e;
	}

	unlock();
}

void UdpSocket::getTimeouts(cvg_double *readTimeout, cvg_double *writeTimeout) {
	lock();

	(*readTimeout) = this->readTimeout;
	(*writeTimeout) = this->writeTimeout;

	unlock();
}

void UdpSocket::getBuffers(cvg_int *readBuffLen, cvg_int *writeBuffLen) {
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

void UdpSocket::setBuffers(cvg_int readBuffLen, cvg_int writeBuffLen) {
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
