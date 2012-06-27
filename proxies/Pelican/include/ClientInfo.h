/*
 * ClientInfo.h
 *
 *  Created on: 27/04/2012
 *      Author: Ignacio Mellado-Bataller
 *
 *  Description:
 *  This class represents a client instance in the server.
 */

#ifndef CLIENTINFO_H_
#define CLIENTINFO_H_

#include <atlante.h>

template<class _addrT> class _ClientInfo {
public:
	class Id : public _addrT {
	public:
		Id(_addrT a) { (*this) = a; }
	};

private:
	_addrT address;

public:
	inline bool operator == (const _ClientInfo<_addrT> &ci) { return address == ci.address; }

	inline const _addrT &getAddress() { return address; }
	inline void setAddress(const _addrT &addr) { address = addr; }

	inline Id getId() const { return Id(address); }
	inline void setId(const Id &id) { address = id; }
};

#endif /* CLIENTINFO_H_ */
