/*
 * OpticalFlow.cpp
 *
 *  Created on: 25/05/2012
 *      Author: Ignacio Mellado-Bataller
 */

#define PYRAMID_LEVELS		4
#define WINDOW_SIZE			7
#define MIN_FEATURE_QUALITY_TO_ACCEPT	0.3

#include "mav/linkquad/OpticalFlow.h"
#include <highgui.h>

namespace Mav {
namespace Linkquad {

OpticalFlow::OpticalFlow() {
	currentFrame = NULL;
	lastFrame = NULL;
	eigImage = NULL;
	tempImage = NULL;
	pyramid1 = NULL;
	pyramid2 = NULL;
	lastFrameFeatures = NULL;
	newFeatures = NULL;
	foundFeatures = NULL;
	errorFeatures = NULL;
	lastFrameFeaturesCopy = NULL;
}

OpticalFlow::~OpticalFlow() {
	destroy();
}

void OpticalFlow::create(cvg_int imageWidth, cvg_int imageHeight, cvg_int numFeatures) {
	destroy();
	try {
		currentFrame = cvCreateImage(cvSize(imageWidth, imageHeight), IPL_DEPTH_8U, 1);
		if (currentFrame == NULL) throw cvgException("[OpticalFlow] Cannot create image");
		lastFrame = cvCreateImage(cvSize(imageWidth, imageHeight), IPL_DEPTH_8U, 1);
		if (lastFrame == NULL) throw cvgException("[OpticalFlow] Cannot create image");
		eigImage = cvCreateImage(cvSize(imageWidth, imageHeight), IPL_DEPTH_8U, 1);
		if (eigImage == NULL) throw cvgException("[OpticalFlow] Cannot create image");
		tempImage = cvCreateImage(cvSize(imageWidth, imageHeight), IPL_DEPTH_8U, 1);
		if (tempImage == NULL) throw cvgException("[OpticalFlow] Cannot create image");
		pyramid1 = cvCreateImage(cvSize(imageWidth, imageHeight), IPL_DEPTH_8U, 1);
		if (pyramid1 == NULL) throw cvgException("[OpticalFlow] Cannot create image");
		pyramid2 = cvCreateImage(cvSize(imageWidth, imageHeight), IPL_DEPTH_8U, 1);
		if (pyramid2 == NULL) throw cvgException("[OpticalFlow] Cannot create image");

		maxNumFeatures = numFeatures;
		lastFrameFeatures = new CvPoint2D32f[numFeatures];
		lastFrameFeaturesCopy = new CvPoint2D32f[numFeatures];
		newFeatures = new CvPoint2D32f[numFeatures];
		foundFeatures = new char[numFeatures];
		errorFeatures = new float[numFeatures];

		firstRun = true;
		enableDebug(false, NULL);
	} catch (cvgException &e) {
		destroy();
		throw e;
	}
}

void OpticalFlow::destroy() {
	if (lastFrameFeatures != NULL) { delete [] lastFrameFeatures; lastFrameFeatures = NULL; }
	if (newFeatures != NULL) { delete [] newFeatures; newFeatures = NULL; }
	if (foundFeatures != NULL) { delete [] foundFeatures; foundFeatures = NULL; }
	if (errorFeatures != NULL) { delete [] errorFeatures; errorFeatures = NULL; }
	if (lastFrameFeaturesCopy != NULL) { delete [] lastFrameFeaturesCopy; lastFrameFeaturesCopy = NULL; }

	if (currentFrame != NULL) { cvReleaseImage(&currentFrame); currentFrame = NULL; }
	if (lastFrame != NULL) { cvReleaseImage(&lastFrame); lastFrame = NULL; }
	if (eigImage != NULL) { cvReleaseImage(&eigImage); eigImage = NULL; }
	if (tempImage != NULL) { cvReleaseImage(&tempImage); tempImage = NULL; }
	if (pyramid1 != NULL) { cvReleaseImage(&tempImage); pyramid1 = NULL; }
	if (pyramid2 != NULL) { cvReleaseImage(&tempImage); pyramid2 = NULL; }
}

cvg_bool OpticalFlow::process(IplImage *frame, cvg_int *prevFrameNumFeatures, CvPoint2D32f **prevFrameFeatures, char **foundFeaturesInCurrentFrame, CvPoint2D32f **currentFrameFeatures) {
	// Convert image to black & white
	cvConvertImage(frame, currentFrame);

	cvg_bool res = !firstRun;
	if (!firstRun) {
		// Find displacements of the features
		CvSize opticalFlowWindow = cvSize(WINDOW_SIZE, WINDOW_SIZE);
		CvTermCriteria terminationCriteria = cvTermCriteria(CV_TERMCRIT_ITER | CV_TERMCRIT_EPS, 20, 0.3);
		cvCalcOpticalFlowPyrLK(	lastFrame, currentFrame, pyramid1, pyramid2,
								lastFrameFeatures, newFeatures, lastFrameFeaturesCount,
								opticalFlowWindow, PYRAMID_LEVELS, foundFeatures, errorFeatures,
								terminationCriteria, 0
								);

		if (prevFrameNumFeatures != NULL) (*prevFrameNumFeatures) = lastFrameFeaturesCount;
		if (prevFrameFeatures != NULL) {
			memcpy(lastFrameFeaturesCopy, lastFrameFeatures, lastFrameFeaturesCount * sizeof(CvPoint2D32f));
			(*prevFrameFeatures) = lastFrameFeaturesCopy;
		}
		if (foundFeaturesInCurrentFrame != NULL) (*foundFeaturesInCurrentFrame) = foundFeatures;
		if (currentFrameFeatures != NULL) (*currentFrameFeatures) = newFeatures;

		if (showDebug) outputDebugInfo(frame, lastFrameFeaturesCount, foundFeatures, lastFrameFeatures, newFeatures);

	} else firstRun = false;

	lastFrameFeaturesCount = maxNumFeatures;
	// Find good features to track in the current frame
	cvGoodFeaturesToTrack(currentFrame, eigImage, tempImage, lastFrameFeatures, &lastFrameFeaturesCount, MIN_FEATURE_QUALITY_TO_ACCEPT, 0.01, NULL);

	// Store current frame for the next iteration
	cvCopy(currentFrame, lastFrame);

	return res;
}

void OpticalFlow::outputDebugInfo(IplImage *frame, cvg_int numFeatures, char *foundFeatures, CvPoint2D32f *lastFeatures, CvPoint2D32f *currentFeatures) {
	cvg_float scaleFactorX = debugImage->width / frame->width;
	cvg_float scaleFactorY = debugImage->height / frame->height;
	for (cvg_int i = 0; i < numFeatures; i++) {
		if (foundFeatures[i] == 0) continue;
		CvPoint p0, p1;
		p0.x = (int)(lastFeatures[i].x * scaleFactorX); p0.y = (int)(lastFeatures[i].y * scaleFactorY);
		p1.x = (int)(currentFeatures[i].x * scaleFactorX); p1.y = (int)(currentFeatures[i].y * scaleFactorY);

//		double module = sqrt((p0.x - p1.x) * (p0.x - p1.x) + (p0.y - p1.y) * (p0.y - p1.y));
//		double angle = atan2((double)(p1.y - p0.y), (double)(p1.x - p0.x));

		cvLine(debugImage, p0, p1, CV_RGB(0, 128, 0), 1, CV_AA);
		cvCircle(debugImage, p0, 2, CV_RGB(0, 128, 0));
	}

	// Show global displacement
//	cvLine(frame, cvPoint(frame->width / 2, frame->height / 2), cvPoint(frame->width / 2 + globalDisplacement.x, frame->height / 2 + globalDisplacement.y), CV_RGB(255, 255, 255), 3, CV_AA, 0);
}

}
}
