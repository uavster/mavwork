/*
 * ManagedServer.h
 *
 *  Created on: 20/05/2012
 *      Author: Ignacio Mellado-Bataller
 */

#ifndef MANAGEDSERVER_H_
#define MANAGEDSERVER_H_

#include <list>
#include <map>
#include "../../base/EventListener.h"
#include "../transport/IConnectedSocket.h"

namespace Comm {

#define template_def template<class _ServerT>

template_def class ManagedServer
	: public virtual _ServerT, public virtual EventListener, public virtual EventGenerator {
private:
	class ServerThread : public virtual cvgThread {
	private:
		ManagedServer<_ServerT> *server;
	protected:
		virtual void run();
	public:
		ServerThread() : cvgThread("ManagedServer::ServerThread") {}
		virtual ~ServerThread() {}
		inline void setParent(ManagedServer<_ServerT> *parent) { server = parent; }
	};

	class ClientThread : public virtual cvgThread {
	protected:
		virtual void run();
	private:
		ManagedServer<_ServerT> *server;
	public:
		IConnectedSocket *socket;
	public:
		ClientThread(ManagedServer<_ServerT> *server, IConnectedSocket *socket) : cvgThread("ManagedServer::ClientThread") {
			this->socket = socket;
			this->server = server;
		}
		virtual ~ClientThread() {}
		void stop();
	};

	class ClientEraser : public virtual cvgThread {
	private:
		ManagedServer<_ServerT> *server;
		cvgMutex mutex;
		std::list<ClientThread *> toBeErased;
	protected:
		virtual void run();
		void processList();
	public:
		ClientEraser() : cvgThread("ManagedServer::ClientEraser") {}
		virtual ~ClientEraser() {}
		inline void setParent(ManagedServer<_ServerT> *parent) { server = parent; }
		void markClientForDeletion(ClientThread *ct);
		void stop();
	};

	ServerThread serverThread;
	std::map<IConnectedSocket *, ClientThread *> clients;
	cvgMutex mutex;
	ClientEraser clientEraser;

	void lock();
	void unlock();

protected:
	virtual void gotEvent(EventGenerator &source, const Event &event);

	class SocketIterator {
	private:
		typename std::map<IConnectedSocket *, ClientThread *>::iterator it;
	public:
		SocketIterator(std::map<IConnectedSocket *, ClientThread *> &clients) { this->it = clients.begin(); }
		IConnectedSocket *getNextSocket(std::map<IConnectedSocket *, ClientThread *> &clients) {
			if (it == clients.end()) return NULL;
			typename std::map<IConnectedSocket *, ClientThread *>::iterator it2 = it;
			it++;
			return it2->first;
		}
		SocketIterator &operator = (const SocketIterator &si) { this->it = si.it; return *this; }
	};

	SocketIterator beginSocketIterator() { lock(); return SocketIterator(clients); }
	IConnectedSocket *getNextSocket(SocketIterator &it) { return it.getNextSocket(clients); }
	void endSocketIterator(SocketIterator &it) { unlock(); }

	// Server implementations override this method, which provides specific processing for every client
	virtual void manageClient(IConnectedSocket *clientSocket) = 0;

public:
	ManagedServer();
	virtual ~ManagedServer() {}

	void open(const IAddress &addr, cvg_int clientQueueLength = -1);
	void close();

	// This event is generated when a new client arrives
	class NewClientEvent : public virtual Event {
	public:
		IConnectedSocket *socket;

		NewClientEvent(IConnectedSocket *socket) { this->socket = socket; }
		virtual ~NewClientEvent() {}
	};

	// This event is generated when a client is disconnected
	class ClientDisconnectedEvent : public virtual Event {
	public:
		IConnectedSocket *socket;

		ClientDisconnectedEvent(IConnectedSocket *socket) { this->socket = socket; }
		virtual ~ClientDisconnectedEvent() {}
	};

	// This event is generated when a client thread is stopped
	class ClientStoppedEvent : public virtual Event {
	public:
		IConnectedSocket *socket;

		ClientStoppedEvent(IConnectedSocket *socket) { this->socket = socket; }
		virtual ~ClientStoppedEvent() {}
	};
};

}

#include "../../sources/ManagedServer.hh"

#undef template_def

#endif /* MANAGEDSERVER_H_ */
