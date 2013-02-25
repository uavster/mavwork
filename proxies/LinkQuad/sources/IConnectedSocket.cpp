/*
 * IConnectedSocket.cpp
 *
 *  Created on: 30/04/2012
 *      Author: Ignacio Mellado-Bataller
 */

#include "comm/transport/IConnectedSocket.h"
#include <atlante.h>

namespace Comm {

cvgString IConnectedSocket::connectionStateToString(ConnectionState s) {
	cvgString str;
	switch(s) {
	case DISCONNECTED: str = "DISCONNECTED"; break;
	case CONNECTING: str = "CONNECTING"; break;
	case CONNECTED: str = "CONNECTED"; break;
	case DISCONNECTING: str = "DISCONNECTING"; break;
	case FAULT: str = "FAULT"; break;
	default:
	case UNKNOWN: str = "UNKNOWN"; break;
	}
	return str;
}

void IConnectedSocket::setConnectionState(ConnectionState newState) {
	if (!stateMutex.lock()) throw cvgException("[IConnectedSocket] Cannot lock state mutex in setConnectionState()");
	if (connState != newState) {
		if (!((newState == CONNECTED && connState != CONNECTING) ||
			  (newState == DISCONNECTED && connState != DISCONNECTING)
			 )
			) {
			ConnectionState oldState = connState;
			connState = newState;
			notifyEvent(ConnectionStateChangedEvent(this, oldState, newState));
		}
	}
	stateMutex.unlock();
}

IConnectedSocket::ConnectionState IConnectedSocket::getConnectionState() {
	if (!stateMutex.lock()) throw cvgException("[IConnectedSocket] Cannot lock state mutex in getConnectionState()");
	ConnectionState cs = connState;
	stateMutex.unlock();
	return cs;
}

}
