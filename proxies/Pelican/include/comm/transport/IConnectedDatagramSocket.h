/*
 * IConnectedDatagramSocket.h
 *
 *  Created on: 29/04/2012
 *      Author: Ignacio Mellado-Bataller
 */

#ifndef ICONNECTEDDATAGRAMSOCKET_H_
#define ICONNECTEDDATAGRAMSOCKET_H_

#include "IConnectedSocket.h"
#include "IDatagramSocket.h"

namespace Comm {

class IConnectedDatagramSocket : public virtual IConnectedSocket, public virtual IDatagramSocket {
public:
	virtual ~IConnectedDatagramSocket() {}
};

}

#endif /* ICONNECTEDDATAGRAMSOCKET_H_ */
