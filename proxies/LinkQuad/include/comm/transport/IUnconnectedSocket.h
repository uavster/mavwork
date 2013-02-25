/*
 * IUnconnectedSocket.h
 *
 *  Created on: 29/04/2012
 *      Author: Ignacio Mellado-Bataller
 */

#ifndef IUNCONNECTEDSOCKET_H_
#define IUNCONNECTEDSOCKET_H_

#include "../network/IAddress.h"
#include "ISocket.h"

namespace Comm {

class IUnconnectedSocket : public virtual ISocket {
public:
	virtual void setAddresses(cvg_bool recvAddrValid, const IAddress &recv, cvg_bool sendAddrValid, const IAddress &send) = 0;
	virtual void setRecvAddress(cvg_bool recvAddrValid, const IAddress &recv) = 0;
	virtual void setSendAddress(cvg_bool sendAddrValid, const IAddress &send) = 0;
//	virtual void getAddresses(cvg_bool *recvAddrValid, IAddress &recv, cvg_bool *sendAddrValid, IAddress &send) = 0;
};

}

#endif /* IUNCONNECTEDSOCKET_H_ */
