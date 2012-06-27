/*
 * SocketAddress.h
 *
 *  Created on: 27/04/2012
 *      Author: Ignacio Mellado-Bataller
 */

#ifndef IADDRESS_H_
#define IADDRESS_H_

#include <atlante.h>

namespace Comm {

class IAddress {
protected:
	// This is here because operators cannot be virtualized
	virtual bool compare(const IAddress &addr) const = 0;
	virtual IAddress &assign(const IAddress &addr) = 0;
	virtual bool lessThan(const IAddress &addr) const = 0;

public:
	IAddress() {}
	IAddress(const cvgString &address) {}
	virtual cvgString toString() const = 0;

	inline IAddress &operator = (const IAddress &addr) { return assign(addr); }
	inline bool operator == (const IAddress &addr) const { return compare(addr); }
	inline bool operator < (const IAddress &addr) const { return lessThan(addr); }
};

// This is a do-nothing address implementation when no real address is needed at all
class DummyAddress : public virtual IAddress {
protected:
	virtual inline bool compare(const IAddress &addr) const { return false; }
	virtual inline IAddress &assign(const IAddress &addr) { return *this; }
	virtual inline bool lessThan(const IAddress &addr) const { return false; }
public:
	inline DummyAddress(const cvgString &address) : IAddress(address) {}
	virtual inline cvgString toString() const { return ""; }
};

}

#endif /* IADDRESS_H_ */
