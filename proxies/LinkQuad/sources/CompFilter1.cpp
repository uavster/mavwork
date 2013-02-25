/*
 * CompFilter1.cpp
 *
 *  Created on: 17/10/2012
 *      Author: Ignacio Mellado-Bataller
 */

#include "control/CompFilter1.h"

CompFilter1::CompFilter1() {
	fs = 1.0f;
	fc = 0.1f;
}

CompFilter1::~CompFilter1() {
}

void CompFilter1::setSamplingFrequency(float fs) {
	this->fs = fs;
	precalc();
}

void CompFilter1::setCutoffFrequency(float fc) {
	this->fc = fc;
	precalc();
}

void CompFilter1::precalc() {
	cvg_float Ts = 1.0f / fs;
	cvg_float tau = 1.0f / (fc * M_PI);
	alpha = tau / (tau + Ts);
	float lNum[2]; lNum[0] = 1.0f - alpha; lNum[1] = 0.0f;
	float lDen[2]; lDen[0] = 1.0f; lDen[1] = -alpha;
	lowpass.create(1, lNum, lDen);
	float hNum[2]; hNum[0] = alpha; hNum[1] = -alpha;
	float hDen[2]; hDen[0] = 1.0f; hDen[1] = -alpha;
	hipass.create(1, hNum, hDen);
}

cvg_float CompFilter1::filter(cvg_float sourceLow, cvg_float sourceHigh) {
	return lowpass.filter(sourceLow) + hipass.filter(sourceHigh);
}



