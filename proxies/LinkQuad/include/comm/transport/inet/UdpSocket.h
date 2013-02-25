/*
 * UdpClientSocket.h
 *
 *  Created on: 28/04/2012
 *      Author: Ignacio Mellado-Bataller
 */

#ifndef UDPSOCKET_H_
#define UDPSOCKET_H_

#include "../IUnconnectedDatagramSocket.h"
#include "../../network/inet/InetAddress.h"

namespace Comm {

class UdpSocket : public virtual IUnconnectedDatagramSocket {
private:
	volatile int descriptor;
	InetAddress outAddress;
	cvg_ulong maxPayload;
	volatile cvg_bool enableSend, enableRecv;
	cvg_double readTimeout, writeTimeout;
	cvgMutex mutex;

	void lock();
	void unlock();

protected:
	void create();
	void destroy();

	cvg_ulong readFlags(void *inBuffer, cvg_ulong lengthBytes, cvg_ulong offsetBytes, cvg_int flags);
	cvg_ulong readFlags(IAddress &addr, void *inBuffer, cvg_ulong lengthBytes, cvg_ulong offsetBytes, cvg_int flags);

public:
	UdpSocket();
	virtual ~UdpSocket();

	virtual void setAddresses(cvg_bool recvAddrValid, const IAddress &recv, cvg_bool sendAddrValid, const IAddress &send);
	virtual void setRecvAddress(cvg_bool recvAddrValid, const IAddress &recv);
	virtual void setSendAddress(cvg_bool sendAddrValid, const IAddress &send);
	virtual cvg_ulong getMaxPayloadSize();

	virtual cvg_ulong read(void *inBuffer, cvg_ulong lengthBytes, cvg_ulong offsetBytes = 0);
	virtual cvg_ulong write(const void *outBuffer, cvg_ulong lengthBytes, cvg_ulong offsetBytes = 0);
	virtual cvg_ulong peek(void *inBuffer, cvg_ulong lengthBytes, cvg_ulong offsetBytes = 0);

	virtual cvg_ulong read(IAddress &addr, void *inBuffer, cvg_ulong lengthBytes, cvg_ulong offsetBytes = 0);
	virtual cvg_ulong write(const IAddress &addr, const void *outBuffer, cvg_ulong lengthBytes, cvg_ulong offsetBytes = 0);
	virtual cvg_ulong peek(IAddress &addr, void *inBuffer, cvg_ulong lengthBytes, cvg_ulong offsetBytes = 0);

	virtual void setTimeouts(cvg_double readTimeout, cvg_double writeTimeout);
	virtual void getTimeouts(cvg_double *readTimeout, cvg_double *writeTimeout);

	virtual void getBuffers(cvg_int *readBuffLen, cvg_int *writeBuffLen);
	virtual void setBuffers(cvg_int readBuffLen, cvg_int writeBuffLen);

	virtual cvg_bool getLocalAddress(IAddress &addr);
	virtual cvg_bool getRemoteAddress(IAddress &addr);

	// Provided auxiliary methods
	inline cvg_ulong read(IAddress &addr, Buffer &inBuffer) { return read(addr, &inBuffer, inBuffer.sizeBytes(), 0); }
	inline cvg_ulong write(const IAddress &addr, const Buffer &outBuffer) { return write(addr, &outBuffer, outBuffer.sizeBytes(), 0); }
	inline cvg_ulong peek(IAddress &addr, Buffer &inBuffer) { return peek(addr, &inBuffer, inBuffer.sizeBytes(), 0); }
	inline cvg_ulong read(Buffer &inBuffer) { return read(&inBuffer, inBuffer.sizeBytes(), 0); }
	inline cvg_ulong write(const Buffer &outBuffer) { return write(&outBuffer, outBuffer.sizeBytes(), 0); }
	inline cvg_ulong peek(Buffer &inBuffer) { return peek(&inBuffer, inBuffer.sizeBytes(), 0); }
};

}

#endif /* UDPSOCKET_H_ */
