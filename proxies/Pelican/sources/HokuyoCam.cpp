/*
 * HokuyoCam.cpp
 *
 *  Created on: 19/06/2012
 *      Author: Ignacio Mellado-Bataller
 */

#include "cam/hokuyo/HokuyoCam.h"

namespace Video {

HokuyoCam::HokuyoCam() {
	buffer = NULL;
}

void HokuyoCam::open() {
	close();
	sensor.open();
	buffer = sensor.createSampleBuffer();
	sensor.read(buffer, NULL);
	numBins = buffer->getLength();
}

void HokuyoCam::close() {
	sensor.close();
	if (buffer != NULL) {
		sensor.destroySampleBuffer(buffer);
		buffer = NULL;
	}
}

cvg_int HokuyoCam::getWidth() {
//	return ceil((numBins * 13) / 8.0);
	return numBins;
}

cvg_int HokuyoCam::getHeight() {
	return 1;
}

cvg_int HokuyoCam::getFrameLength() {
//	return ceil((getWidth() * getHeight() * 13) / 8.0);
	return getWidth() * getHeight() * 2;
}

IVideoSource::VideoFormat HokuyoCam::getFormat() {
	return RAW_D16;
}

cvg_ulong HokuyoCam::getTicksPerSecond() {
	return 1.0 / sensor.getFrequency();
}

cvg_bool HokuyoCam::captureFrame(void *frameBuffer, cvg_ulong *timestamp) {
	cvg_long ts;
	sensor.read(buffer, &ts);
	for (cvg_int i = 0; i < numBins; i++) {
		((cvg_short *)frameBuffer)[i] = (cvg_short)buffer->getSample(i);
	}
	(*timestamp) = ts;

	// Convert long bins to 13-bit bins
/*	cvg_int outputLength = getFrameLength();
	cvg_int outputBit = 0;
	cvg_int inputBit = 0, inputIndex = 0;
	for (cvg_int inputBit = 0; inputBit < outputLength * 8; outputBit++) {
		cvg_short inSample = (cvg_short)buffer.getSample(inputIndex);
		frameBuffer[outputBit >> 3] = (inSample & (1 << (inputBit & 0x7)));
		inputBit++;
		if (inputBit == 13) inputIndex++;
	}*/

}

}
