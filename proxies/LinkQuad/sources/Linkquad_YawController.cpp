/*
 * Linkquad_YawController.cpp
 *
 *  Created on: 24/10/2012
 *      Author: Ignacio Mellado-Bataller
 */

#include "mav/linkquad/YawController.h"

#define GAIN_P				5.0					// [deg/s / (deg)]
#define GAIN_I				0.0
#define GAIN_D				(0.1 * GAIN_P)

namespace Mav {
namespace Linkquad {

YawController::YawController() {
	yawPid.setGains(GAIN_P, GAIN_I, GAIN_D);
	enabled = false;
	reference = feedback = 0.0;
}

void YawController::setReference(cvg_double yawRef_deg) {
	reference = yawRef_deg;
	calcError();
}

void YawController::setFeedback(cvg_double yawMeasure_deg) {
	feedback = yawMeasure_deg;
	calcError();
}

void YawController::calcError() {
	cvg_double err = fmod(reference - feedback, 360.0);
	if (err < -180) err += 360;
	else if (err > 180) err -= 360;
	yawPid.setError(err);
}

cvg_double YawController::getOutput() {
	return yawPid.getOutput();
}

void YawController::enable(cvg_bool e) {
	if (enabled != e) {
		yawPid.reset();
	}
	enabled = e;
}

}
}

