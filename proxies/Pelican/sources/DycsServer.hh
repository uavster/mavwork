/*
 * DycsServer.hh
 *
 *  Created on: 15/05/2012
 *      Author: Ignacio Mellado-Bataller
 */
#include <iostream>

#define template_def template<class _TransportServerT, class _TransportSocketT>
#define DycsServer_ DycsServer<_TransportServerT, _TransportSocketT>
#define DycsServerConn_ DycsServerConn<_TransportServerT, _TransportSocketT>

namespace Comm {

using namespace Proto::Dycs;

template_def DycsServer_::DycsServer() {
	currentCommander = NULL;
	currentCommanderPriority = 0;
	socketEventReceiver.setParent(this);
}

template_def void DycsServer_::open(const IAddress &address, cvg_int clientQueueLength) {
	transportServer.open(address, clientQueueLength);
}

template_def void DycsServer_::close() {
	transportServer.close();

	lock();
	while(clients.size() > 0) {
		typename std::list<DycsServerConn<_TransportServerT, _TransportSocketT> *>::iterator it = clients.begin();
		(*it)->removeEventListener(&socketEventReceiver);
		clients.erase(it);
	}
	unlock();
}

template_def DycsServerConn_ *DycsServer_::accept(cvg_double timeout) {
	_TransportSocketT *socket = transportServer.accept(timeout);
	DycsServerConn_ *newClient = NULL;
	if (socket != NULL) {
		newClient = new DycsServerConn_(socket, this);
		newClient->addEventListener(&socketEventReceiver);
		lock();
		clients.push_back(newClient);
		unlock();
	}
	return newClient;
}

template_def cvg_bool DycsServer_::isCommander(DycsServerConn_ *c) {
	lock();
	cvg_bool result = c != NULL && c == currentCommander;
	unlock();
	return result;
}

template_def Proto::Dycs::DycsAccessMode DycsServer_::notifyAccessModeRequest(DycsServerConn_ *client, Proto::Dycs::DycsAccessMode requestedAccessMode, cvg_int requestedPriority) {
	lock();
	if (requestedAccessMode == COMMANDER) {
		// The client wants to be the commander. Let it happen if there is no commander or the current one has lower priority.
		if (currentCommander == NULL ||
			(currentCommander != client && requestedPriority < currentCommanderPriority)) {
			currentCommander = client;
			currentCommanderPriority = requestedPriority;
		} else if (currentCommander == client && requestedPriority != currentCommanderPriority) {
			// The client was already the commander, adjust its priority
			// If there's another awaiting commander with higher priority, it will take control as soon as it notifies an access request through this methods
			currentCommanderPriority = requestedPriority;
		}
	} else {
		// The current client wants to resign. If it was the commander, give control to the highest-priority awaiting commander
		if (client == currentCommander) {
			currentCommander = findAnotherCommander();
		}
	}
	Proto::Dycs::DycsAccessMode mode = (client == currentCommander) ? COMMANDER : LISTENER;
	unlock();
	return mode;
}

template_def DycsServerConn_ *DycsServer_::findAnotherCommander() {
	// Search a new commander with a higher priority
	typename std::list<DycsServerConn_ *>::iterator it;
	DycsServerConn_ *highestPriorityCommander = NULL;
	cvg_int highestPriority = CVG_INT_MAX;
	for (it = clients.begin(); it != clients.end(); it++) {
		cvg_int priority;
		if ((*it) != currentCommander &&
			(*it)->getRequestedAccessMode(&priority) == COMMANDER &&
			priority < highestPriority
			)
		{
			highestPriorityCommander = (*it);
			highestPriority = priority;
		}
	}
	return highestPriorityCommander;
}

template_def void DycsServer_::SocketEventReceiver::gotEvent(EventGenerator &source, const Event &e) {
	const IConnectedSocket::ConnectionStateChangedEvent *csce = dynamic_cast<const IConnectedSocket::ConnectionStateChangedEvent *>(&e);
	if (csce == NULL) return;
	_TransportSocketT *socket = dynamic_cast<_TransportSocketT *>(csce->socket);
	if (csce->oldState == csce->newState) return;
	if (csce->oldState != IConnectedSocket::CONNECTED || csce->newState == IConnectedSocket::CONNECTED) return;
	DycsServerConn_ *client = dynamic_cast<DycsServerConn_ *>(&source);
	if (client == NULL) return;
	server->lock();
	server->clients.remove(client);
	if (client == server->currentCommander) {
std::cout << "The current commander has resigned. Looking for another one..." << std::endl;
		// Find another client who requested COMMANDER mode
		server->currentCommander = server->findAnotherCommander();
std::cout << "Client " << server->currentCommander << " set as new COMMANDER" << std::endl;
	}
	server->unlock();
}

}

#undef DycsServer_
#undef DycsServerConn_
#undef template_def
