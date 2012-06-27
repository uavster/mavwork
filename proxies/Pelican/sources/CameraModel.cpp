/*
 * Camera.cpp
 *
 *  Created on: 28/02/2012
 *      Author: Ignacio Mellado
 */

#include "cam/CameraModel.h"
#include <sys/time.h>
#include <iostream>
#include <highgui.h>
#include <stdio.h>

namespace Video {

CameraModel::CameraModel() {
	calIntrinsicMatrix = cvCreateMat(3, 3, CV_32FC1);
	calDistCoeffs = cvCreateMat(4, 1, CV_32FC1);
	calNeededImages = CAMERA_DEFAULT_CALIBRATION_NUM_IMAGES;
	calNPointsX = CAMERA_CAL_DEFAULT_BOARD_POINTS_X;
	calNPointsY = CAMERA_CAL_DEFAULT_BOARD_POINTS_Y;
	calInterval = CAMERA_CAL_DEFAULT_INTERVAL;
	output = NULL;
	undistortMap1 = NULL;
	undistortMap2 = NULL;
	mode = NORMAL;
    cvInitFont(&debugFont, CV_FONT_HERSHEY_PLAIN, 2.0f, 1.0f, 0, 2);
}

CameraModel::CameraModel(const Matrix3 &matrix, double *distCoeffs) {
	calIntrinsicMatrix = cvCreateMat(3, 3, CV_32FC1);
	calDistCoeffs = cvCreateMat(4, 1, CV_32FC1);
	calNeededImages = CAMERA_DEFAULT_CALIBRATION_NUM_IMAGES;
	calNPointsX = CAMERA_CAL_DEFAULT_BOARD_POINTS_X;
	calNPointsY = CAMERA_CAL_DEFAULT_BOARD_POINTS_Y;
	calInterval = CAMERA_CAL_DEFAULT_INTERVAL;
	output = NULL;
	undistortMap1 = NULL;
	undistortMap2 = NULL;
	mode = NORMAL;
    cvInitFont(&debugFont, CV_FONT_HERSHEY_PLAIN, 1.0f, 1.0f, 0, 2);

	for (cvg_int i = 0; i < 3; i++)
		for (cvg_int j = 0; j < 3; j++)
			CV_MAT_ELEM(*calIntrinsicMatrix, float, i, j) = matrix.value[i][j];
	for (cvg_int i = 0; i < calDistCoeffs->rows; i++)
		CV_MAT_ELEM(*calDistCoeffs, float, i, 0) = (distCoeffs != NULL ? distCoeffs[i] : 0.0f);
}

CameraModel &CameraModel::operator = (const CameraModel &c) {
	for (cvg_int i = 0; i < 3; i++)
		for (cvg_int j = 0; j < 3; j++)
			CV_MAT_ELEM(*calIntrinsicMatrix, float, i, j) = CV_MAT_ELEM(*c.calIntrinsicMatrix, float, i, j);
	for (cvg_int i = 0; i < calDistCoeffs->rows; i++)
		CV_MAT_ELEM(*calDistCoeffs, float, i, 0) = CV_MAT_ELEM(*c.calDistCoeffs, float, i, 0);
	return *this;
}

void CameraModel::setMatrixParams(double fx, double fy, double cx, double cy) {
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
	return Matrix3(	CV_MAT_ELEM(*calIntrinsicMatrix, float, 0, 0), CV_MAT_ELEM(*calIntrinsicMatrix, float, 0, 1), CV_MAT_ELEM(*calIntrinsicMatrix, float, 0, 2),
					CV_MAT_ELEM(*calIntrinsicMatrix, float, 1, 0), CV_MAT_ELEM(*calIntrinsicMatrix, float, 1, 1), CV_MAT_ELEM(*calIntrinsicMatrix, float, 1, 2),
					CV_MAT_ELEM(*calIntrinsicMatrix, float, 2, 0), CV_MAT_ELEM(*calIntrinsicMatrix, float, 2, 1), CV_MAT_ELEM(*calIntrinsicMatrix, float, 2, 2)
					);
}

CameraModel::~CameraModel() {
	if (output != NULL) cvReleaseImage(&output);
	if (undistortMap1 != NULL) cvReleaseMat(&undistortMap1);
	if (undistortMap2 != NULL) cvReleaseMat(&undistortMap2);
	cvReleaseMat(&calIntrinsicMatrix);
	cvReleaseMat(&calDistCoeffs);
}

IplImage *CameraModel::run(IplImage *rgb) {
	if (output == NULL) {
		CvSize size = cvSize(rgb->width, rgb->height);
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
	if (calBWImage == NULL) {
		CvSize size; size.width = rgb->width; size.height = rgb->height;
		calBWImage = cvCreateImage(size, IPL_DEPTH_8U, 1);
	}

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
	//	This is equivalent to and faster than: cvUndistort2(rgb, output, calIntrinsicMatrix, calDistCoeffs);
	if (undistortMap1 == NULL) {
		undistortMap1 = cvCreateMat(rgb->height, rgb->width, CV_16SC2);
		undistortMap2 = cvCreateMat(rgb->height, rgb->width, CV_16UC1);
		cvInitUndistortRectifyMap(calIntrinsicMatrix, calDistCoeffs, NULL, calIntrinsicMatrix, undistortMap1, undistortMap2);
	}
	cvRemap(rgb, output, undistortMap1, undistortMap2);
}

void CameraModel::saveModel(const char *fileName) {
	FILE *f = fopen(fileName != NULL ? fileName : "camera.txt", "w");
	if (f == NULL) throw cvgException("[Camera] Cannot open file for writting");
	double k1, k2, p1, p2;
	getRadialDistortionCoeffs(&k1, &k2);
	getTangentialDistortionCoeffs(&p1, &p2);
	cvgString s = cvgString() + getFocalDistanceX() + " " + getFocalDistanceY() + " " + getCenterX() + " " + getCenterY() + " " + k1 + " " + k2 + " " + p1 + " " + p2;
	s = s.replace(",", ".");
	fprintf(f, "%s\n", s.c_str());
	fclose(f);
}

void CameraModel::loadModel(const char *fileName) {
	FILE *f = fopen(fileName != NULL ? fileName : "camera.txt", "r");
	if (f == NULL) throw cvgException("[Camera] Cannot open file for reading");
	float fx, fy, cx, cy;
	float k1, k2, p1, p2;
	if (fscanf(f, "%f %f %f %f %f %f %f %f", &fx, &fy, &cx, &cy, &k1, &k2, &p1, &p2) != 8)
		throw cvgException("[Camera] Unable to load camera parameters");
	fclose(f);

	setMatrixParams(fx, fy, cx, cy);
	CV_MAT_ELEM(*calDistCoeffs, float, 0, 0) = k1;
	CV_MAT_ELEM(*calDistCoeffs, float, 1, 0) = k2;
	CV_MAT_ELEM(*calDistCoeffs, float, 2, 0) = p1;
	CV_MAT_ELEM(*calDistCoeffs, float, 3, 0) = p2;
}

}
