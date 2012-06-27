/*
 * Socket.h
 *
 *  Created on: 27/04/2012
 *      Author: Ignacio Mellado-Bataller
 */

#ifndef SOCKET_H_
#define SOCKET_H_

#include "../../data/Buffer.h"
#include "../network/IAddress.h"

namespace Comm {

class ISocket {
public:
	inline virtual ~ISocket() {}

	// Implementations must provide this methods
	virtual cvg_ulong read(IAddress &addr, void *inBuffer, cvg_ulong lengthBytes, cvg_ulong offsetBytes = 0) = 0;
	virtual cvg_ulong write(const IAddress &addr, const void *outBuffer, cvg_ulong lengthBytes, cvg_ulong offsetBytes = 0) = 0;
	virtual cvg_ulong peek(IAddress &addr, void *inBuffer, cvg_ulong lengthBytes, cvg_ulong offsetBytes = 0) = 0;
	virtual cvg_ulong read(void *inBuffer, cvg_ulong lengthBytes, cvg_ulong offsetBytes) = 0;
	virtual cvg_ulong write(const void *outBuffer, cvg_ulong lengthBytes, cvg_ulong offsetBytes = 0) = 0;
	virtual cvg_ulong peek(void *inBuffer, cvg_ulong lengthBytes, cvg_ulong offsetBytes) = 0;

	virtual void setTimeouts(cvg_double readTimeout, cvg_double writeTimeout) = 0;
	virtual void getTimeouts(cvg_double *readTimeout, cvg_double *writeTimeout) = 0;

	// These methods return true if the address is valid; false, otherwise
	virtual cvg_bool getLocalAddress(IAddress &addr) = 0;
	virtual cvg_bool getRemoteAddress(IAddress &addr) = 0;

	// Provided auxiliary methods
	inline cvg_ulong read(IAddress &addr, Buffer &inBuffer) { return read(addr, &inBuffer, inBuffer.sizeBytes(), 0); }
	inline cvg_ulong write(const IAddress &addr, const Buffer &outBuffer) { return write(addr, &outBuffer, outBuffer.sizeBytes(), 0); }
	inline cvg_ulong peek(IAddress &addr, Buffer &inBuffer) { return peek(addr, &inBuffer, inBuffer.sizeBytes(), 0); }
	inline cvg_ulong read(Buffer &inBuffer) { return read(&inBuffer, inBuffer.sizeBytes(), 0); }
	inline cvg_ulong write(const Buffer &outBuffer) { return write(&outBuffer, outBuffer.sizeBytes(), 0); }
	inline cvg_ulong peek(Buffer &inBuffer) { return peek(&inBuffer, inBuffer.sizeBytes(), 0); }

	void *userData;
};

}

#endif /* SOCKET_H_ */
