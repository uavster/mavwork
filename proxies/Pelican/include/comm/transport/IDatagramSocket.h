/*
 * IDatagramSocket.h
 *
 *  Created on: 29/04/2012
 *      Author: Ignacio Mellado-Bataller
 */

#ifndef IDATAGRAMSOCKET_H_
#define IDATAGRAMSOCKET_H_

#include "ISocket.h"

namespace Comm {

class IDatagramSocket : public virtual ISocket {
public:
	virtual cvg_ulong getMaxPayloadSize() = 0;
};

}

#endif /* IDATAGRAMSOCKET_H_ */
