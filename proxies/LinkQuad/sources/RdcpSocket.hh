/*
 * RdcpSocket.hh
 *
 *  Created on: 07/05/2012
 *      Author: Ignacio Mellado-Bataller
 *
 *  TODO: make the socket thread-safe
 */

#include <stdio.h>
#include "comm/transport/cvg/RdcpProtocol.h"
#include <iostream>
#include <cstdlib>
#include <new>
#include <exception>

namespace Comm {

using namespace Proto::Rdcp;

#define RDCPSOCKET_DEFAULT_BEACON_PERIODS						(1.0/30.0)
#define RDCPSOCKET_DEFAULT_READ_TIMEOUT							1.0

#define RDCPSOCKET_BAD_PACKETS_FOR_FAULT_WHILE_CONNECTED		5
#define RDCPSOCKET_RECV_TIMEOUT									0.05
#define RDCPSOCKET_SILENT_RECVS_FOR_FAULT_WHILE_CONNECTING		40
#define RDCPSOCKET_CONNECTED_SILENT_RECV_BAD_PACKET_COUNT_INC	1

template<class _UnconnDatTransportT, class _PortAddressT> RdcpSocket<_UnconnDatTransportT, _PortAddressT>::RdcpSocket() {
	sender.setSocket(this);
	receiver.setSocket(this);
	sender.setPayload(DynamicBuffer<char>(0));
	setBeaconPeriods(RDCPSOCKET_DEFAULT_BEACON_PERIODS, RDCPSOCKET_DEFAULT_BEACON_PERIODS);
	isClient = true;
	setConnectionState(DISCONNECTED);
}

template<class _UnconnDatTransportT, class _PortAddressT> RdcpSocket<_UnconnDatTransportT, _PortAddressT>::RdcpSocket(cvg_double sendBeaconPeriod, cvg_double recvBeaconPeriod, const Buffer &sendPayload) {
	sender.setSocket(this);
	receiver.setSocket(this);
	setSenderPayload(sendPayload);
	setBeaconPeriods(recvBeaconPeriod, sendBeaconPeriod);
	isClient = true;
	setConnectionState(DISCONNECTED);
}

template<class _UnconnDatTransportT, class _PortAddressT> void RdcpSocket<_UnconnDatTransportT, _PortAddressT>::connect(const _PortAddressT &recvAddr, const _PortAddressT &sendAddr, const _PortAddressT &remoteSendAddr, cvg_bool isClient) {
	this->isClient = isClient;

	baseSocket.setAddresses(true, recvAddr, true, sendAddr);

	baseSocket.setBuffers(1024, 1024);	// Reduce delay

	_PortAddressT recvA;
	cvg_bool recvEnabled = baseSocket.getLocalAddress(recvA);
	if (!recvEnabled) throw cvgException("[RdcpSocket] There was a problem setting addresses in connect()");

	sender.setLocalRecvAddress(recvA);
	sender.setRemoteSendAddress(isClient ? recvA : remoteSendAddr);

	setConnectionState(CONNECTING);

	receiver.reset();
	receiver.start();
	sender.start();

	// Wait until the socket is actually connected or an error occurs
	while(getConnectionState() == CONNECTING) {
		usleep(20000);
	}
}

template<class _UnconnDatTransportT, class _PortAddressT> void RdcpSocket<_UnconnDatTransportT, _PortAddressT>::connect(const IAddress &sendAddr) {
	const _PortAddressT *paddr = dynamic_cast<const _PortAddressT *>(&sendAddr);
	if (paddr == NULL) throw cvgException(cvgString("[RdcpSocket] The argument provided to connect() is not of type ") + typeid(_PortAddressT).name());

	_PortAddressT addr;
	addr.setAnyHost();
	addr.setPort(addr.anyPort());

	connect(addr, *paddr, *paddr, true);

	if (getConnectionState() != CONNECTED)
		throw cvgException("[RdcpSocket] Unable to connect to " + paddr->toString());
}

template<class _UnconnDatTransportT, class _PortAddressT> void RdcpSocket<_UnconnDatTransportT, _PortAddressT>::disconnect_() {
	sender.stop();
	receiver.stop();
	baseSocket.setAddresses(false, DummyAddress(""), false, DummyAddress(""));
}

template<class _UnconnDatTransportT, class _PortAddressT> void RdcpSocket<_UnconnDatTransportT, _PortAddressT>::disconnect() {
	ConnectionState s = getConnectionState();
	if (s == DISCONNECTED || s == DISCONNECTING) return;
	setConnectionState(DISCONNECTING);
	disconnect_();
	setConnectionState(DISCONNECTED);
}

template<class _UnconnDatTransportT, class _PortAddressT> void RdcpSocket<_UnconnDatTransportT, _PortAddressT>::Sender::run() {
	try {
		if (socket->getConnectionState() != CONNECTED && socket->getConnectionState() != CONNECTING) { usleep(10000); return; }
		if (dataMutex.lock()) {
			if (dataCondition.wait(&dataMutex, beaconPeriod))
				sendBuffer = dataBuffer;
			dataMutex.unlock();
		} else usleep(1e6 * beaconPeriod);

		((RdcpHeader *)&sendBuffer)->signature = socket->isClient ? RDCPHEADER_CLIENT_TO_SERVER_SIGNATURE : RDCPHEADER_SERVER_TO_CLIENT_SIGNATURE;
		((RdcpHeader *)&sendBuffer)->indexH = (cvg_uint)((packetCounter >> 32) & 0xFFFFFFFF);
		((RdcpHeader *)&sendBuffer)->indexL = (cvg_uint)(packetCounter & 0xFFFFFFFF);
		((RdcpHeader *)&sendBuffer)->responsePort = remoteSendAddress.getPort();
		RdcpHeader_hton((RdcpHeader *)&sendBuffer);
		socket->baseSocket.write(sendBuffer);
		packetCounter++;
	} catch(cvgException &e) {
		dataMutex.unlock();
		socket->setFaultState();
	} catch(std::exception &e) {
		dataMutex.unlock();
		socket->setFaultState();
	}
}

template<class _UnconnDatTransportT, class _PortAddressT> void RdcpSocket<_UnconnDatTransportT, _PortAddressT>::Receiver::run() {
	try {
		ConnectionState connState = socket->getConnectionState();
		if (connState != CONNECTED && connState != CONNECTING) { usleep(10000); return; }

		StaticBuffer<char, 65535> recvBuffer;
		_PortAddressT recvAddr;
		cvg_ulong bytes = socket->baseSocket.read(recvAddr, recvBuffer);

		cvg_bool error = false;
		cvg_ulong seqNumber;
		if (bytes <= sizeof(RdcpHeader)) error = true;
		else {
			RdcpHeader_ntoh((RdcpHeader *)&recvBuffer);
			cvg_int expectedSignature = socket->isClient ? RDCPHEADER_SERVER_TO_CLIENT_SIGNATURE : RDCPHEADER_CLIENT_TO_SERVER_SIGNATURE;
			cvg_int receivedSignature = ((RdcpHeader *)&recvBuffer)->signature;
			if (socket->isClient) { receivedSignature &= 0x7FFF; expectedSignature &= 0x7FFF; }
			if (receivedSignature != expectedSignature) error = true;
			else {
				seqNumber = ((cvg_ulong)((RdcpHeader *)&recvBuffer)->indexH << 32) | ((cvg_ulong)((RdcpHeader *)&recvBuffer)->indexL);
				if (gotFirstPacket && seqNumber <= lastSeqNumber) return;
			}
		}

		if (error) {
			if (connState == CONNECTED) {
				numBadPackets += bytes == 0 ? RDCPSOCKET_CONNECTED_SILENT_RECV_BAD_PACKET_COUNT_INC : 1;
				if (numBadPackets >= RDCPSOCKET_BAD_PACKETS_FOR_FAULT_WHILE_CONNECTED) {
					socket->setFaultState();
					return;
				}
			} else if (connState == CONNECTING) {
				numBadPackets++;
				if (numBadPackets >= RDCPSOCKET_SILENT_RECVS_FOR_FAULT_WHILE_CONNECTING) {
					socket->setFaultState();
					return;
				}
			}
		} else {
			socket->setConnectionState(CONNECTED);

			// If it's a client socket and it's the first received packet, change destination address
			if (socket->isClient && !gotFirstPacket && !(((RdcpHeader *)&recvBuffer)->signature & 0x8000)) {
				socket->baseSocket.setSendAddress(true, recvAddr);
			}

			numBadPackets = 0;

			lastSeqNumber = seqNumber;
			gotFirstPacket = true;

			if (!dataMutex.lock()) throw cvgException("[RdcpSocket] Cannot lock data mutex");
			dataBuffer.copy(&((char *)&recvBuffer)[sizeof(RdcpHeader)], bytes - sizeof(RdcpHeader));
			msgAvail = true;
			dataCondition.signal();
			dataMutex.unlock();
		}

	} catch(cvgException &e) {
		dataMutex.unlock();
		socket->setFaultState();
	} catch(std::exception &e) {
		dataMutex.unlock();
		socket->setFaultState();
	}
}

template<class _UnconnDatTransportT, class _PortAddressT> void RdcpSocket<_UnconnDatTransportT, _PortAddressT>::Receiver::setBeaconPeriod(cvg_double period) {
	if (!dataMutex.lock())
		throw cvgException("[RdcpSocket::Receiver] Cannot lock parameters");
	beaconPeriod = period;
	cvg_double readTimeout, writeTimeout;
	socket->baseSocket.getTimeouts(&readTimeout, &writeTimeout);
	socket->baseSocket.setTimeouts(RDCPSOCKET_RECV_TIMEOUT, writeTimeout);
	dataMutex.unlock();
}

template<class _UnconnDatTransportT, class _PortAddressT> void RdcpSocket<_UnconnDatTransportT, _PortAddressT>::Sender::setBeaconPeriod(cvg_double period) {
	if (!dataMutex.lock())
		throw cvgException("[RdcpSocket::Sender] Cannot lock parameters");
	beaconPeriod = period;
	dataMutex.unlock();
}

template<class _UnconnDatTransportT, class _PortAddressT> void RdcpSocket<_UnconnDatTransportT, _PortAddressT>::Sender::setPayload(const void *payload, cvg_ulong length) {
	if (!dataMutex.lock()) throw cvgException("[RdcpSocket::Sender] Cannot lock data buffer");
	this->dataBuffer.copy(payload, length, sizeof(RdcpHeader));
	dataCondition.signal();
	dataMutex.unlock();
}

template<class _UnconnDatTransportT, class _PortAddressT> void RdcpSocket<_UnconnDatTransportT, _PortAddressT>::Sender::setPayload(const Buffer &payload) {
	setPayload(&payload, payload.sizeBytes());
}

template<class _UnconnDatTransportT, class _PortAddressT> RdcpSocket<_UnconnDatTransportT, _PortAddressT>::Sender::Sender()
	: cvgThread("RdcpSocket_sender") {
	packetCounter = 0;
}

template<class _UnconnDatTransportT, class _PortAddressT> RdcpSocket<_UnconnDatTransportT, _PortAddressT>::Receiver::Receiver()
	: cvgThread("RdcpSocket_receiver") {
}

template<class _UnconnDatTransportT, class _PortAddressT> cvg_ulong RdcpSocket<_UnconnDatTransportT, _PortAddressT>::getMaxPayloadSize() {
	return baseSocket.getMaxPayloadSize() - sizeof(RdcpHeader);
}

template<class _UnconnDatTransportT, class _PortAddressT> cvg_ulong RdcpSocket<_UnconnDatTransportT, _PortAddressT>::write(const void *outBuffer, cvg_ulong lengthBytes, cvg_ulong offsetBytes) {
	if (getConnectionState() != CONNECTED) return 0;
	cvg_ulong maxPacket = getMaxPayloadSize();
	if (lengthBytes > maxPacket) throw cvgException("[RdcpSocket] The packet to be sent is too big for the current transport layer");
	sender.setPayload(&((char *)outBuffer)[offsetBytes], lengthBytes);
	return lengthBytes;
}

template<class _UnconnDatTransportT, class _PortAddressT> void RdcpSocket<_UnconnDatTransportT, _PortAddressT>::Receiver::reset() {
	gotFirstPacket = false;
	msgAvail = false;
	faultState = false;
	numBadPackets = 0;
	readTimeout = RDCPSOCKET_DEFAULT_READ_TIMEOUT;
}

template<class _UnconnDatTransportT, class _PortAddressT> cvg_ulong RdcpSocket<_UnconnDatTransportT, _PortAddressT>::Receiver::getPayload(void *payload, cvg_ulong lengthBytes, cvg_bool peek) {
	if (!dataMutex.lock()) throw cvgException("[RdcpSocket::Receiver] Cannot lock data buffer");
	cvg_bool timeout;
	do {
		timeout = !dataCondition.wait(&dataMutex, readTimeout);
	} while(!timeout && !faultState && !msgAvail);
	cvg_ulong bytes = 0;
	if (msgAvail && !timeout && !faultState) {
		bytes = lengthBytes <= dataBuffer.sizeBytes() ? lengthBytes : dataBuffer.sizeBytes();
		memcpy(payload, &dataBuffer, bytes);
	}
	if (!peek) msgAvail = false;	// First thread doing a read() consumes the message; peek() does not consume it
	dataMutex.unlock();
	return bytes;
}

template<class _UnconnDatTransportT, class _PortAddressT> cvg_ulong RdcpSocket<_UnconnDatTransportT, _PortAddressT>::Receiver::getPayload(Buffer &payload, cvg_bool peek) {
	return getPayload(&payload, payload.sizeBytes(), peek);
}

template<class _UnconnDatTransportT, class _PortAddressT> cvg_ulong RdcpSocket<_UnconnDatTransportT, _PortAddressT>::read(void *payload, cvg_ulong lengthBytes, cvg_ulong offsetBytes) {
	if (getConnectionState() != CONNECTED) return 0;
	return receiver.getPayload(&((char *)payload)[offsetBytes], lengthBytes, false);
}

template<class _UnconnDatTransportT, class _PortAddressT> cvg_ulong RdcpSocket<_UnconnDatTransportT, _PortAddressT>::peek(void *payload, cvg_ulong lengthBytes, cvg_ulong offsetBytes) {
	if (getConnectionState() != CONNECTED) return 0;
	return receiver.getPayload(&((char *)payload)[offsetBytes], lengthBytes, true);
}

template<class _UnconnDatTransportT, class _PortAddressT> void RdcpSocket<_UnconnDatTransportT, _PortAddressT>::Receiver::setFaultState() {
	// If there are pending reads in other threads, signal them to leave because of the error
	if (dataMutex.lock()) {
		faultState = true;
		dataCondition.signal();
		dataMutex.unlock();
	}
}

template<class _UnconnDatTransportT, class _PortAddressT> void RdcpSocket<_UnconnDatTransportT, _PortAddressT>::setFaultState() {
	setConnectionState(FAULT);
	receiver.setFaultState();
}

template<class _UnconnDatTransportT, class _PortAddressT> void RdcpSocket<_UnconnDatTransportT, _PortAddressT>::Receiver::setReadTimeout(cvg_double timeout) {
	if (!dataMutex.lock()) throw cvgException("[RdcpSocket::Receiver] Cannot lock data mutex in setReadTimeout()");
	readTimeout = timeout;
	dataMutex.unlock();
}

template<class _UnconnDatTransportT, class _PortAddressT> cvg_double RdcpSocket<_UnconnDatTransportT, _PortAddressT>::Receiver::getReadTimeout() {
	if (!dataMutex.lock()) throw cvgException("[RdcpSocket::Receiver] Cannot lock data mutex in getReadTimeout()");
	cvg_double timeout = readTimeout;
	dataMutex.unlock();
	return timeout;
}

template<class _UnconnDatTransportT, class _PortAddressT> void RdcpSocket<_UnconnDatTransportT, _PortAddressT>::setTimeouts(cvg_double readTimeout, cvg_double writeTimeout) {
	receiver.setReadTimeout(readTimeout);
}

template<class _UnconnDatTransportT, class _PortAddressT> void RdcpSocket<_UnconnDatTransportT, _PortAddressT>::getTimeouts(cvg_double *readTimeout, cvg_double *writeTimeout) {
	(*readTimeout) = receiver.getReadTimeout();
	(*writeTimeout) = 0.0;
}

template<class _UnconnDatTransportT, class _PortAddressT> cvg_bool RdcpSocket<_UnconnDatTransportT, _PortAddressT>::getLocalAddress(IAddress &addr) {
	return baseSocket.getLocalAddress(addr);
}

template<class _UnconnDatTransportT, class _PortAddressT> cvg_bool RdcpSocket<_UnconnDatTransportT, _PortAddressT>::getRemoteAddress(IAddress &addr) {
	return baseSocket.getRemoteAddress(addr);
}

template<class _UnconnDatTransportT, class _PortAddressT> void RdcpSocket<_UnconnDatTransportT, _PortAddressT>::getBuffers(cvg_int *readBuffLen, cvg_int *writeBuffLen) {
	baseSocket.getBuffers(readBuffLen, writeBuffLen);
}

template<class _UnconnDatTransportT, class _PortAddressT> void RdcpSocket<_UnconnDatTransportT, _PortAddressT>::setBuffers(cvg_int readBuffLen, cvg_int writeBuffLen) {
	baseSocket.setBuffers(readBuffLen, writeBuffLen);
}

}
