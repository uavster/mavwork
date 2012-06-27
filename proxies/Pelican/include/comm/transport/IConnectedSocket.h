/*
 * IConnectedSocket.h
 *
 *  Created on: 29/04/2012
 *      Author: Ignacio Mellado-Bataller
 */

#ifndef ICONNECTEDSOCKET_H_
#define ICONNECTEDSOCKET_H_

#include "../network/IAddress.h"
#include "ISocket.h"
#include "../../base/EventGenerator.h"

namespace Comm {

class IConnectedSocket : public virtual ISocket, public virtual EventGenerator {
public:
	enum ConnectionState { DISCONNECTED, CONNECTING, CONNECTED, DISCONNECTING, FAULT, UNKNOWN };

private:
	ConnectionState connState;
	cvgMutex stateMutex;

public:
	virtual void connect(const IAddress &addr) = 0;
	virtual void disconnect() = 0;

	virtual void setConnectionState(ConnectionState s);
	virtual ConnectionState getConnectionState();
	static cvgString connectionStateToString(ConnectionState s);

	virtual cvg_ulong read(void *inBuffer, cvg_ulong lengthBytes, cvg_ulong offsetBytes = 0) = 0;
	virtual cvg_ulong write(const void *outBuffer, cvg_ulong lengthBytes, cvg_ulong offsetBytes = 0) = 0;
	virtual cvg_ulong peek(void *inBuffer, cvg_ulong lengthBytes, cvg_ulong offsetBytes = 0) = 0;
	virtual inline cvg_ulong read(IAddress &addr, void *inBuffer, cvg_ulong lengthBytes, cvg_ulong offsetBytes = 0) {
		return read(inBuffer, lengthBytes, offsetBytes);
	}
	virtual inline cvg_ulong write(const IAddress &addr, const void *outBuffer, cvg_ulong lengthBytes, cvg_ulong offsetBytes = 0) {
		return write(outBuffer, lengthBytes, offsetBytes);
	}
	virtual inline cvg_ulong peek(IAddress &addr, void *inBuffer, cvg_ulong lengthBytes, cvg_ulong offsetBytes = 0) {
		return peek(inBuffer, lengthBytes, offsetBytes);
	}

	// Provided auxiliary methods
	inline cvg_ulong read(Buffer &inBuffer) { return read(&inBuffer, inBuffer.sizeBytes(), 0); }
	inline cvg_ulong write(const Buffer &outBuffer) { return write(&outBuffer, outBuffer.sizeBytes(), 0); }
	inline cvg_ulong peek(Buffer &inBuffer) { return peek(&inBuffer, inBuffer.sizeBytes(), 0); }

	// Events that may be notified to listeners
	class ConnectionStateChangedEvent : public virtual Event {
	public:
		IConnectedSocket *socket;
		ConnectionState oldState, newState;

		ConnectionStateChangedEvent(IConnectedSocket *socket, ConnectionState oldState, ConnectionState newState) {
			this->socket = socket;
			this->oldState = oldState;
			this->newState = newState;
		}
	};

};

}

#endif /* ICONNECTEDSOCKET_H_ */
