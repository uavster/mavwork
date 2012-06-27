/*
 * TcpSocket.h
 *
 *  Created on: 27/04/2012
 *      Author: Ignacio Mellado-Bataller
 *
 *  Notes: This class is OS-dependent. The current version works for Linux.
 */

#ifndef TCPSOCKET_H_
#define TCPSOCKET_H_

#include "../IConnectedStreamSocket.h"
#include "../../network/inet/InetAddress.h"

namespace Comm {

class TcpServer;

class TcpSocket : public virtual IConnectedStreamSocket {
	friend class TcpServer;
private:
	int descriptor;
	cvg_double readTimeout, writeTimeout;
public:
	TcpSocket();
	virtual ~TcpSocket();

	virtual void connect(const IAddress &addr);
	virtual void disconnect();

	virtual cvg_ulong read(void *inBuffer, cvg_ulong lengthBytes, cvg_ulong offsetBytes = 0);
	virtual cvg_ulong write(const void *outBuffer, cvg_ulong lengthBytes, cvg_ulong offsetBytes = 0);
	virtual cvg_ulong peek(void *inBuffer, cvg_ulong lengthBytes, cvg_ulong offsetBytes = 0);

	virtual void setTimeouts(cvg_double readTimeout, cvg_double writeTimeout);
	virtual void getTimeouts(cvg_double *readTimeout, cvg_double *writeTimeout);

	virtual cvg_bool getLocalAddress(IAddress &addr);
	virtual cvg_bool getRemoteAddress(IAddress &addr);
};

}

#endif /* TCPSOCKET_H_ */
