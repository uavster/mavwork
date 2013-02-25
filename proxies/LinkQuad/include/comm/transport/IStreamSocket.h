/*
 * IStreamSocket.h
 *
 *  Created on: 29/04/2012
 *      Author: Ignacio Mellado-Bataller
 */

#ifndef ISTREAMSOCKET_H_
#define ISTREAMSOCKET_H_

#include "ISocket.h"

namespace Comm {

class IStreamSocket : public virtual ISocket {
public:
	virtual ~IStreamSocket() {}
};

}

#endif /* ISTREAMSOCKET_H_ */
