/*
 * InetAddress.h
 *
 *  Created on: 27/04/2012
 *      Author: Ignacio Mellado-Bataller
 *
 *  Notes: This file is OS dependent. Current version only
 *  works for Linux
 */

#ifndef INETADDRESS_H_
#define INETADDRESS_H_

#include "../IPortAddress.h"
#include <arpa/inet.h>

namespace Comm {

class InetAddress : public virtual IPortAddress {
private:
	struct sockaddr_in value;

protected:
	// IAddress interface
	virtual bool compare(const IAddress &addr) const;
	virtual InetAddress &assign(const IAddress &addr);
	virtual bool lessThan(const IAddress &addr) const;

public:
	// IAddress interface
	InetAddress();
	InetAddress(const cvgString &address);
	virtual cvgString toString() const;
	inline InetAddress &operator = (const InetAddress &addr) { return assign(addr); }

	// IPortAddress interface
	virtual inline cvg_long getPort() const { return ntohs(value.sin_port); }
	virtual inline void setPort(cvg_long port) { value.sin_port = htons(port); }
	virtual inline cvg_long anyPort() const { return 0; }
	virtual void setAnyHost();

	// Custom interface
	inline const struct in_addr &getHost() const { return value.sin_addr; }
	const struct sockaddr_in *toAddr() const;
	void fromAddr(const struct sockaddr_in &sa);
};

}

#endif /* INETADDRESS_H_ */
