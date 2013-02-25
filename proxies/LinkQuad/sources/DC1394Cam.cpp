/*
 * DC1394Cam.cpp
 *
 *  Created on: 02/10/2012
 *      Author: Ignacio Mellado-Bataller
 */
#include <iostream>
#include "cam/dc1394/DC1394Cam.h"

#define FRAME_WIDTH		640
#define FRAME_HEIGHT	480
#define FRAME_FORMAT	ANY_FORMAT
#define FRAME_BUFFERS	10
#define FRAME_RATE		30

namespace Video {

void DC1394Cam::open() {
	driver.open(monoModesAreRaw);	// Our Point Grey camera returns some raw modes as mono
	driver.setVideoMode(FRAME_WIDTH, FRAME_HEIGHT, FRAME_FORMAT, DC1394_DRIVER_FPS_ANY, 30);
	driver.setNumBuffers(FRAME_BUFFERS);
	driver.setFramerate(FRAME_RATE);
	driver.setAutoBrightness();
	driver.setAutoExposure();
	driver.setAutoGain();
	driver.setAutoShutter();
	camStarted = false;
}

void DC1394Cam::close() {
	driver.stopCapture();
	driver.close();
}

cvg_int DC1394Cam::getWidth() {
	cvg_int width, height;
	driver.getFrameSize(&width, &height);
	return width;
}

cvg_int DC1394Cam::getHeight() {
	cvg_int width, height;
	driver.getFrameSize(&width, &height);
	return height;
}

cvg_int DC1394Cam::getFrameLength() {
	cvg_int width, height;
	driver.getFrameSize(&width, &height);
	return width * height * 3;
}

IVideoSource::VideoFormat DC1394Cam::getFormat() {
	return driver.getCurrentVideoFormat();
}

cvg_ulong DC1394Cam::getTicksPerSecond() {
	return CVG_LITERAL_ULONG(1000000);
}

cvg_bool DC1394Cam::captureFrame(void *frameBuffer, cvg_ulong *timestamp) {
	if (!camStarted) {
		driver.startCapture();
		camStarted = true;
	}
	if (driver.waitForFrame((cvg_char *)frameBuffer)) {
		if (timestamp != NULL)
			*timestamp = cvgTimer::getSystemSeconds() * 1e6;
		return true;
	}
	return false;
}

}
