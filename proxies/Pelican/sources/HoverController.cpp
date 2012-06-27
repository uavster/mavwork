/*
 * HoverController.cpp
 *
 *  Created on: 19/06/2012
 *      Author: Ignacio Mellado-Bataller
 */

#include "mav/pelican/HoverController.h"

#define GAIN_P				3.0					// [deg / (m/s)]
#define GAIN_I				(GAIN_P / 5.0)		// Ti = 5s
#define GAIN_D				0.0
#define MAX_OUTPUT			9.0					// [deg]

HoverController::HoverController() {
	pidVx.setReference(0.0);
	pidVy.setReference(0.0);

	pidVx.setGains(-GAIN_P, -GAIN_I, -GAIN_D);
	pidVy.setGains(GAIN_P, GAIN_I, GAIN_D);
	pidVx.enableMaxOutput(true, MAX_OUTPUT);
	pidVy.enableMaxOutput(true, MAX_OUTPUT);

	pidVx.enableAntiWindup(true, MAX_OUTPUT);
	pidVy.enableAntiWindup(true, MAX_OUTPUT);
}

void HoverController::run(cvg_double Vx_measure, cvg_double Vy_measure, float *pitchCmdDeg, float *rollCmdDeg) {
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



