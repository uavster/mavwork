/*
 * NavigationServer.cpp
 *
 *  Created on: 21/05/2012
 *      Author: Ignacio Mellado-Bataller
 */

#define template_def		template<class _ServerT, class _CommandT, class _StateT>
#define NavigationServer_	NavigationServer<_ServerT, _CommandT, _StateT>

#define NAVSERVER_READ_TIMEOUT	0.5
#define NAVSERVER_WRITE_TIMEOUT	0.5

namespace Comm {

template_def NavigationServer_::NavigationServer() {
	serverEventHandler.setParent(this);
}

template_def void NavigationServer_::open(const IAddress &addr, cvg_int maxQueueLength) {
	ManagedServer<_ServerT>::open(addr, maxQueueLength);
	addEventListener(&serverEventHandler);
}

template_def void NavigationServer_::close() {
	removeEventListener(&serverEventHandler);
	ManagedServer<_ServerT>::close();
}

template_def void NavigationServer_::manageClient(IConnectedSocket *socket) {
	if (socket->getConnectionState() != IConnectedSocket::CONNECTED) {
		usleep(100000);
		return;
	}

	_CommandT buffer;
	cvgTimer readTimer;
	if (socket->read(&buffer, sizeof(_CommandT)) == sizeof(_CommandT)) {
		buffer.ntoh();
		gotCommandData(buffer);
	}
}

template_def void NavigationServer_::notifyStateInfo(const _StateT &newState) {
	StaticBuffer<_StateT, 1> buffer;
	buffer.copy(&newState, sizeof(_StateT));
	((_StateT *)&buffer)->hton();
	typename ManagedServer<_ServerT>::SocketIterator it = ManagedServer<_ServerT>::beginSocketIterator();
	while(true) {
		IConnectedSocket *socket = getNextSocket(it);
		if (socket == NULL) break;
		socket->write(&buffer, sizeof(_StateT));
	}
	endSocketIterator(it);
}

template_def void NavigationServer_::ManServerListener::gotEvent(EventGenerator &source, const Event &e) {
//std::cout << "NavigationServer::gotEvent : " << typeid(e).name() << std::endl;
//	ManagedServer<_ServerT>::gotEvent(source, e);
//std::cout << "NavigationServer after ManagedServer::gotEvent" << std::endl;

	const typename ManagedServer<_ServerT>::NewClientEvent *nce = dynamic_cast<const typename ManagedServer<_ServerT>::NewClientEvent *>(&e);
	if (nce == NULL) return;

	nce->socket->setTimeouts(NAVSERVER_READ_TIMEOUT, NAVSERVER_WRITE_TIMEOUT);
}

}

#undef NAVSERVER_READ_TIMEOUT
#undef NAVSERVER_WRITE_TIMEOUT

#undef NavigationServer_
#undef template_def
