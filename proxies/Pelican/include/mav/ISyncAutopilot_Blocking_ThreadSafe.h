/*
 * ISyncAutopilot_Blocking_ThreadSafe.h
 *
 *  Created on: 13/05/2012
 *      Author: Ignacio Mellado-Bataller
 */

#ifndef ISYNCAUTOPILOT_BLOCKING_THREADSAFE_H_
#define ISYNCAUTOPILOT_BLOCKING_THREADSAFE_H_

#include "ISyncAutopilot.h"
#include <atlante.h>

namespace Mav {

class ISyncAutopilot_Blocking_ThreadSafe : public virtual ISyncAutopilot {
public:
	virtual inline ~ISyncAutopilot_Blocking_ThreadSafe() {}
	// This method prevents compiling a SyncToAsyncAutopilot adapter template for
	// blocking thread-safe classes with classes that are not blocking
	// and thread-safe
	inline cvg_bool isBlockingThreadSafe() { return true; }
};

}

#endif /* ISYNCAUTOPILOT_BLOCKING_THREADSAFE_H_ */
