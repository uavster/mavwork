/*
 * CameraModel.cpp
 *
 *  Created on: 28/02/2012
 *      Author: Ignacio Mellado
 */

#include "cam/CameraModel.h"
#include <sys/time.h>
#include <iostream>
#include <highgui.h>
#include <unistd.h>
#include <stdio.h>

namespace Video {

CameraModel::CameraModel() {
	calIntrinsicMatrix = cvCreateMat(3, 3, CV_32FC1);
	calIntScaledMatrix = cvCreateMat(3, 3, CV_32FC1);
	calDistCoeffs = cvCreateMat(4, 1, CV_32FC1);
	calNeededImages = CAMERA_DEFAULT_CALIBRATION_NUM_IMAGES;
	calNPointsX = CAMERA_CAL_DEFAULT_BOARD_POINTS_X;
	calNPointsY = CAMERA_CAL_DEFAULT_BOARD_POINTS_Y;
	calInterval = CAMERA_CAL_DEFAULT_INTERVAL;
	calResolution = currentResolution = cvSize(320, 240);
	output = NULL;
	undistortMap1 = NULL;
	undistortMap2 = NULL;
	lastFrameSize = cvSize(-1, -1);
	mode = NORMAL;
    cvInitFont(&debugFont, CV_FONT_HERSHEY_PLAIN, 2.0f, 1.0f, 0, 2);
}

CameraModel::CameraModel(cvg_int calWidth, cvg_int calHeight, const Matrix3 &matrix, double *distCoeffs) {
	calResolution = currentResolution = cvSize(calWidth, calHeight);
	calIntrinsicMatrix = cvCreateMat(3, 3, CV_32FC1);
	calIntScaledMatrix = cvCreateMat(3, 3, CV_32FC1);
	calDistCoeffs = cvCreateMat(4, 1, CV_32FC1);
	calNeededImages = CAMERA_DEFAULT_CALIBRATION_NUM_IMAGES;
	calNPointsX = CAMERA_CAL_DEFAULT_BOARD_POINTS_X;
	calNPointsY = CAMERA_CAL_DEFAULT_BOARD_POINTS_Y;
	calInterval = CAMERA_CAL_DEFAULT_INTERVAL;
	output = NULL;
	undistortMap1 = NULL;
	undistortMap2 = NULL;
	lastFrameSize = cvSize(-1, -1);
	mode = NORMAL;
    cvInitFont(&debugFont, CV_FONT_HERSHEY_PLAIN, 1.0f, 1.0f, 0, 2);

	for (cvg_int i = 0; i < 3; i++)
		for (cvg_int j = 0; j < 3; j++)
			CV_MAT_ELEM(*calIntrinsicMatrix, float, i, j) = matrix.value[i][j];
	for (cvg_int i = 0; i < calDistCoeffs->rows; i++)
		CV_MAT_ELEM(*calDistCoeffs, float, i, 0) = (distCoeffs != NULL ? distCoeffs[i] : 0.0f);
}

CameraModel &CameraModel::operator = (const CameraModel &c) {
	// If something changed, release buffers so they are reallocated (on demand) with the correct size
	if (	c.calResolution.width != calResolution.width || c.calResolution.height != calResolution.height ||
			CV_MAT_ELEM(*c.calIntrinsicMatrix, float, 0, 0) != CV_MAT_ELEM(*calIntrinsicMatrix, float, 0, 0) ||
			CV_MAT_ELEM(*c.calIntrinsicMatrix, float, 1, 1) != CV_MAT_ELEM(*calIntrinsicMatrix, float, 1, 1) ||
			CV_MAT_ELEM(*c.calIntrinsicMatrix, float, 0, 2) != CV_MAT_ELEM(*calIntrinsicMatrix, float, 0, 2) ||
			CV_MAT_ELEM(*c.calIntrinsicMatrix, float, 1, 2) != CV_MAT_ELEM(*calIntrinsicMatrix, float, 1, 2)
		)
	{
		destroyBuffers();
	}

	calResolution = c.calResolution;
	currentResolution = c.currentResolution;
	for (cvg_int i = 0; i < 3; i++)
		for (cvg_int j = 0; j < 3; j++)
			CV_MAT_ELEM(*calIntrinsicMatrix, float, i, j) = CV_MAT_ELEM(*c.calIntrinsicMatrix, float, i, j);
	for (cvg_int i = 0; i < calDistCoeffs->rows; i++)
		CV_MAT_ELEM(*calDistCoeffs, float, i, 0) = CV_MAT_ELEM(*c.calDistCoeffs, float, i, 0);
	mode = c.mode;
	return *this;
}

void CameraModel::setMatrixParams(cvg_int calWidth, cvg_int calHeight, double fx, double fy, double cx, double cy) {
	// If something changed, release buffers so they are reallocated (on demand) with the correct size
	if (	calWidth != calResolution.width || calHeight != calResolution.height ||
			fx != CV_MAT_ELEM(*calIntrinsicMatrix, float, 0, 0) || fy != CV_MAT_ELEM(*calIntrinsicMatrix, float, 1, 1) ||
			cx != CV_MAT_ELEM(*calIntrinsicMatrix, float, 0, 2) || cy != CV_MAT_ELEM(*calIntrinsicMatrix, float, 1, 2)
		)
	{
		destroyBuffers();
	}

	calResolution = cvSize(calWidth, calHeight);
	CV_MAT_ELEM(*calIntrinsicMatrix, float, 0, 0) = fx;
	CV_MAT_ELEM(*calIntrinsicMatrix, float, 0, 1) = 0.0f;
	CV_MAT_ELEM(*calIntrinsicMatrix, float, 0, 2) = cx;
	CV_MAT_ELEM(*calIntrinsicMatrix, float, 1, 0) = 0.0f;
	CV_MAT_ELEM(*calIntrinsicMatrix, float, 1, 1) = fy;
	CV_MAT_ELEM(*calIntrinsicMatrix, float, 1, 2) = cy;
	CV_MAT_ELEM(*calIntrinsicMatrix, float, 2, 0) = 0.0f;
	CV_MAT_ELEM(*calIntrinsicMatrix, float, 2, 1) = 0.0f;
	CV_MAT_ELEM(*calIntrinsicMatrix, float, 2, 2) = 1.0f;
}

Matrix3 CameraModel::getCameraMatrix() {
	cvg_double wfactor = currentResolution.width / (cvg_double)calResolution.width;
	cvg_double hfactor = currentResolution.height / (cvg_double)calResolution.height;
	return Matrix3(	CV_MAT_ELEM(*calIntrinsicMatrix, float, 0, 0) * wfactor, CV_MAT_ELEM(*calIntrinsicMatrix, float, 0, 1), CV_MAT_ELEM(*calIntrinsicMatrix, float, 0, 2) * wfactor,
					CV_MAT_ELEM(*calIntrinsicMatrix, float, 1, 0), CV_MAT_ELEM(*calIntrinsicMatrix, float, 1, 1) * hfactor, CV_MAT_ELEM(*calIntrinsicMatrix, float, 1, 2) * hfactor,
					CV_MAT_ELEM(*calIntrinsicMatrix, float, 2, 0), CV_MAT_ELEM(*calIntrinsicMatrix, float, 2, 1), CV_MAT_ELEM(*calIntrinsicMatrix, float, 2, 2)
					);
}

void CameraModel::destroyBuffers() {
	if (output != NULL) { cvReleaseImage(&output); output = NULL; }
	if (undistortMap1 != NULL) { cvReleaseMat(&undistortMap1); undistortMap1 = NULL; }
	if (undistortMap2 != NULL) { cvReleaseMat(&undistortMap2); undistortMap2 = NULL; }
}

CameraModel::~CameraModel() {
	destroyBuffers();
	cvReleaseMat(&calIntrinsicMatrix);
	cvReleaseMat(&calIntScaledMatrix);
	cvReleaseMat(&calDistCoeffs);
}

IplImage *CameraModel::run(IplImage *rgb) {
	// If frame size changed, release buffers so they can be reallocated on demand with the correct size
	if (rgb->width != lastFrameSize.width || rgb->height != lastFrameSize.height) {
		destroyBuffers();
		lastFrameSize = cvSize(rgb->width, rgb->height);
	}

	// If no output buffer yet, allocate it
	if (output == NULL) {
		CvSize size; size.width = rgb->width; size.height = rgb->height;
		output = cvCreateImage(size, IPL_DEPTH_8U, 3);
	}

	switch(mode) {
	case NORMAL: correctImage(rgb); break;
	case CALIBRATION: doCalibration(rgb); break;
	}

	return output;
}

void CameraModel::setMode(CameraMode mode) {
	if (this->mode != CALIBRATION && mode == CALIBRATION) {
		// Init calibration
		cvg_int totalPoints = calNPointsX * calNPointsY;
		calImagePoints = cvCreateMat(totalPoints * calNeededImages, 2, CV_32FC1);
		calObjectPoints = cvCreateMat(totalPoints * calNeededImages, 3, CV_32FC1);
		calPointCounts = cvCreateMat(calNeededImages, 1, CV_32SC1);
		calBWImage = NULL;
		calCorners = new CvPoint2D32f[totalPoints];
		calCurFrame = -1;
	} else if (this->mode == CALIBRATION && mode != CALIBRATION) {
		// Free calibration resources
		cvReleaseMat(&calImagePoints);
		cvReleaseMat(&calObjectPoints);
		cvReleaseMat(&calPointCounts);
		if (calBWImage != NULL) cvReleaseImage(&calBWImage);
		delete [] calCorners;
	}
	this->mode = mode;
}

void CameraModel::doCalibration(IplImage *rgb) {
	if (calBWImage != NULL && (rgb->width != calBWImage->width || rgb->height != calBWImage->height)) {
		cvReleaseImage(&calBWImage);
		calBWImage = NULL;
	}
	if (calBWImage == NULL) {
		calBWImage = cvCreateImage(cvSize(rgb->width, rgb->height), IPL_DEPTH_8U, 1);
	}

	calResolution = currentResolution = cvSize(rgb->width, rgb->height);

	double elapsed;
	struct timeval now;
	gettimeofday(&now, NULL);
	if (calCurFrame >= 0) {
		elapsed = (double)(now.tv_sec - calLastFrameTime.tv_sec) + 1e-6 * (double)(now.tv_usec - calLastFrameTime.tv_usec);
	} else { calLastFrameTime = now; }

	cvCopy(rgb, output);
	cvPutText(output, "CALIBRATING", cvPoint(10, output->height * 0.1), &debugFont, CV_RGB(255, 255, 255));

	if (calCurFrame == -1 || elapsed > calInterval) {
		if (calCurFrame == -1) calCurFrame = 0;

		calLastFrameTime = now;

		CvSize boardSize = cvSize(calNPointsX, calNPointsY);
		int cornerCount;
		int found = cvFindChessboardCorners(rgb, boardSize, calCorners, &cornerCount, CV_CALIB_CB_ADAPTIVE_THRESH | CV_CALIB_CB_FILTER_QUADS);
		cvCvtColor(rgb, calBWImage, CV_BGR2GRAY);
		cvFindCornerSubPix(calBWImage, calCorners, cornerCount, cvSize(11, 11), cvSize(-1, -1), cvTermCriteria(CV_TERMCRIT_EPS + CV_TERMCRIT_ITER, 30, 0.1));

		cvDrawChessboardCorners(output, boardSize, calCorners, cornerCount, found);

		cvg_int boardTotalPoints = calNPointsX * calNPointsY;
		if (cornerCount == boardTotalPoints) {
			for (cvg_int i = 0; i < boardTotalPoints; i++) {
				cvg_int currentRow = calCurFrame * boardTotalPoints + i;
				CV_MAT_ELEM(*calImagePoints, float, currentRow, 0) = calCorners[i].x;
				CV_MAT_ELEM(*calImagePoints, float, currentRow, 1) = calCorners[i].y;
				CV_MAT_ELEM(*calObjectPoints, float, currentRow, 0) = (float)(i / calNPointsX);
				CV_MAT_ELEM(*calObjectPoints, float, currentRow, 1) = (float)(i % calNPointsX);
				CV_MAT_ELEM(*calObjectPoints, float, currentRow, 2) = 0.0f;
			}
			CV_MAT_ELEM(*calPointCounts, int, calCurFrame, 0) = boardTotalPoints;
			calCurFrame++;
			cvSaveImage((cvgString("calibration") + calCurFrame + ".png").c_str(), output);
		}
	}

	if (calCurFrame >= calNeededImages) {
		// We got all the samples: calibrate
/*		CV_MAT_ELEM(*calIntrinsicMatrix, float, 0, 0) = rgb->width;	// Initial focal lengths
		CV_MAT_ELEM(*calIntrinsicMatrix, float, 0, 1) = 0.0f;
		CV_MAT_ELEM(*calIntrinsicMatrix, float, 0, 2) = rgb->width / 2;
		CV_MAT_ELEM(*calIntrinsicMatrix, float, 1, 0) = 0.0f;
		CV_MAT_ELEM(*calIntrinsicMatrix, float, 1, 1) = rgb->width;
		CV_MAT_ELEM(*calIntrinsicMatrix, float, 1, 2) = rgb->height / 2;
		CV_MAT_ELEM(*calIntrinsicMatrix, float, 2, 0) = 0.0f;
		CV_MAT_ELEM(*calIntrinsicMatrix, float, 2, 1) = 0.0f;
		CV_MAT_ELEM(*calIntrinsicMatrix, float, 2, 2) = 1.0f;*/
		CV_MAT_ELEM(*calIntrinsicMatrix, float, 0, 0) = 1.0f;
		CV_MAT_ELEM(*calIntrinsicMatrix, float, 1, 1) = 1.0f;
		cvCalibrateCamera2(	calObjectPoints, calImagePoints, calPointCounts, cvGetSize(rgb),
							calIntrinsicMatrix, calDistCoeffs, NULL, NULL, 0);
		mode = NORMAL;
	}
}

void CameraModel::correctImage(IplImage *rgb) {
	currentResolution = cvSize(rgb->width, rgb->height);

	if (rgb->width == calResolution.width && rgb->height == calResolution.height) {
		//	This is equivalent to and faster than: cvUndistort2(rgb, output, calIntrinsicMatrix, calDistCoeffs);
		if (undistortMap1 == NULL) {
			undistortMap1 = cvCreateMat(rgb->height, rgb->width, CV_16SC2);
			undistortMap2 = cvCreateMat(rgb->height, rgb->width, CV_16UC1);
			cvInitUndistortRectifyMap(calIntrinsicMatrix, calDistCoeffs, NULL, calIntrinsicMatrix, undistortMap1, undistortMap2);
		}
		cvRemap(rgb, output, undistortMap1, undistortMap2);
	} else {
		cvg_double wfactor = rgb->width / (cvg_double)calResolution.width;
		cvg_double hfactor = rgb->height / (cvg_double)calResolution.height;
		CV_MAT_ELEM(*calIntScaledMatrix, float, 0, 0) = CV_MAT_ELEM(*calIntrinsicMatrix, float, 0, 0) * wfactor;
		CV_MAT_ELEM(*calIntScaledMatrix, float, 0, 1) = 0.0f;
		CV_MAT_ELEM(*calIntScaledMatrix, float, 0, 2) = CV_MAT_ELEM(*calIntrinsicMatrix, float, 0, 2) * wfactor;
		CV_MAT_ELEM(*calIntScaledMatrix, float, 1, 0) = 0.0f;
		CV_MAT_ELEM(*calIntScaledMatrix, float, 1, 1) = CV_MAT_ELEM(*calIntrinsicMatrix, float, 1, 1) * hfactor;
		CV_MAT_ELEM(*calIntScaledMatrix, float, 1, 2) = CV_MAT_ELEM(*calIntrinsicMatrix, float, 1, 2) * hfactor;
		CV_MAT_ELEM(*calIntScaledMatrix, float, 2, 0) = 0.0f;
		CV_MAT_ELEM(*calIntScaledMatrix, float, 2, 1) = 0.0f;
		CV_MAT_ELEM(*calIntScaledMatrix, float, 2, 2) = 1.0f;

		if (undistortMap1 == NULL) {
			undistortMap1 = cvCreateMat(rgb->height, rgb->width, CV_16SC2);
			undistortMap2 = cvCreateMat(rgb->height, rgb->width, CV_16UC1);
			cvInitUndistortRectifyMap(calIntScaledMatrix, calDistCoeffs, NULL, calIntScaledMatrix, undistortMap1, undistortMap2);
		}
		cvRemap(rgb, output, undistortMap1, undistortMap2);
	}
}

void CameraModel::saveModel(const char *fileName) const {
	FILE *f = fopen(fileName != NULL ? fileName : "camera.txt", "w");
	if (f == NULL) throw cvgException("[CameraModel] Cannot open file for writting");
	double k1, k2, p1, p2;
	getRadialDistortionCoeffs(&k1, &k2);
	getTangentialDistortionCoeffs(&p1, &p2);
	cvgString s = cvgString() + calResolution.width + " " + calResolution.height + " " + getFocalDistanceX() + " " + getFocalDistanceY() + " " + getCenterX() + " " + getCenterY() + " " + k1 + " " + k2 + " " + p1 + " " + p2;
	s = s.replace(",", ".");
	fprintf(f, "%s\n", s.c_str());
	fclose(f);
}

void CameraModel::loadModel(const char *fileName) {
	if (fileName == NULL) fileName = "camera.txt";
	FILE *f = fopen(fileName, "r");
	if (f == NULL) throw cvgException("[CameraModel] Cannot open file for reading");
	float fx, fy, cx, cy;
	float k1, k2, p1, p2;
	float calX, calY;
	if (fscanf(f, "%f %f %f %f %f %f %f %f %f %f", &calX, &calY, &fx, &fy, &cx, &cy, &k1, &k2, &p1, &p2) != 10) {
		cvgString fullPath = "";
		char path[512];
		path[0] = '\0';
		if (getcwd(path, sizeof(path)) != NULL) fullPath = path;
		if (fullPath.length() == 0 || fullPath[fullPath.length() - 1] != '/') fullPath += "/";
		fullPath += fileName;
		throw cvgException("[CameraModel] Unable to load camera parameters from " + fullPath);
	}
	fclose(f);

	setMatrixParams(calX, calY, fx, fy, cx, cy);
	CV_MAT_ELEM(*calDistCoeffs, float, 0, 0) = k1;
	CV_MAT_ELEM(*calDistCoeffs, float, 1, 0) = k2;
	CV_MAT_ELEM(*calDistCoeffs, float, 2, 0) = p1;
	CV_MAT_ELEM(*calDistCoeffs, float, 3, 0) = p2;
}

double CameraModel::getFocalDistanceX() const {
	cvg_double wfactor = currentResolution.width / (cvg_double)calResolution.width;
	return (double)CV_MAT_ELEM(*calIntrinsicMatrix, float, 0, 0) * wfactor;
}

double CameraModel::getFocalDistanceY() const {
	cvg_double hfactor = currentResolution.height / (cvg_double)calResolution.height;
	return (double)CV_MAT_ELEM(*calIntrinsicMatrix, float, 1, 1) * hfactor;
}

double CameraModel::getCenterX() const {
	cvg_double wfactor = currentResolution.width / (cvg_double)calResolution.width;
	return CV_MAT_ELEM(*calIntrinsicMatrix, float, 0, 2) * wfactor;
}

double CameraModel::getCenterY() const {
	cvg_double hfactor = currentResolution.height / (cvg_double)calResolution.height;
	return CV_MAT_ELEM(*calIntrinsicMatrix, float, 1, 2) * hfactor;
}

}
