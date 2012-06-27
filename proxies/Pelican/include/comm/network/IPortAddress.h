/*
 * IPortAddress.h
 *
 *  Created on: 05/05/2012
 *      Author: Ignacio Mellado-Bataller
 *
 *  This interface is common to all addresses that identify the processes
 *  in a machine by an ID, i.e. a port.
 */

#ifndef IPORTADDRESS_H_
#define IPORTADDRESS_H_

#include "IAddress.h"

namespace Comm {

class IPortAddress : public virtual IAddress {
public:
	virtual cvg_long getPort() const = 0;
	virtual void setPort(cvg_long port) = 0;

	virtual cvg_long anyPort() const = 0;
	virtual void setAnyHost() = 0;
};

}

#endif /* IPORTADDRESS_H_ */
