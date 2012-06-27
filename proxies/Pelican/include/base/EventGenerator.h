/*
 * EventGenerator.h
 *
 *  Created on: 30/04/2012
 *      Author: Ignacio Mellado-Bataller
 */

#ifndef EVENTGENERATOR_H_
#define EVENTGENERATOR_H_

#include "Event.h"
#include "EventListener.h"
#include <list>

class EventGenerator {
public:
	virtual ~EventGenerator() {}

	void notifyEvent(const Event &e);

	void addEventListener(EventListener *el);
	void removeEventListener(EventListener *el);
	void removeAllEventListeners();

private:
	std::list<EventListener *> listeners;
};

#endif /* EVENTGENERATOR_H_ */
