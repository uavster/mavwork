/*
 * AltSpeedController.cpp
 *
 *  Created on: 11/06/2012
 *      Author: Ignacio Mellado-Bataller
 */

//#define SHOW_DEBUG

#define FEEDFORWARD					0.6 // 0.65072
#define GAIN_P						0.1
#define GAIN_I						(GAIN_P / 2.5)
#define GAIN_D						(GAIN_P * 0.0)

#define ALT_SPEED_MEASURE_SATURATION_FACTOR		3.0

#define COMMAND_UPPER_LIMIT			0.3
#define COMMAND_LOWER_LIMIT			-0.15
#define SUBSAMPLING_DIVIDER			6

#include <iostream>
#include "mav/pelican/AltSpeedController.h"

AltSpeedController::AltSpeedController() {
	pid.setGains(GAIN_P, GAIN_I, GAIN_D);

	pid.enableAntiWindup(true, 0.2);
//	for (cvg_int i = 0; i < ALTSC_D_FILTER_LENGTH; i++) dHistory[i] = 0.0;
	started = false;
	debugCount = 0;
	enabled = false;
	repeatCount = 0;
	lastSpeed = lastSpeedF = 0;

//	float num[] = { 0.079955585370677, 0.0 };
//	float den[] = { 1.0, -0.920044414629323 };

	float num[] = { 0.153518275109386, 0.0 };
	float den[] = { 1.0, -0.846481724890614 };
	outputFilter.create(1, num, den);
}

void AltSpeedController::setReference(cvg_double ref) {
#ifdef SHOW_DEBUG
	debugRef(cvgString("AS_ref:") + ref);
#endif
	pid.setReference(ref);
}

cvg_double AltSpeedController::getOutput() {
	if (!enabled) {
#ifdef SHOW_DEBUG
		debugRef(cvgString("AS_cmd:") + 0.0);
#endif
		return 0.0;
	}

	cvg_double pid_value = pid.getOutput();
	if (pid_value > COMMAND_UPPER_LIMIT) pid_value = COMMAND_UPPER_LIMIT;
	else if (pid_value < COMMAND_LOWER_LIMIT) pid_value = COMMAND_LOWER_LIMIT;
	pid_value += FEEDFORWARD;
#ifdef SHOW_DEBUG
	debugRef(cvgString("AS_cmd:") + pid_value);
#endif
	return pid_value;
}

void AltSpeedController::updateAltitude(cvg_double curAlt, cvg_double maxAltSpeedCommand) {
	if (!enabled) return;

/*	cvg_double elapsed = dTimer.getElapsedSeconds();
	dTimer.restart(started);

	// Filter altitude
	for (cvg_int i = 0; i < ALTSC_D_FILTER_LENGTH - 1; i++)
		dHistory[i] = dHistory[i + 1];
	dHistory[ALTSC_D_FILTER_LENGTH - 1] = curAlt;

	cvg_double fAlt = 0.0;
	cvg_double weight = 2.0 / (ALTSC_D_FILTER_LENGTH + 1);
	for (cvg_int i = 0; i < ALTSC_D_FILTER_LENGTH; i++) {
		fAlt += weight * dHistory[ALTSC_D_FILTER_LENGTH - i - 1];
#if ALTSC_D_FILTER_LENGTH > 1
		weight -= (2.0 / (ALTSC_D_FILTER_LENGTH * (ALTSC_D_FILTER_LENGTH + 1)));
#endif
	}
*/
	cvg_double fAlt = curAlt;

/*	if (fAlt == lastAlt) {
		repeatCount++;
		if (repeatCount >= 10) repeatCount = 0;
	} else {
		repeatCount = 0;
	}
*/
	repeatCount++;
	if (repeatCount >= SUBSAMPLING_DIVIDER) repeatCount = 0;

	cvg_double altSpeed, altSpeedF;
	if (repeatCount == 0) {
		if (!started) {
			started = true;
			lastAlt = fAlt;
		}
		cvg_double elapsed = dTimer.getElapsedSeconds();
		dTimer.restart();
		altSpeed = (fAlt - lastAlt) / elapsed;

		// Altitude speed saturation to avoid altitude sensor peaks
		cvg_double speedLimit = fabs(maxAltSpeedCommand * ALT_SPEED_MEASURE_SATURATION_FACTOR);
		if (altSpeed > speedLimit)
			altSpeed = speedLimit;
		else if (altSpeed < -speedLimit)
			altSpeed = -speedLimit;

		lastAlt = fAlt;
//		altSpeedF = outputFilter.filter(altSpeed);
//		lastSpeedF = altSpeedF;
		lastSpeed = altSpeed;
	} else {
//		altSpeedF = lastSpeedF;
		altSpeed = lastSpeed;
	}

	altSpeedF = outputFilter.filter(altSpeed);

#ifdef SHOW_DEBUG
	debugInfo(cvgString("A_measure:") + curAlt + " A_filt:" + fAlt + " AS_calc:" + altSpeed + " AS_filt:" + altSpeedF);
#endif
	pid.setFeedback(altSpeedF);
}

void AltSpeedController::enable(cvg_bool e) {
	if (enabled && !e) {
		pid.reset();
		started = false;
//		for (cvg_int i = 0; i < ALTSC_D_FILTER_LENGTH; i++) dHistory[i] = 0.0;
	} else if (!enabled && e) {
		pid.reset();
		started = false;
	}
	enabled = e;
}

void AltSpeedController::debugRef(const cvgString &str) {
	if (debugCount > 0) std::cout << " ";
	else std::cout << debugTimer.getElapsedSeconds() << " [cmd] ";
	std::cout << str;
	debugCount++;
	if (debugCount >= 2) {
		std::cout << std::endl;
		debugCount = 0;
	}
}

void AltSpeedController::debugInfo(const cvgString &str) {
	std::cout << debugTimer.getElapsedSeconds() << " [info] " << str.c_str() << std::endl;
}
