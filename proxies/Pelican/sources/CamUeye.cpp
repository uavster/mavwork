/*
 * CamUeye.cpp
 *
 *  Created on: 22/05/2012
 *      Author: Ignacio Mellado-Bataller
 */

#include "cam/camueye/CamUeye.h"
#include "cam/camueye/CamUeye_driver.h"
#include <highgui.h>

namespace Video {

CamUeye::CamUeye() {
	width = height = -1;
}

void CamUeye::open() {
#ifdef CAMUEYE_TEST_MODE
	im = cvLoadImage("test.jpg");
	if (im == NULL) throw cvgException("[CamUeye] Cannot load test image");
#else
	driver = new CamUeye_driver;
	if (	!driver->startCam() ||
			!driver->startLive()
		)
		throw cvgException("[CamUeye] Unable to start camera");

	driver->getImageSize(&width, &height);

	//	Pista de fútbol 6/6/2012 15:00 parte soleada
//	 	driver->SetExposure(0.1);
//		driver->setMasterGain(0.1);

//	Pista de fútbol 5/6/2012 19:00
// 	driver->SetExposure(1.0);
//	driver->setMasterGain(0.1);

 	// Gimnasio
	//driver->SetExposure(IS_SET_ENABLE_AUTO_SHUTTER); //driver->SetExposure(3.0);
	//driver->setMasterGain(IS_SET_AUTO_GAIN_MAX);

//	driver->setRGBGains(1, 0.6, 1);
#endif
}

void CamUeye::close() {
#ifdef CAMUEYE_TEST_MODE
	cvReleaseImage(&im);
#else
//	driver->stopLive();
	driver->stopCam();
	delete driver;
#endif
}

cvg_int CamUeye::getWidth() {
#ifdef CAMUEYE_TEST_MODE
	return im->width;
#else
	return width;
#endif
}

cvg_int CamUeye::getHeight() {
#ifdef CAMUEYE_TEST_MODE
	return im->height;
#else
	return height;
#endif
}

cvg_int CamUeye::getFrameLength() {
#ifdef CAMUEYE_TEST_MODE
	return im->width * im->height * 3;
#else
	return width * height * 3;
#endif
}

CamUeye::VideoFormat CamUeye::getFormat() {
#ifdef CAMUEYE_TEST_MODE
	return RAW_BGR24;
#else
	return RAW_BGR24;
#endif
}

cvg_ulong CamUeye::getTicksPerSecond() {
//	return 1000000;
	return 10000000;
}

cvg_bool CamUeye::captureFrame(void *frameBuffer, cvg_ulong *timestamp) {
#ifdef CAMUEYE_TEST_MODE
	memcpy(frameBuffer, im->imageData, getFrameLength());
	usleep(1e6 / 30);
	return true;
#else
//	static cvgTimer camUeye_timer;
//	if (timestamp != NULL) (*timestamp) = 1e6 * camUeye_timer.getElapsedSeconds();
	return driver->GetImage((char *)frameBuffer, timestamp);
#endif
}

}
