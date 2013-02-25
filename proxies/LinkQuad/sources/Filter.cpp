#include <iostream>
#include "control/Filter.h"
#include <string.h>

Filter::Filter() {
	order = 0;
}

Filter::Filter(Filter &f) {
	create(f.order, f.num, f.den);
}

Filter::~Filter() {
	destroy();
}

void Filter::create(int order, float *num, float *den) {
	destroy();

	order++;
	this->order = order;

	this->num = new float[order];
	this->den = new float[order];
	sampleHistory = new float[order];
	outputHistory = new float[order];

	memcpy(this->num, num, order*sizeof(float));
	memcpy(this->den, den, order*sizeof(float));
	for (int i = 0; i < order; i++) { sampleHistory[i] = 0.0f; outputHistory[i] = 0.0f; }

	trigger = -1;
	index = 0;
}

void Filter::destroy() {
	if (order != 0) {
		delete [] num;
		delete [] den;
		delete [] sampleHistory;
		delete [] outputHistory;
		order = 0;
	}
}

int Filter::mod(int a, int b) {
	a = a % b;
	return a >= 0 ? a : b + a;
}

float Filter::filter(float currentSample) {
	sampleHistory[index] = currentSample;
	float output = sampleHistory[index] * num[0];
	for (int i = 1; i < order; i++) {
		int j = mod(index - i, order);
		output += sampleHistory[j] * num[i];
		output -= outputHistory[j] * den[i];
	}
	output /= den[0];
	outputHistory[index] = output;
	index = mod(index + 1, order);
	if (trigger > 0) {
		trigger --;
		return currentSample;
	}
	else return output;
}
