/*
 * AltitudeSensor.h
 *
 *  Created on: 01/06/2012
 *      Author: Ignacio Mellado-Bataller
 */

#ifndef ALTITUDESENSOR_H_
#define ALTITUDESENSOR_H_

#include "mav/linkquad/AltitudeFilter.h"

namespace Mav {
namespace Linkquad {

class AltitudeSensor {
private:
	cvg_bool isOpen;
	Mav::Linkquad::AltitudeFilter altitudeFilter;

public:
	AltitudeSensor();
	virtual ~AltitudeSensor();

	void open();
	void close();

	// Samples come from a 12-bit ADC
	cvg_double getAltitude(cvg_uint sample);
};

}
}

#endif /* ALTITUDESENSOR_H_ */
