/*
 * EventGenerator.cpp
 *
 *  Created on: 30/04/2012
 *      Author: Ignacio Mellado-Bataller
 */

#include "base/EventGenerator.h"

void EventGenerator::addEventListener(EventListener *el) {
	std::list<EventListener *>::iterator i;
	for(i = listeners.begin(); i != listeners.end(); i++)
		if ((*i) == el) return;

	listeners.push_back(el);
}

void EventGenerator::notifyEvent(const Event &e) {
	std::list<EventListener *>::iterator i;
	for(i = listeners.begin(); i != listeners.end(); i++)
		(*i)->gotEvent(*this, e);
}

void EventGenerator::removeEventListener(EventListener *el) {
	listeners.remove(el);
}

void EventGenerator::removeAllEventListeners() {
	listeners.clear();
}
