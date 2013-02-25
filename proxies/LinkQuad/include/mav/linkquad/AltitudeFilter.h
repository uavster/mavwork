/*
 * AltitudeFilter.h
 *
 *  Created on: Jun 28, 2012
 *      Author: jespestana
 */

#ifndef ALTITUDE_FILTER_H_
#define ALTITUDE_FILTER_H_

#include <math.h>
#include <atlante.h>

namespace Mav {
namespace Linkquad {

class AltitudeFilter {

private:

	cvg_float elapsed;
	cvg_bool  checking_time;
	cvgTimer timer;

	enum FilterState {
		ABOVE_GROUND = 0,
		ABOVE_OBJECT
	};

	FilterState 	current_state;
	cvg_double		altitude;

public:
	AltitudeFilter();
	inline ~AltitudeFilter() {}

	cvg_double processAltitude( cvg_double altitude_k);

	inline FilterState getState() { return current_state; }

	inline cvg_double getAltitude() { return altitude; }

};

}
}

#endif /* ALTITUDE_FILTER_H_ */
