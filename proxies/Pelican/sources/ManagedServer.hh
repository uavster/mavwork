/*
 * ManagedServer.hh
 *
 *  Created on: 20/05/2012
 *      Author: Ignacio Mellado-Bataller
 */

#define ManagedServer_ ManagedServer<_ServerT>
#define template_def template<class _ServerT>

#define MANAGEDSERVER_ACCEPT_WAIT_TIME		0.1

namespace Comm {

template_def ManagedServer_::ManagedServer() {
	serverThread.setParent(this);
	clientEraser.setParent(this);
}

template_def void ManagedServer_::open(const IAddress &addr, cvg_int clientQueueLength) {
	clientEraser.start();
	_ServerT::open(addr, clientQueueLength);
	serverThread.start();
}

template_def void ManagedServer_::ClientThread::stop() {
	cvgThread::stop();
	socket->removeEventListener(server);
	socket->disconnect();
	server->notifyEvent(ClientStoppedEvent(socket));
}

template_def void ManagedServer_::close() {
	serverThread.stop();
	_ServerT::close();
	clientEraser.stop();

	lock();
	while(clients.size() > 0) {
		typename std::map<IConnectedSocket *, ClientThread *>::iterator it = clients.begin();
		ClientThread *ct = it->second;
		clients.erase(it);
		notifyEvent(ClientDisconnectedEvent(ct->socket));
		ct->stop();
		delete ct;
	}
	unlock();
}

template_def void ManagedServer_::lock() {
	if (!mutex.lock()) throw cvgException("[ManagedServer] Unable to lock mutex");
}

template_def void ManagedServer_::unlock() {
	mutex.unlock();
}

template_def void ManagedServer_::ServerThread::run() {
	// Accept any new incomming connection
	IConnectedSocket *newSocket = server->accept(MANAGEDSERVER_ACCEPT_WAIT_TIME);
	if (newSocket != NULL) {
		ClientThread *clientThread = new ClientThread(server, newSocket);
		newSocket->addEventListener(server);
		server->lock();
		server->clients[newSocket] = clientThread;
		server->unlock();
		// Notify all listeners that a new client arrived
		server->notifyEvent(NewClientEvent(newSocket));
		clientThread->start();
	}
}

template_def void ManagedServer_::ClientThread::run() {
	// Manage client transactions
	server->manageClient(socket);
}

template_def void ManagedServer_::gotEvent(EventGenerator &source, const Event &event) {
//std::cout << "ManagedServer::gotEvent : " << typeid(event).name() << std::endl;
	notifyEvent(event);
//std::cout << "ManagerServer after notifyEvent" << std::endl;

	const IConnectedSocket::ConnectionStateChangedEvent *csce = dynamic_cast<const IConnectedSocket::ConnectionStateChangedEvent *>(&event);
	if (csce == NULL || csce->newState == csce->oldState) return;
	IConnectedSocket *socket = dynamic_cast<IConnectedSocket *>(&source);
	if (socket == NULL) return;

	lock();
	typename std::map<IConnectedSocket *, ClientThread *>::iterator it;
	it = clients.find(socket);
	ClientThread *ct = NULL;
	if (it != clients.end()) {
		if (csce->newState == IConnectedSocket::FAULT || csce->newState == IConnectedSocket::DISCONNECTED) {
			ct = it->second;
			clients.erase(it);
			notifyEvent(ClientDisconnectedEvent(ct->socket));
		}
	}
	unlock();
	if (ct != NULL) clientEraser.markClientForDeletion(ct);
}

template_def void ManagedServer_::ClientEraser::markClientForDeletion(ClientThread *ct) {
	if (!this->isStarted()) return;
	if (!mutex.lock()) throw cvgException("[ManagedServer::ClientEraser] Cannot lock mutex");
	toBeErased.push_back(ct);
	mutex.unlock();
}

template_def void ManagedServer_::ClientEraser::processList() {
	if (!mutex.lock()) throw cvgException("[ManagedServer::ClientEraser] Cannot lock mutex");
	while(toBeErased.size() > 0) {
		typename std::list<ClientThread *>::iterator it = toBeErased.begin();
		ClientThread *ct = *it;
		toBeErased.erase(it);
		ct->stop();
		delete ct;
	}
	mutex.unlock();
}

template_def void ManagedServer_::ClientEraser::run() {
	processList();
	usleep(250000);
}

template_def void ManagedServer_::ClientEraser::stop() {
	cvgThread::stop();
	processList();
}

}

#undef template_def
#undef ManagedServer_
