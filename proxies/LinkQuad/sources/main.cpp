/*
 * main.cpp
 *
 *  Created on: 27/04/2012
 *      Author: Ignacio Mellado-Bataller
 */

#include <iostream>
#include <atlante.h>

// Proxy to use
//#include "PelicanProxy.h"
//#define PROXY_CLASS		PelicanProxy
#include "LinkquadProxy.h"
#define PROXY_CLASS		LinkquadProxy

#include <signal.h>

volatile cvg_bool loop = true;

void _endSignalCatcher(int s) {
	loop = false;
}

#include "cam/dc1394/DC1394_driver.h"
#include <highgui.h>

int main(int argc, char **argv) {
	// Set termination signals
	struct sigaction sa;
	bzero(&sa, sizeof(sa));
	sa.sa_handler = &_endSignalCatcher;
	sigaction(SIGINT, &sa, NULL);
	sigaction(SIGTERM, &sa, NULL);

////	Video::DC1394_driver cam1;
//	Video::DC1394Cam cam1;
//	try {
//		cam1.takeMonoModesAsRaw(true);
//		cam1.open();
//		std::cout << "The camera is open" << std::endl;
//
///*		std::list<Video::DC1394_driver::VideoMode> modes;
//		cam1.getSupportedVideoModes(modes);
//		std::list<Video::DC1394_driver::VideoMode>::iterator it;
//		for (it = modes.begin(); it != modes.end(); it++) {
//			std::cout << "Id: " << it->id << ", width: " << it->width << ", height: " << it->height << ", color mode: " << it->colorId << ", is color: " << (it->isColor ? "yes" : "no") << std::endl;
//		}
//*/
///*		cam1.setVideoMode(640, 480, Video::IVideoSource::RAW_BGR24);
//		cam1.setNumBuffers(2);
//		cam1.setFramerate(30);
//		cam1.setAutoBrightness();
//		cam1.setAutoExposure();
//		cam1.setAutoGain();
//		cam1.setAutoShutter();
//		cam1.startCapture();
//*/
//		IplImage *im = cvCreateImage(cvSize(640, 480), IPL_DEPTH_8U, 3);
//		cvNamedWindow("hola", CV_WINDOW_AUTOSIZE);
//		StaticBuffer<char, 640*480*3> buff;
//		while(loop) {
//			cvg_ulong ts;
//			if (cam1.captureFrame(im->imageData, &ts)) { cvShowImage("hola", im); }
//			else usleep(10);
//			cvWaitKey(1);
//		}
//		cvDestroyWindow("hola");
//		cvReleaseImage(&im);
//
//		std::cout << "Closing..." << std::endl;
//		cam1.close();
//		std::cout << "Closed" << std::endl;
//	} catch(cvgException &e) {
//		std::cout << e.getMessage() << std::endl;
//	}
//	return 0;

	PROXY_CLASS proxy;
	try {
		proxy.open();
		if (argc > 1 && strcmp(argv[1], "trim") == 0) proxy.trim();
		while(loop) {
			usleep(250000);
		}
		proxy.close();
	} catch(cvgException &e) {
		std::cerr << "[cvgException] " << e.getMessage() << std::endl;
		proxy.close();
	} catch(std::exception &e) {
		std::cerr << "[std::exception] " << e.what() << std::endl;
		proxy.close();
	}
	return 0;
}



