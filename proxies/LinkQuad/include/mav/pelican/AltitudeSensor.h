/*
 * AltitudeSensor.h
 *
 *  Created on: 01/06/2012
 *      Author: Ignacio Mellado-Bataller
 */

#ifndef ALTITUDESENSOR_H_
#define ALTITUDESENSOR_H_

#include <phidget21.h>
#include "mav/pelican/AltitudeFilter.h"

namespace Mav {
namespace Pelican {

class AltitudeSensor {
private:
	CPhidgetInterfaceKitHandle handle;
	bool isOpen;
	AltitudeFilter altitude_filter;

public:
	AltitudeSensor();
	virtual ~AltitudeSensor();

	void open();
	void close();

	double getAltitude();
};

}
}

#endif /* ALTITUDESENSOR_H_ */
