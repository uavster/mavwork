/*
 * DycsProtocol.h
 *
 *  Created on: 15/05/2012
 *      Author: Ignacio Mellado-Bataller
 */

#ifndef DYCSPROTOCOL_H_
#define DYCSPROTOCOL_H_

namespace Comm { namespace Proto { namespace Dycs {

#ifndef __PACKED
#define __PACKED	__attribute__((__packed__))
#endif

enum DycsAccessMode { COMMANDER = 0, LISTENER = 1, UNKNOWN_MODE = 2 };

struct DycsClientToServerHeader__ {
	unsigned char requestedAccessMode : 3;
	unsigned char requestedPriority : 5;	// [0, 31] 0 is the highest priority
} __PACKED;

typedef DycsClientToServerHeader__ DycsClientToServerHeader;

struct DycsServertToClientHeader__ {
	unsigned char	grantedAccessMode;
} __PACKED;

typedef DycsServertToClientHeader__ DycsServertToClientHeader;

}}}

#endif /* DYCSPROTOCOL_H_ */
