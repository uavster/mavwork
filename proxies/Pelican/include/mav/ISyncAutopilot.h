/*
 * ISyncAutopilot.h
 *
 *  Created on: 13/05/2012
 *      Author: Ignacio Mellado-Bataller
 *
 *  Notes: This interface is intended for MAVs that only support synchronous transactions. As the
 *  system uses an IAsyncAutopilot, synchronous MAVs can use any of the provided synchronous-to-
 *  asynchronous adaptation classes. They can choose a flavor for the adapter depending on their
 *  blocking/non-blocking and thread-safe/thread-unsafe nature.
 */

#ifndef ISYNCAUTOPILOT_H_
#define ISYNCAUTOPILOT_H_

#include "Mav1Protocol.h"
#include <atlante.h>
#include "../base/EventListener.h"

namespace Mav {

class ISyncAutopilot : public virtual EventListener {
protected:
	virtual void gotEvent(EventGenerator &source, const Event &e) {}

public:
	virtual void open() = 0;
	virtual void close() = 0;

	// These methods are synchronously called by the system to send commands and receive state info to/from the MAV
	virtual cvg_bool setCommandData(const CommandData &cd) = 0;
	virtual cvg_bool getStateInfo(StateInfo &si) = 0;

	virtual void connectionLost(cvg_bool cl) = 0;
};

}

#endif /* ISYNCAUTOPILOT_H_ */
