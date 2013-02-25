/*
 * InetAddress.cpp
 *
 *  Created on: 27/04/2012
 *      Author: Ignacio Mellado-Bataller
 */

#include <iostream>
#include "comm/network/inet/InetAddress.h"

namespace Comm {

InetAddress::InetAddress() : IAddress("") {
	value.sin_family = AF_INET;
}

InetAddress::InetAddress(const cvgString &address) : IAddress(address) {
	bzero(&value, sizeof(value));
	value.sin_family = AF_INET;

	try {
		cvgStringVector sv = address.split(":");
		if (sv.size() < 2)
			value.sin_port = 0;
		else if (sv.size() == 2)
			value.sin_port = sv[1].toInt();
		else
			throw cvgException("Invalid Internet address");
		value.sin_port = htons(value.sin_port);

		// Check IP validity
		value.sin_addr.s_addr = inet_addr(sv[0].c_str());
		if (this->value.sin_addr.s_addr == (in_addr_t)-1)
			throw cvgException("Bad IP format");

	} catch(cvgException e) {
		throw cvgException("[InetAddress] Constructor failed. Reason:\n\t" + e.getMessage());
	}
}

cvgString InetAddress::toString() const {
	return cvgString(inet_ntoa(value.sin_addr)) + ":" + (cvg_uint)getPort();
}

const struct sockaddr_in *InetAddress::toAddr() const {
	return &value;
}

void InetAddress::fromAddr(const struct sockaddr_in &sa) {
	if (sa.sin_family != AF_INET)
		throw cvgException("[InetAddress] Argument passed to fromAddr() is not an AF_INET address");
	memcpy(&value, &sa, sizeof(struct sockaddr_in));
}

InetAddress &InetAddress::assign(const IAddress &addr) {
	const InetAddress *iaddr = dynamic_cast<const InetAddress *>(&addr);
	if (iaddr == NULL) throw cvgException("[InetAddress] The right operand of = is not of type InetAddress");
	memcpy(&value, &iaddr->value, sizeof(value));
	return *this;
}

bool InetAddress::compare(const IAddress &addr) const {
	const InetAddress *iaddr = dynamic_cast<const InetAddress *>(&addr);
	return 	iaddr != NULL &&
			value.sin_family == iaddr->value.sin_family &&
			value.sin_addr.s_addr == iaddr->value.sin_addr.s_addr &&
			value.sin_port == iaddr->value.sin_port;
}

bool InetAddress::lessThan(const IAddress &addr) const {
	const InetAddress *iaddr = dynamic_cast<const InetAddress *>(&addr);
	if (value.sin_family != iaddr->value.sin_family)
		return value.sin_family < iaddr->value.sin_family;
	else {
		if (value.sin_addr.s_addr != iaddr->value.sin_addr.s_addr)
			return value.sin_addr.s_addr < iaddr->value.sin_addr.s_addr;
		else {
			return value.sin_port < iaddr->value.sin_port;
		}
	}
}

void InetAddress::setAnyHost() {
	value.sin_addr.s_addr = INADDR_ANY;
}

}
