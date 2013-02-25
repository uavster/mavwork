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
namespace Pelican {

class AltitudeFilter {

private:

	float elapsed;
	bool  checking_time;
	cvgTimer timer;

	enum filterState {
		ABOVE_GROUND = 0,
		ABOVE_OBJECT
	};

	filterState current_state;
	double		altitude;

public:
	AltitudeFilter();
	~AltitudeFilter() {}

	double processAltitude( double altitude_k);

	inline int getState() { return (int) current_state; }

	inline double getAltitude() { return altitude; }

};

}
}

#endif /* ALTITUDE_FILTER_H_ */
