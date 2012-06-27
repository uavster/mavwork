/*
 * ClientList.h
 *
 *  Created on: 04/05/2012
 *      Author: Ignacio Mellado-Bataller
 */

#ifndef CLIENTLIST_H_
#define CLIENTLIST_H_

#include <map>
#include <ClientInfo.h>
#include <atlante.h>

template<class _addrT> class ClientList {
private:
	typedef std::map<ClientInfo::Id, ClientInfo<_addrT> > ClientMap;
	ClientMap clients;
	cvgMutex mutex;

public:
	ClientList();
	~ClientList();

	ClientInfo *addNewClient(const ClientInfo &cinfo);
	cvg_bool removeClient(const ClientInfo::Id &cId);
	ClientInfo *getClientById(const ClientInfo::Id &cId);
};

#include "ClientList.hh"

#endif /* CLIENTLIST_H_ */
