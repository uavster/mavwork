/*
 * TcpServerSocket.h
 *
 *  Created on: 28/04/2012
 *      Author: Ignacio Mellado-Bataller
 */

#ifndef TCPSERVERSOCKET_H_
#define TCPSERVERSOCKET_H_

#include "../IServer.h"
#include "../../network/inet/InetAddress.h"
#include "TcpSocket.h"

namespace Comm {

class TcpServer : public virtual IServer {
private:
	int descriptor;
public:
	TcpServer();

	virtual void open(const IAddress &address, cvg_int maxQueueLength = 20);
	virtual void close();

	virtual TcpSocket *accept(cvg_double timeout = -1.0);
};

}

#endif /* TCPSERVERSOCKET_H_ */
