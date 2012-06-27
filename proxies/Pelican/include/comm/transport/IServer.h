/*
 * IServer.h
 *
 *  Created on: 28/04/2012
 *      Author: Ignacio Mellado-Bataller
 */

#ifndef ISERVER_H_
#define ISERVER_H_

#include "../network/IAddress.h"
#include "IConnectedSocket.h"

namespace Comm {

class IServer {
public:
	virtual void open(const IAddress &address, cvg_int clientQueueLength = -1) = 0;
	virtual void close() = 0;

	virtual IConnectedSocket *accept(cvg_double timeout = -1.0) = 0;
};

}

#endif /* ISERVER_H_ */
