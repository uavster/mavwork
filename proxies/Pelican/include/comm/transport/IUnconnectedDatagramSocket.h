/*
 * IUnconnectedDatagramSocket.h
 *
 *  Created on: 29/04/2012
 *      Author: Ignacio Mellado-Bataller
 */

#ifndef IUNCONNECTEDDATAGRAMSOCKET_H_
#define IUNCONNECTEDDATAGRAMSOCKET_H_

#include "IUnconnectedSocket.h"
#include "IDatagramSocket.h"

namespace Comm {

class IUnconnectedDatagramSocket : public virtual IUnconnectedSocket, public virtual IDatagramSocket {
};

}

#endif /* IUNCONNECTEDDATAGRAMSOCKET_H_ */
