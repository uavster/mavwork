/*
 * RdcpSocket.h
 *
 *  Created on: 04/05/2012
 *      Author: Ignacio Mellado-Bataller
 */

#ifndef RDCPSOCKET_H_
#define RDCPSOCKET_H_

#include "../IConnectedDatagramSocket.h"
#include "RdcpServer.h"

namespace Comm {

template<class _UnconnDatTransportT, class _PortAddressT> class RdcpSocket :
	public virtual IConnectedDatagramSocket
	{

	template<class, class> friend class RdcpServer;

private:
	_UnconnDatTransportT baseSocket;

	class Receiver : public virtual cvgThread {
	private:
		RdcpSocket<_UnconnDatTransportT, _PortAddressT> *socket;
		cvgMutex dataMutex;
		cvgCondition dataCondition;
		DynamicBuffer<char> dataBuffer;
		cvg_double beaconPeriod;
		cvg_bool gotFirstPacket;
		cvg_ulong lastSeqNumber;
		cvg_bool msgAvail, faultState;
		cvg_double readTimeout;
		cvg_int numBadPackets;
	protected:
		virtual void run();
	public:
		Receiver();
		void reset();
		inline void setSocket(RdcpSocket<_UnconnDatTransportT, _PortAddressT> *socket) { this->socket = socket; }
		void setBeaconPeriod(cvg_double period);
		cvg_ulong getPayload(void *buffer, cvg_ulong length, cvg_bool peek);
		inline cvg_ulong getPayload(Buffer &payload, cvg_bool peek);
		void setFaultState();
		void setReadTimeout(cvg_double timeout);
		cvg_double getReadTimeout();
	};

	class Sender : public virtual cvgThread {
	private:
		RdcpSocket<_UnconnDatTransportT, _PortAddressT> *socket;
		cvgMutex dataMutex;
		cvgCondition dataCondition;
		DynamicBuffer<char> dataBuffer, sendBuffer;
		cvg_double beaconPeriod;
		cvg_ulong packetCounter;
		_PortAddressT localRecvAddress, remoteSendAddress;
	protected:
		virtual void run();
	public:
		Sender();
		inline void setSocket(RdcpSocket<_UnconnDatTransportT, _PortAddressT> *socket) { this->socket = socket; }
		inline void setRemoteSendAddress(const _PortAddressT &ra) { remoteSendAddress = ra; }
		inline void setLocalRecvAddress(const _PortAddressT &la) { localRecvAddress = la; }
		inline void setPayload(const Buffer &payload);
		void setPayload(const void *buffer, cvg_ulong length);
		void setBeaconPeriod(cvg_double period);
		inline const _PortAddressT &getRemoteSendAddress() { return remoteSendAddress; }
		inline const _PortAddressT &getLocalRecvAddress() { return localRecvAddress; }
	};

	Sender sender;
	Receiver receiver;

protected:
	void setFaultState();
	void connect(const _PortAddressT &recvAddr, const _PortAddressT &sendAddress, const _PortAddressT &remoteSendAddress, cvg_bool isClient);
	void disconnect_();

private:
	cvg_bool isClient;
	cvg_bool gotDataEventEnabled;

public:
	RdcpSocket();
	RdcpSocket(cvg_double sendBeaconPeriod, cvg_double recvBeaconPeriod, const Buffer &defaultSendPayload);
	inline virtual ~RdcpSocket() {}

	inline void setSenderPayload(const Buffer &payload) { sender.setPayload(payload); }
	inline void setBeaconPeriods(cvg_double recvBeaconPeriod, cvg_double sendBeaconPeriod) {
		sender.setBeaconPeriod(sendBeaconPeriod);
		receiver.setBeaconPeriod(recvBeaconPeriod);
	}

	// IConnectedSocket interface
	virtual void connect(const IAddress &addr);
	virtual void disconnect();

	// ISocket interface
	virtual cvg_ulong read(void *inBuffer, cvg_ulong lengthBytes, cvg_ulong offsetBytes = 0);
	virtual cvg_ulong write(const void *outBuffer, cvg_ulong lengthBytes, cvg_ulong offsetBytes = 0);
	virtual cvg_ulong peek(void *inBuffer, cvg_ulong lengthBytes, cvg_ulong offsetBytes = 0);

	virtual void setTimeouts(cvg_double readTimeout, cvg_double writeTimeout);
	virtual void getTimeouts(cvg_double *readTimeout, cvg_double *writeTimeout);

	virtual cvg_bool getLocalAddress(IAddress &addr);
	virtual cvg_bool getRemoteAddress(IAddress &addr);

	// IDatagramSocket interface
	virtual cvg_ulong getMaxPayloadSize();

	// Provided auxiliary methods
	inline cvg_ulong read(Buffer &inBuffer) { return read(&inBuffer, inBuffer.sizeBytes(), 0); }
	inline cvg_ulong write(const Buffer &outBuffer) { return write(&outBuffer, outBuffer.sizeBytes(), 0); }
	inline cvg_ulong peek(Buffer &inBuffer) { return peek(&inBuffer, inBuffer.sizeBytes(), 0); }
};

}

#include "../../../../sources/RdcpSocket.hh"

#endif /* RDCPSOCKET_H_ */
