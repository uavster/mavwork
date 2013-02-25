/*
 * Camera.h
 *
 *  Created on: 28/02/2012
 *      Author: Ignacio Mellado
 */

#ifndef CAMERAMODEL_H_
#define CAMERAMODEL_H_

#define CAMERA_DEFAULT_CALIBRATION_NUM_IMAGES	50
#define CAMERA_CAL_DEFAULT_BOARD_POINTS_X	9
#define CAMERA_CAL_DEFAULT_BOARD_POINTS_Y	6
#define CAMERA_CAL_DEFAULT_INTERVAL			1.2

#include <atlante.h>
#include <opencv2/opencv.hpp>

namespace Video {

class CameraModel {
public:
	typedef enum { CALIBRATION, NORMAL } CameraMode;

private:
	IplImage *output;

	CameraMode mode;

	// Calibration members
	cvg_int calNeededImages;
	cvg_int calNPointsX, calNPointsY;
	CvMat *calImagePoints, *calObjectPoints, *calPointCounts, *calIntrinsicMatrix, *calIntScaledMatrix, *calDistCoeffs;
	IplImage *calBWImage;
	CvPoint2D32f *calCorners;
	double calInterval;
	cvg_int calCurFrame;
	struct timeval calLastFrameTime;
	CvSize calResolution;
	CvSize currentResolution;

	CvFont debugFont;

	// Undistor maps
	CvMat *undistortMap1, *undistortMap2;
	CvSize lastFrameSize;

protected:
	void doCalibration(IplImage *rgb);
	void correctImage(IplImage *rgb);
	void destroyBuffers();

public:

	CameraModel();
	CameraModel(cvg_int calWidth, cvg_int calHeight, const Matrix3 &matrix, double *distCoeffs = NULL);
	virtual ~CameraModel();

	inline void setResolution(cvg_int width, cvg_int height) { currentResolution = cvSize(width, height); }
	double getFocalDistanceX() const;
	double getFocalDistanceY() const;
	double getCenterX() const;
	double getCenterY() const;
	inline void getRadialDistortionCoeffs(double *k1, double *k2) const { *k1 = CV_MAT_ELEM(*calDistCoeffs, float, 0, 0); *k2 = CV_MAT_ELEM(*calDistCoeffs, float, 1, 0); }
	inline void getTangentialDistortionCoeffs(double *p1, double *p2) const { *p1 = CV_MAT_ELEM(*calDistCoeffs, float, 2, 0); *p2 = CV_MAT_ELEM(*calDistCoeffs, float, 3, 0); }
	CameraModel &operator = (const CameraModel &c);
	void setMatrixParams(cvg_int calWidth, cvg_int calHeight, double fx, double fy, double cx, double cy);
	Matrix3 getCameraMatrix();

	IplImage *run(IplImage *rgb);
	void setMode(CameraMode mode);
	inline CameraMode getMode() const { return mode; }
	inline void setCalibrationInfo(cvg_int numImages, cvg_int boardNumCrossPointsX, cvg_int boardNumCrossPointsY, double imageIntervalSeconds) {
		calNeededImages = numImages;
		calNPointsX = boardNumCrossPointsX;
		calNPointsY = boardNumCrossPointsY;
		calInterval = imageIntervalSeconds;
	}

	void saveModel(const char *fileName = NULL) const;
	void loadModel(const char *fileName = NULL);
};

}
#endif /* CAMERA_H_ */
