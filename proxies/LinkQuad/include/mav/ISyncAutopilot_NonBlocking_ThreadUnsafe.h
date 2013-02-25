/*
 * ISyncAutopilot_NonBlocking_ThreadUnsafe.h
 *
 *  Created on: 13/05/2012
 *      Author: Ignacio Mellado-Bataller
 */

#ifndef ISYNCAUTOPILOT_NONBLOCKING_THREADUNSAFE_H_
#define ISYNCAUTOPILOT_NONBLOCKING_THREADUNSAFE_H_

#include "ISyncAutopilot.h"
#include <atlante.h>

namespace Mav {

class ISyncAutopilot_NonBlocking_ThreadUnsafe : public virtual ISyncAutopilot {
public:
	virtual inline ~ISyncAutopilot_NonBlocking_ThreadUnsafe() {}
	// This method prevents compiling a SyncToAsyncAutopilot adapter template for
	// non-blocking thread-unsafe classes with classes that are not non-blocking
	// and thread-unsafe
	inline cvg_bool isNonBlockingThreadUnsafe() { return true; }
};

}

#endif /* ISYNCAUTOPILOT_NONBLOCKING_THREADUNSAFE_H_ */
