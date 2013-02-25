/*
 * DycsClient.h
 *
 *  Created on: 15/05/2012
 *      Author: Ignacio Mellado-Bataller
 */

#ifndef DYCSCLIENT_H_
#define DYCSCLIENT_H_

#include "DycsProtocol.h"
#include "../../../base/EventGenerator.h"
#include "../../../base/EventListener.h"
#include "../../transport/IConnectedDatagramSocket.h"
#include "DycsProtocol.h"
#include "../../transport/IConnectedSocket.h"

namespace Comm {

template<class _TransportServerT, class _TransportSocketT> class DycsServer;

template<class _TransportServerT, class _TransportSocketT> class DycsServerConn
	: public virtual EventListener, public virtual IConnectedSocket {
		template<class, class> friend class DycsServer;
private:
	_TransportSocketT *socket;
	DycsServer<_TransportServerT, _TransportSocketT> *server;
	Proto::Dycs::DycsAccessMode requestedAccessMode;
	cvg_int requestedPriority;
protected:
	virtual void gotEvent(EventGenerator &source, const Event &e);
	inline Proto::Dycs::DycsAccessMode getRequestedAccessMode(cvg_int *priority) {
		(*priority) = requestedPriority;
		return requestedAccessMode;
	}
	cvg_ulong readOrPeek(cvg_bool read_peek, void *inBuffer, cvg_ulong lengthBytes, cvg_ulong offsetBytes);

public:
	DycsServerConn(_TransportSocketT *socket, DycsServer<_TransportServerT, _TransportSocketT> *server = NULL);
	virtual ~DycsServerConn();

	virtual void connect(const IAddress &addr);
	virtual void disconnect();

	virtual ConnectionState getConnectionState();

	virtual void setTimeouts(cvg_double readTimeout, cvg_double writeTimeout);
	virtual void getTimeouts(cvg_double *readTimeout, cvg_double *writeTimeout);

	virtual void getBuffers(cvg_int *readBuffLen, cvg_int *writeBuffLen);
	virtual void setBuffers(cvg_int readBuffLen, cvg_int writeBuffLen);

	virtual cvg_bool getLocalAddress(IAddress &addr);
	virtual cvg_bool getRemoteAddress(IAddress &addr);

	virtual cvg_ulong read(void *inBuffer, cvg_ulong lengthBytes, cvg_ulong offsetBytes = 0);
	virtual cvg_ulong write(const void *outBuffer, cvg_ulong lengthBytes, cvg_ulong offsetBytes = 0);
	virtual cvg_ulong peek(void *inBuffer, cvg_ulong lengthBytes, cvg_ulong offsetBytes);

	inline cvg_ulong read(Buffer &inBuffer) { return read(&inBuffer, inBuffer.sizeBytes(), 0); }
	inline cvg_ulong write(const Buffer &outBuffer) { return write(&outBuffer, outBuffer.sizeBytes(), 0); }
	inline cvg_ulong peek(Buffer &inBuffer) { return peek(&inBuffer, inBuffer.sizeBytes(), 0); }
};

}

#include "../../../../sources/DycsServerConn.hh"

#endif /* DYCSCLIENT_H_ */
