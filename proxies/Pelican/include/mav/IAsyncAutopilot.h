/*
 * IAsyncAutopilot.h
 *
 *  Created on: 13/05/2012
 *      Author: Ignacio Mellado-Bataller
 *
 *  Notes: Classes implementing this interface must generate an event any time
 *  MAV state information is available. The proxy system registers as a listener to
 *  get the info asynchronously and send it to the application as soon as it is available.
 *  This has less delay than polling the MAV.
 *
 *  Drones that support asynchronous notifications may implement this interface directly,
 *  as it is the one used by the system. On the other hand, MAVs that only support
 *  synchronous transactions can use the provide adapter classes.
 */

#ifndef IASYNCAUTOPILOT_H_
#define IASYNCAUTOPILOT_H_

#include "Mav1Protocol.h"
#include "base/EventGenerator.h"
#include "base/EventListener.h"
#include <string.h>

namespace Mav {

class IAsyncAutopilot : public virtual EventGenerator, public virtual EventListener {
protected:
	inline virtual void gotEvent(EventGenerator &source, const Event &e) {}

public:
	virtual void open() = 0;
	virtual void close() = 0;

	// setControlData() is synchronously called by the system to send commands to the MAV.
	// This method must be a non-blocking call.
	virtual void setCommandData(const CommandData &cd) = 0;

	class GotStateInfoEvent : public virtual Event {
	public:
		StateInfo stateInfo;
		inline GotStateInfoEvent(const StateInfo &si) { memcpy(&stateInfo, &si, sizeof(StateInfo)); }
	};
};

}

#endif /* IASYNCAUTOPILOT_H_ */
