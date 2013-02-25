/*
 * ClientList.cpp
 *
 *  Created on: 04/05/2012
 *      Author: Ignacio Mellado-Bataller
 */

template<class _addrT> ClientList<_addrT>::ClientList() {
}

template<class _addrT> ClientList<_addrT>::~ClientList() {
}

template<class _addrT> ClientInfo *ClientList<_addrT>::addNewClient(const ClientInfo &cinfo) {
	if (!mutex.lock()) throw cvgException("[ClientInfo] Unable to lock mutex in addNewClient()");

	ClientInfo *cInfo = NULL;
	try {
		if (getClientById(cinfo.getId()) != NULL)
			throw cvgException("[ClientInfo] The client is already in the list");
		clients[cinfo.getId()] = cinfo;
		cInfo = &clients[cinfo.getId()];
	} catch (cvgException &e) {
		mutex.unlock();
		throw e;
	}

	mutex.unlock();

	return cInfo;
}

template<class _addrT> cvg_bool ClientList<_addrT>::removeClient(const ClientInfo::Id &cId) {
	if (!mutex.lock()) throw cvgException("[ClientInfo] Unable to lock mutex in removeClient()");

	cvg_bool result = clients.erase(cId) == 1;

	mutex.unlock();

	return result;
}

template<class _addrT> ClientInfo *ClientList<_addrT>::getClientById(const ClientInfo::Id &cId) {
	if (!mutex.lock()) throw cvgException("[ClientInfo] Unable to lock mutex in getClientById()");

	ClientMap::iterator it = clients.find(cId);

	mutex.unlock();

	return it != clients.end() ? &(it->second) : NULL;
}
