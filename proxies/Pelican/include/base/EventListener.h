/*
 * EventListener.h
 *
 *  Created on: 30/04/2012
 *      Author: Ignacio Mellado-Bataller
 */

#ifndef EVENTLISTENER_H_
#define EVENTLISTENER_H_

#include "Event.h"

class EventGenerator;

class EventListener {
	friend class EventGenerator;
protected:
	virtual void gotEvent(EventGenerator &source, const Event &e) = 0;
};

#endif /* EVENTLISTENER_H_ */
