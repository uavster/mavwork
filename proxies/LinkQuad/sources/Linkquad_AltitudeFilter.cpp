/*
 * AltitudeFilter.cpp
 *
 *  Created on: Jun 28, 2012
 *      Author: jespestana
 */

#include "mav/linkquad/AltitudeFilter.h"

#define ALTITUDE_STD				0.10
#define ALTITUDE_OBJECT				2.00
#define ALTITUDE_TIMER_THRESHOLD	0.25

namespace Mav {
namespace Linkquad {

AltitudeFilter::AltitudeFilter() {

	current_state = ABOVE_GROUND;
	altitude      = 0.30; 			// m (take-off height)
	checking_time = false;

}

cvg_double AltitudeFilter::processAltitude(cvg_double altitude_k) {

	switch (current_state) {
	case AltitudeFilter::ABOVE_GROUND:
		if ( fabs(altitude - altitude_k) < 3*ALTITUDE_STD ) {
			// We continue to be ABOVE_GROUND
			altitude      = altitude_k;
			checking_time = false;
		} else {
			// We have to check if we are above the object
			if ( fabs(altitude - (altitude_k+ALTITUDE_OBJECT)) < 3*ALTITUDE_STD ) {
				// We are noe above the object
				altitude = altitude_k + ALTITUDE_OBJECT;
				current_state = AltitudeFilter::ABOVE_OBJECT;
				checking_time = false;
			} else {
				// The measurement is considered not good
				if (!checking_time) {
					timer.restart(false);
					checking_time = true;
				} else {
					elapsed = timer.getElapsedSeconds();
					if ( elapsed > ALTITUDE_TIMER_THRESHOLD ) {
						current_state = AltitudeFilter::ABOVE_GROUND;
						altitude      = altitude_k;
						checking_time = false;
					} // else
						// altitude = altitude;
				}
			}
		}
		break;
	case AltitudeFilter::ABOVE_OBJECT:
		if ( fabs(altitude - (altitude_k+ALTITUDE_OBJECT)) < 3*ALTITUDE_STD ) {
			// We continue to be ABOVE_OBJECT
			altitude = altitude_k+ALTITUDE_OBJECT;
			checking_time = false;
		} else {
			// We have to check if we are above ground again
			if ( fabs((altitude) - altitude_k) < 3*ALTITUDE_STD ) {
				// We are noe above ground again
				altitude = altitude_k;
				checking_time = false;
				current_state = AltitudeFilter::ABOVE_GROUND;
			} else {
				// The measurement is considered not good
				if (!checking_time) {
					timer.restart(false);
					checking_time = true;
				} else {
					elapsed = timer.getElapsedSeconds();
					if ( elapsed > ALTITUDE_TIMER_THRESHOLD ) {
						current_state = AltitudeFilter::ABOVE_GROUND;
						altitude      = altitude_k;
						checking_time = false;
					} // else
					// altitude = altitude;
				}
			}
		}
		break;
	}

	return altitude;

}

}
}
