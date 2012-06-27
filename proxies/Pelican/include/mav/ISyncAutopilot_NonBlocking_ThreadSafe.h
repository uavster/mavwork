/*
 * ISyncAutopilot_NonBlocking_ThreadSafe.h
 *
 *  Created on: 13/05/2012
 *      Author: Ignacio Mellado-Bataller
 */

#ifndef ISYNCAUTOPILOT_NONBLOCKING_THREADSAFE_H_
#define ISYNCAUTOPILOT_NONBLOCKING_THREADSAFE_H_

#include "ISyncAutopilot.h"
#include <atlante.h>

namespace Mav {

class ISyncAutopilot_NonBlocking_ThreadSafe : public virtual ISyncAutopilot {
public:
	virtual inline ~ISyncAutopilot_NonBlocking_Threadsafe() {}
	// This method prevents compiling a SyncToAsyncAutopilot adapter template for
	// non-blocking thread-safe classes with classes that are not non-blocking
	// and thread-safe
	inline cvg_bool isNonBlockingThreadSafe() { return true; }
};

}

#endif /* ISYNCAUTOPILOT_NONBLOCKING_THREADSAFE_H_ */
