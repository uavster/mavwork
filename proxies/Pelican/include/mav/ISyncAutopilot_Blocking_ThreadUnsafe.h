/*
 * ISyncAutopilot.h
 *
 *  Created on: 12/05/2012
 *      Author: Ignacio Mellado-Bataller
 */

#ifndef ISYNCAUTOPILOT_BLOCKING_THREADUNSAFE_H_
#define ISYNCAUTOPILOT_BLOCKING_THREADUNSAFE_H_

#include "ISyncAutopilot.h"
#include <atlante.h>

namespace Mav {

class ISyncAutopilot_Blocking_ThreadUnsafe : public virtual ISyncAutopilot {
public:
	virtual inline ~ISyncAutopilot_Blocking_ThreadUnsafe() {}
	// This method prevents compiling a SyncToAsyncAutopilot adapter template for
	// blocking thread-unsafe classes with classes that are not blocking
	// and thread-unsafe
	inline cvg_bool isBlockingThreadUnsafe() { return true; }
};

}

#endif /* ISYNCAUTOPILOT_BLOCKING_THREADUNSAFE_H_ */
