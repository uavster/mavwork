/*
 * IConnectedStreamSocket.h
 *
 *  Created on: 29/04/2012
 *      Author: Ignacio Mellado-Bataller
 */

#ifndef ICONNECTEDSTREAMSOCKET_H_
#define ICONNECTEDSTREAMSOCKET_H_

#include "IConnectedSocket.h"
#include "IStreamSocket.h"

namespace Comm {

class IConnectedStreamSocket : public virtual IConnectedSocket, public virtual IStreamSocket {
};

}

#endif /* ICONNECTEDSTREAMSOCKET_H_ */
