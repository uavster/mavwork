/*
 * AltSpeedController.h
 *
 *  Created on: 11/06/2012
 *      Author: Ignacio Mellado-Bataller
 */

#ifndef ALTSPEEDCONTROLLER_H_
#define ALTSPEEDCONTROLLER_H_

#include "../../control/PID.h"
#include "../../control/Filter.h"

#include <atlante.h>

#define ALTSC_D_FILTER_LENGTH		30

namespace Mav {
namespace Pelican {

class AltSpeedController {
private:
	PID pid;
//	cvg_double dHistory[ALTSC_D_FILTER_LENGTH];
	cvg_double lastAlt;
	cvg_bool started;
	cvgTimer dTimer;
	cvg_bool enabled;

	cvg_int debugCount;
	cvgTimer debugTimer;
	cvg_int repeatCount;
	cvg_double lastSpeed, lastSpeedF;
	Filter outputFilter;

public:
	AltSpeedController();

	void updateAltitude(cvg_double altitude, cvg_double maxAltSpeedCommand);
	void setReference(cvg_double ref);
	cvg_double getOutput();

	void debugRef(const cvgString &str);
	void debugInfo(const cvgString &str);

	void enable(cvg_bool e);
};

}
}

#endif /* ALTSPEEDCONTROLLER_H_ */
