/*
 * IUnconnectedStreamSocket.h
 *
 *  Created on: 29/04/2012
 *      Author: Ignacio Mellado-Bataller
 */

#ifndef IUNCONNECTEDSTREAMSOCKET_H_
#define IUNCONNECTEDSTREAMSOCKET_H_

#include "IUnconnectedSocket.h"
#include "IStreamSocket.h"

namespace Comm {

class IUnconnectedStreamSocket : public virtual IUnconnectedSocket, public virtual IStreamSocket {
};

}

#endif /* IUNCONNECTEDSTREAMSOCKET_H_ */
