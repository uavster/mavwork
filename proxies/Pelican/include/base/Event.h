/*
 * Event.h
 *
 *  Created on: 30/04/2012
 *      Author: Ignacio Mellado-Bataller
 */

#ifndef EVENT_H_
#define EVENT_H_

#include <typeinfo>
#include <atlante.h>

class Event {
public:
	virtual ~Event() {}
	// This class must have at least one virtual member, so subclasses may be identified by the typeid
	// operator in EventListener::gotData(const Event &e)
	virtual inline cvgString toString() const { return cvgString(typeid(*this).name()); }
};

#endif /* EVENT_H_ */
