/*
 * HoverController.cpp
 *
 *  Created on: 19/06/2012
 *      Author: Ignacio Mellado-Bataller
 */

#include "mav/linkquad/HoverController.h"

#define GAIN_P				(3 * 3.0)					// [deg / (m/s)]
#define GAIN_I				(GAIN_P / 5.0)		// Ti = 5s
#define GAIN_D				0.5 //0.01
#define MAX_OUTPUT			(3 * 9.0)					// [deg]
#define ALTITUDE_FOR_NOMINAL_D	0.7					// [m]

namespace Mav {
namespace Linkquad {

HoverController::HoverController() {
	pidVx.setReference(0.0);
	pidVy.setReference(0.0);

	pidVx.setGains(-GAIN_P, -GAIN_I, -GAIN_D);
	pidVy.setGains(GAIN_P, GAIN_I, GAIN_D);
	pidVx.enableMaxOutput(true, MAX_OUTPUT);
	pidVy.enableMaxOutput(true, MAX_OUTPUT);

	pidVx.enableAntiWindup(true, MAX_OUTPUT);
	pidVy.enableAntiWindup(true, MAX_OUTPUT);

	enabled = false;
}

void HoverController::run(cvg_double Vx_measure, cvg_double Vy_measure, cvg_double altitude, float *pitchCmdDeg, float *rollCmdDeg) {
	cvg_double D = GAIN_D;
	if (altitude <= ALTITUDE_FOR_NOMINAL_D) {
		if (altitude > ALTITUDE_FOR_NOMINAL_D * 0.5) {
			D -= 2.0 * GAIN_D * (ALTITUDE_FOR_NOMINAL_D - altitude) /  ALTITUDE_FOR_NOMINAL_D;
			if (D <= 0.0) D = 0.0;
		}
		else
			D = 0.0;
	}
	pidVx.setGains(-GAIN_P, -GAIN_I, -D);
	pidVx.setFeedback(Vx_measure);
	pidVy.setFeedback(Vy_measure);
	(*pitchCmdDeg) = pidVx.getOutput();
	(*rollCmdDeg) = pidVy.getOutput();
}

void HoverController::enable(cvg_bool e) {
	if (enabled != e) {
		pidVx.reset();
		pidVy.reset();
	}
	enabled = e;
}

}
}
