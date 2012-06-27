/*
 * Hokuyo.cpp
 *
 *  Created on: 18/06/2012
 *      Author: Ignacio Mellado-Bataller
 */

#define HOKUYO_FREQUENCY		10.0

#include "cam/hokuyo/Hokuyo.h"

Hokuyo::Hokuyo() {
	started = false;
}

Hokuyo::~Hokuyo() {
	close();
}

void Hokuyo::outError(const char *str) {
	cvgException e = cvgException(cvgString("[Hokuyo] ") + str + ". Error: " + urg_error(&urg));
	close();
	throw e;
}

Hokuyo::SampleBuffer::SampleBuffer(cvg_int numBins) {
	this->numBins = numBins;
	buffer = new long[numBins];
}

Hokuyo::SampleBuffer::~SampleBuffer() {
	delete [] buffer;
}

Hokuyo::SampleBuffer *Hokuyo::createSampleBuffer() {
	return new SampleBuffer(urg_dataMax(&urg));
}

void Hokuyo::destroySampleBuffer(SampleBuffer *sb) {
	if (sb != NULL) delete sb;
}

void Hokuyo::open(const char *deviceString, cvg_int bps) {
	close();
	if (urg_connect(&urg, deviceString, bps) < 0)
		outError("Unable to connect to device");
	started = true;
}

void Hokuyo::close() {
	if (started) {
		urg_disconnect(&urg);
		started = false;
	}
}

void Hokuyo::read(SampleBuffer *sb, cvg_long *timestampMs) {
	int ret = urg_requestData(&urg, URG_GD, URG_FIRST, URG_LAST);
	if (ret < 0) outError("Cannot request data");

	ret = urg_receiveData(&urg, sb->getDataPtr(), sb->getMaxLength());
	if (ret < 0) outError("Cannot receive data");

	sb->setCurrentLength(ret);

	if (timestampMs != NULL)
		(*timestampMs) = urg_recentTimestamp(&urg);
}

cvg_double Hokuyo::getFrequency() {
	return HOKUYO_FREQUENCY;
}
