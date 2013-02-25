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
#include <cv.h>

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
	CvMat *calImagePoints, *calObjectPoints, *calPointCounts, *calIntrinsicMatrix, *calDistCoeffs;
	IplImage *calBWImage;
	CvPoint2D32f *calCorners;
	double calInterval;
	cvg_int calCurFrame;
	struct timeval calLastFrameTime;

	CvFont debugFont;

	// Undistor maps
	CvMat *undistortMap1, *undistortMap2;

protected:
	void doCalibration(IplImage *rgb);
	void correctImage(IplImage *rgb);

public:

	CameraModel();
	CameraModel(const Matrix3 &matrix, double *distCoeffs = NULL);
	virtual ~CameraModel();
	inline double getFocalDistanceX() { return (double)CV_MAT_ELEM(*calIntrinsicMatrix, float, 0, 0); }
	inline double getFocalDistanceY() { return (double)CV_MAT_ELEM(*calIntrinsicMatrix, float, 0, 0); }
	inline double getCenterX() { return CV_MAT_ELEM(*calIntrinsicMatrix, float, 0, 2); }
	inline double getCenterY() { return CV_MAT_ELEM(*calIntrinsicMatrix, float, 1, 2); }
	inline void getRadialDistortionCoeffs(double *k1, double *k2) { *k1 = CV_MAT_ELEM(*calDistCoeffs, float, 0, 0); *k2 = CV_MAT_ELEM(*calDistCoeffs, float, 1, 0); }
	inline void getTangentialDistortionCoeffs(double *p1, double *p2) { *p1 = CV_MAT_ELEM(*calDistCoeffs, float, 2, 0); *p2 = CV_MAT_ELEM(*calDistCoeffs, float, 3, 0); }
	CameraModel &operator = (const CameraModel &c);
	void setMatrixParams(double fx, double fy, double cx, double cy);
	Matrix3 getCameraMatrix();

	IplImage *run(IplImage *rgb);
	void setMode(CameraMode mode);
	inline CameraMode getMode() { return mode; }
	inline void setCalibrationInfo(cvg_int numImages, cvg_int boardNumCrossPointsX, cvg_int boardNumCrossPointsY, double imageIntervalSeconds) {
		calNeededImages = numImages;
		calNPointsX = boardNumCrossPointsX;
		calNPointsY = boardNumCrossPointsY;
		calInterval = imageIntervalSeconds;
	}

	void saveModel(const char *fileName = NULL);
	void loadModel(const char *fileName = NULL);
};

}
#endif /* CAMERAMODEL_H_ */
