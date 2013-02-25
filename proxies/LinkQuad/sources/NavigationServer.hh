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
// After this time lapse, with no updates from the autopilot, a state info packet is sent to apps after
// setting some fields on the application layer through notifyLongTimeSinceLastStateInfoUpdate()
#define MAX_TIME_SINCE_NO_STATE_UPDATES		0.5

namespace Comm {

template_def NavigationServer_::NavigationServer() {
	serverEventHandler.setParent(this);
	autopilotWatcher.setParent(this);
}

template_def void NavigationServer_::open(const IAddress &addr, cvg_int maxQueueLength) {
	ManagedServer<_ServerT>::open(addr, maxQueueLength);
	addEventListener(&serverEventHandler);
	autopilotWatcher.start();
}

template_def void NavigationServer_::close() {
	autopilotWatcher.stop();
	removeEventListener(&serverEventHandler);
	ManagedServer<_ServerT>::close();
}

template_def void NavigationServer_::manageClient(IConnectedSocket *socket) {
	if (socket->getConnectionState() != IConnectedSocket::CONNECTED) {
		usleep(100000);
		return;
	}

	_CommandT buffer;
	if (socket->read(&buffer, sizeof(_CommandT)) == sizeof(_CommandT)) {
		buffer.ntoh();
		gotCommandData(buffer);
	}
}

template_def void NavigationServer_::notifyStateInfo(const _StateT &newState) {
	sendStateToClients(false, &newState);
}

template_def void NavigationServer_::sendStateToClients(cvg_bool onlyCheckComm, const _StateT *newState) {
	StaticBuffer<_StateT, 1> buffer;

	typename ManagedServer<_ServerT>::SocketIterator it = ManagedServer<_ServerT>::beginSocketIterator();
	cvg_bool sendToClients = true;
	if (!onlyCheckComm) {
		buffer.copy(newState, sizeof(_StateT));
		((_StateT *)&buffer)->hton();
		memcpy(&lastStateNotification, newState, sizeof(_StateT));
	} else {
		// If time lapse since last state update is too high, send a "bad comm with autopilot" packet
		if (lastStateInfoUpdateTimer.getElapsedSeconds() > MAX_TIME_SINCE_NO_STATE_UPDATES) {
			lastStateInfoUpdateTimer.restart();
			buffer.copy(&lastStateNotification, sizeof(_StateT));
			notifyLongTimeSinceLastStateInfoUpdate((_StateT *)&buffer);
			((_StateT *)&buffer)->hton();
		} else sendToClients = false;
	}
	// Send new state to all clients
	while(sendToClients) {
		IConnectedSocket *socket = getNextSocket(it);
		if (socket == NULL) break;
		socket->write(&buffer, sizeof(_StateT));
	}
	if (!onlyCheckComm) lastStateInfoUpdateTimer.restart(false);
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

template_def NavigationServer_::AutopilotWatcher::AutopilotWatcher() : cvgThread("AutopilotWatcher") {
}

template_def void NavigationServer_::AutopilotWatcher::run() {
	parent->sendStateToClients(true, NULL);
	usleep(200000);
}

}

#undef NAVSERVER_READ_TIMEOUT
#undef NAVSERVER_WRITE_TIMEOUT

#undef NavigationServer_
#undef template_def
