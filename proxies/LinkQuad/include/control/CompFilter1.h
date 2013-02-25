/*
 * CompFilter1.h
 *
 *  Created on: 17/10/2012
 *      Author: Ignacio Mellado-Bataller
 *
 *  Description: 1st order complementary filter
 */

#ifndef COMPFILTER1_H_
#define COMPFILTER1_H_

#include <atlante.h>
#include "Filter.h"

class CompFilter1 {
public:
	CompFilter1();
	~CompFilter1();

	void setSamplingFrequency(float fs);
	void setCutoffFrequency(float fc);
	cvg_float filter(cvg_float sourceLow, cvg_float sourceHigh);

protected:
	void precalc();

private:
	cvg_float fs, fc, alpha;
	cvg_float value;
	Filter lowpass, hipass;
};

#endif /* COMPFILTER1_H_ */
