/*
 * AltitudeSensor.cpp
 *
 *  Created on: 01/06/2012
 *      Author: Ignacio Mellado-Bataller
 */

// 1st order model (y = CORRECTION_FACTOR * x + CORRECTION_OFFSET) fitted to:
// x [LSB]: 109, 216, 308, 433, 577
// y [m]: 0.26, 0.55, 0.79, 1.12, 1.485

#define CORRECTION_FACTOR	0.002617069317221
#define CORRECTION_OFFSET	-0.018968977638932

#include "mav/linkquad/AltitudeSensor.h"
#include <atlante.h>

namespace Mav {
namespace Linkquad {

AltitudeSensor::AltitudeSensor() {
	isOpen = false;
}

AltitudeSensor::~AltitudeSensor() {
	close();
}

void AltitudeSensor::open() {
	close();
	isOpen = true;
}

void AltitudeSensor::close() {
	isOpen = false;
}

cvg_double AltitudeSensor::getAltitude(cvg_uint sample) {
	return altitudeFilter.processAltitude((cvg_double)CORRECTION_FACTOR * sample + CORRECTION_OFFSET);
}

}
}
