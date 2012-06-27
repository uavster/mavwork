/*
 * NavigationServer.h
 *
 *  Created on: 21/05/2012
 *      Author: Ignacio Mellado-Bataller
 */

#ifndef NAVIGATIONSERVER_H_
#define NAVIGATIONSERVER_H_

#include "../ManagedServer.h"

namespace Comm {

template<class _ServerT, class _CommandT, class _StateT> class NavigationServer
	: public virtual ManagedServer<_ServerT > {

private:
	class ManServerListener : public virtual EventListener {
	private:
		NavigationServer<_ServerT, _CommandT, _StateT> *parent;
	protected:
		virtual void gotEvent(EventGenerator &source, const Event &e);
	public:
		inline void setParent(NavigationServer<_ServerT, _CommandT, _StateT> *p) { parent = p; }
	};
	ManServerListener serverEventHandler;

protected:
	virtual void manageClient(IConnectedSocket *socket);

public:
	NavigationServer();
	virtual ~NavigationServer() {}

	virtual void open(const IAddress &addr, cvg_int maxQueueLength = -1);
	virtual void close();

	void notifyStateInfo(const _StateT &newState);
	virtual void gotCommandData(const _CommandT &newCommands) = 0;
};

}

#include "../../../../sources/NavigationServer.hh"

#endif /* NAVIGATIONSERVER_H_ */
