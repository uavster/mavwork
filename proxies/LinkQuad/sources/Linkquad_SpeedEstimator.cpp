/*
 * SpeedEstimator.cpp
 *
 *  Created on: 25/05/2012
 *      Author: Ignacio Mellado-Bataller
 */
#include <iostream>
#include "mav/linkquad/SpeedEstimator.h"
#include <cv.h>
#include <highgui.h>
#include "cam/FrameGrabber.h"
#include "cam/IVideoSource.h"
#include <stdlib.h>

//#define CALIBRATE_CAMERA
//#define LOCAL_DEBUG

#define DISABLE_RANSAC

#define FRAME_RATE_LIMIT						9

#define COMPLEMENTARY_FILTER_CUTTOFF_FREQ		2.0f
#define ALTITUDE_FILTER_CUTOFF_FREQ				2.0f

// When distortion is low, this may save many cycles on some platforms
#define DISABLE_CAMERA_UNDISTORT

#define OPTFLOW_NUM_FEATURES					30
#define OPTFLOW_FPS_ALTITUDE_FACTOR				1000 //(17*1.5)

// --- IMU TO CAMERA TRANSFORM ---
#define CAMERA_DISTANCE_TO_IMU					0.165
#define CAMERA_TRANSFORM_FROM_DRONE_FRAME		HTMatrix4(RotMatrix3::identity(), Vector3(0, 0, CAMERA_DISTANCE_TO_IMU))

// ---- Camera properties based on FMVU-03MTC datasheet ----
// Natural image dimensions of camera in pixels (sensor size, i.e. max resolution or mode)
#define CAMERA_NATIVE_IMAGE_WIDTH_PIXELS		640 //752
#define CAMERA_NATIVE_IMAGE_HEIGHT_PIXELS		480
// Sensor pixel size in meters
#define CAMERA_SENSOR_PIXEL_WIDTH_METERS		6e-6
#define CAMERA_SENSOR_PIXEL_HEIGHT_METERS		6e-6

// Sensor size in meters
#define CAMERA_SENSOR_WIDTH_METERS				(CAMERA_NATIVE_IMAGE_WIDTH_PIXELS * CAMERA_SENSOR_PIXEL_WIDTH_METERS)
#define CAMERA_SENSOR_HEIGHT_METERS				(CAMERA_NATIVE_IMAGE_HEIGHT_PIXELS * CAMERA_SENSOR_PIXEL_HEIGHT_METERS)

// --- CHARACTERISTICS OF THE PROCESSING IMAGE (SCALING AND CLIPPING) ---
// Altitude Z0
#define BASE_ALTITUDE_METERS					1.0
// Image scaling factor at altitude Z0
#define BASE_IMAGE_SCALING_FACTOR				0.125
// Image clipping factor at altitude Z0
#define BASE_IMAGE_CLIPPING_FACTOR				1.0

// Image scaling factor
#define IMAGE_SCALING_FACTOR_CUR				((curImuAltitude - CAMERA_DISTANCE_TO_IMU) * (BASE_IMAGE_SCALING_FACTOR / BASE_ALTITUDE_METERS))
#define IMAGE_SCALING_FACTOR_PREV				((prevImuAltitude - CAMERA_DISTANCE_TO_IMU) * (BASE_IMAGE_SCALING_FACTOR / BASE_ALTITUDE_METERS))
// Image clipping factor
#define IMAGE_CLIPPING_FACTOR_CUR				((BASE_IMAGE_CLIPPING_FACTOR * BASE_ALTITUDE_METERS) / (curImuAltitude - CAMERA_DISTANCE_TO_IMU))
#define IMAGE_CLIPPING_FACTOR_PREV				((BASE_IMAGE_CLIPPING_FACTOR * BASE_ALTITUDE_METERS) / (prevImuAltitude - CAMERA_DISTANCE_TO_IMU))

// --- PIXEL PHYSICAL DIMENSIONS ON THE CAMERA PLANE FOR THE PROCESSING IMAGE ---
// Image dimensions of a captured frame
#define CAPTURE_IMAGE_WIDTH_PIXELS				frameWidth
#define CAPTURE_IMAGE_HEIGHT_PIXELS				frameHeight
// Image pixel size in meters
#define FRAME_PIXEL_WIDTH_METERS_CUR			(CAMERA_SENSOR_WIDTH_METERS / (CAPTURE_IMAGE_WIDTH_PIXELS * curScalingFactor))
#define FRAME_PIXEL_HEIGHT_METERS_CUR			(CAMERA_SENSOR_HEIGHT_METERS / (CAPTURE_IMAGE_HEIGHT_PIXELS * curScalingFactor))
#define FRAME_PIXEL_WIDTH_METERS_PREV			(CAMERA_SENSOR_WIDTH_METERS / (CAPTURE_IMAGE_WIDTH_PIXELS * prevScalingFactor))
#define FRAME_PIXEL_HEIGHT_METERS_PREV			(CAMERA_SENSOR_HEIGHT_METERS / (CAPTURE_IMAGE_HEIGHT_PIXELS * prevScalingFactor))

// --- PROCESSING IMAGE FOCAL DISTANCE AND CENTER ---
// Focal distance in pixels is incremented by clipping, but decremented by scaling
#define FRAME_FOCAL_DISTANCE_X_PIXELS_CUR		(cameraModel.getFocalDistanceX() * curScalingFactor)
#define FRAME_FOCAL_DISTANCE_Y_PIXELS_CUR		(cameraModel.getFocalDistanceY() * curScalingFactor)
#define FRAME_FOCAL_DISTANCE_X_PIXELS_PREV		(cameraModel.getFocalDistanceX() * prevScalingFactor)
#define FRAME_FOCAL_DISTANCE_Y_PIXELS_PREV		(cameraModel.getFocalDistanceY() * prevScalingFactor)
// Center in pixels is only affected by scaling, as clipping keeps the image center in our case
#define FRAME_CENTER_X_PIXELS_CUR				(cameraModel.getCenterX() * curScalingFactor * curClippingFactor)
#define FRAME_CENTER_Y_PIXELS_CUR				(cameraModel.getCenterY() * curScalingFactor * curClippingFactor)
#define FRAME_CENTER_X_PIXELS_PREV				(cameraModel.getCenterX() * prevScalingFactor * prevClippingFactor)
#define FRAME_CENTER_Y_PIXELS_PREV				(cameraModel.getCenterY() * prevScalingFactor * prevClippingFactor)

// --- GET PIXEL COORDINATES IN THE CAPTURED IMAGE SPACE FROM COORDINATES IN THE PROCESSING IMAGE SPACE ---
#define DPIXEL_TO_IPIXEL_X_CUR(x)				((1.0 / curScalingFactor) * x + cameraModel.getCenterX())
#define DPIXEL_TO_IPIXEL_Y_CUR(y)				((1.0 / curScalingFactor) * y + cameraModel.getCenterY())
#define DPIXEL_TO_IPIXEL_X_PREV(x)				((1.0 / prevScalingFactor) * x + cameraModel.getCenterX())
#define DPIXEL_TO_IPIXEL_Y_PREV(y)				((1.0 / prevScalingFactor) * y + cameraModel.getCenterY())

#define GET_2D_IPIXEL_FROM_3D_DPIXEL_X_CUR(x, z)	(DPIXEL_TO_IPIXEL_X_CUR(FRAME_FOCAL_DISTANCE_X_PIXELS_CUR * x / z))
#define GET_2D_IPIXEL_FROM_3D_DPIXEL_Y_CUR(y, z)	(DPIXEL_TO_IPIXEL_Y_CUR(FRAME_FOCAL_DISTANCE_Y_PIXELS_CUR * y / z))
#define GET_2D_IPIXEL_FROM_3D_DPIXEL_X_PREV(x, z)	(DPIXEL_TO_IPIXEL_X_PREV(FRAME_FOCAL_DISTANCE_X_PIXELS_PREV * x / z))
#define GET_2D_IPIXEL_FROM_3D_DPIXEL_Y_PREV(y, z)	(DPIXEL_TO_IPIXEL_Y_PREV(FRAME_FOCAL_DISTANCE_Y_PIXELS_PREV * y / z))

namespace Mav {
namespace Linkquad {

SpeedEstimator::SpeedEstimator()
	: cvgThread("SpeedEstimator") {
/*
	srand(time(NULL));
	RansacVector3 rv;
	cvg_int numPoints = 25;
	Vector3 set[numPoints];
	Vector3 realVector = Vector3(1, 2, 3);
	for (int i = 0; i < numPoints; i++) {
		double noise = 0.2 * 2 * ((rand()/(double)RAND_MAX) - 0.5);
		set[i] = realVector + Vector3(noise, noise, noise);
	}
	set[5] = set[5] * 2.1;
	set[9] = set[9] * 0.6;
	set[14] = set[14] * 1.5;
	set[22] = set[22] * 2;
	cvg_bool isInSet[numPoints];
	Vector3 bestModel;
	rv.compute(set, numPoints, bestModel, isInSet);
	std::cout << "(" << bestModel.x << ", " << bestModel.y << ", " << bestModel.z << ")" << std::endl;
	for (int i = 0; i < numPoints; i++) {
		std::cout << (isInSet[i] ? "1" : "0") << " ";
	}
	std::cout << std::endl;
	throw cvgException();
*/
	srand(time(NULL));
	image = NULL;
	doDebug = false;
}

void SpeedEstimator::open(cvg_float accelSamplingFreq) {
	close();
	try {
		desiredFramePeriod = 0.0f;
		firstAccelInt = true;
		measureIsValid = false;
		cameraSpeedFromWorld = Vector3();
		camSpeedFromCam = Vector3();
		droneSpeedFromYawedWorld = Vector3();

		lastClippingFactor = lastScalingFactor = -1.0;

#ifndef CALIBRATE_CAMERA
		try {
			cameraModel.loadModel("camera-model.txt");
		} catch (cvgException &e) {
		}
#else
		cameraModel.setMode(Video::CameraModel::CALIBRATION);
#endif

		compFilterX.setSamplingFrequency(accelSamplingFreq);
		compFilterX.setCutoffFrequency(COMPLEMENTARY_FILTER_CUTTOFF_FREQ);
		compFilterY.setSamplingFrequency(accelSamplingFreq);
		compFilterY.setCutoffFrequency(COMPLEMENTARY_FILTER_CUTTOFF_FREQ);

		cvg_float Ts = 1.0f / accelSamplingFreq;
		cvg_float tau = 1.0f / (ALTITUDE_FILTER_CUTOFF_FREQ * M_PI);
		cvg_double alpha = tau / (tau + Ts);
		float lNum[2]; lNum[0] = 1.0f - alpha; lNum[1] = 0.0f;
		float lDen[2]; lDen[0] = 1.0f; lDen[1] = -alpha;
		altitudeFilter.create(1, lNum, lDen);

		start();
	} catch (cvgException &e) {
		close();
		throw e;
	}
}

void SpeedEstimator::close() {
	stop();
	optFlow.destroy();
	if (image != NULL) { cvReleaseImageHeader(&image); image = NULL; }

#ifdef CALIBRATE_CAMERA
	cameraModel.saveModel("camera-model.txt");
#endif
}

void SpeedEstimator::run() {
	try {
		if (getVideoFrame()) {
//cvgTimer perfTimer;
			cvg_ulong curTime = frameTime;

			// Get drone euler angles and altitude for the current frames (as soon as possible, so they're synchronized with the video frame)
			cvg_double prevImuAltitude, curImuAltitude;
			Vector3 prevImuEulerZYX, curImuEulerZYX;
			if (!inputMutex.lock()) throw cvgException("[SpeedEstimator] Cannot lock mutex");
			curImuAltitude = imuAltitude;
			curImuEulerZYX = imuEulerZYX;
			inputMutex.unlock();
			// Get drone euler angles and altitude for the last processed frame
			prevImuAltitude = lastImuAltitude;
			prevImuEulerZYX = lastImuEulerZYX;
			// Store the current values for the next iteration
			lastImuAltitude = curImuAltitude;
			lastImuEulerZYX = curImuEulerZYX;

			if (image == NULL) {
				cameraModel.setResolution(frameWidth, frameHeight);
				CvSize size = cvSize(frameWidth, frameHeight);
				image = cvCreateImageHeader(size, IPL_DEPTH_8U, 3);
				// Clipping and scaling are two independent operations that round the resulting size
				cvg_double finalWidth = BASE_IMAGE_SCALING_FACTOR * ((cvg_int)(BASE_IMAGE_CLIPPING_FACTOR * frameWidth));
				cvg_double finalHeight = BASE_IMAGE_SCALING_FACTOR * ((cvg_int)(BASE_IMAGE_CLIPPING_FACTOR * frameHeight));
				if (image == NULL) throw cvgException("[SpeedEstimator] Cannot create image header");
				scaledImage = cvCreateImage(cvSize(finalWidth, finalHeight), IPL_DEPTH_8U, 3);
				if (scaledImage == NULL) throw cvgException("[SpeedEstimator] Cannot create image");
				optFlow.create(finalWidth, finalHeight, OPTFLOW_NUM_FEATURES);
			}

			// Grab new frame
			cvSetData(image, &frameBuffer, frameWidth * 3);
//std::cout << "Frame->IPL image: " << perfTimer.getElapsedSeconds() << " s" << std::endl;
//perfTimer.restart(true);

	#ifdef CALIBRATE_CAMERA
			cvShowImage("SpeedEstimator", cameraModel.run(image));
			cvWaitKey(1);
	#else
		#ifdef DISABLE_CAMERA_UNDISTORT
			IplImage *calImage = image;
		#else
			IplImage *calImage = cameraModel.run(image);
//std::cout << "Undistort: " << perfTimer.getElapsedSeconds() << " s" << std::endl;
//perfTimer.restart(true);
		#endif
//			cvg_double filteredImuAltitude = altitudeFilter.filter(curImuAltitude);
			cvg_double curClippingFactor = IMAGE_CLIPPING_FACTOR_CUR;
			cvg_double curScalingFactor = IMAGE_SCALING_FACTOR_CUR;
			cvg_double prevClippingFactor = (lastClippingFactor >= 0.0 ? lastClippingFactor : curClippingFactor);
			cvg_double prevScalingFactor = (lastScalingFactor >= 0.0 ? lastScalingFactor : curScalingFactor);

			// Clipping factor can't be higher than the base value
			if (curClippingFactor >= BASE_IMAGE_CLIPPING_FACTOR) {
				curClippingFactor = BASE_IMAGE_CLIPPING_FACTOR;
				curScalingFactor = scaledImage->width / (curClippingFactor * frameWidth);
			}
			else if (curClippingFactor < 0.0) curClippingFactor = 0.0;

			if (curScalingFactor < 0.0) curScalingFactor = 1.0;

			// If the resulting image width or height is smaller than 1 pixel, adjust clippingFactor so that its lower dimension is 1 pixel
			if (frameWidth < frameHeight) {
				if ((cvg_int)((cvg_int)(frameWidth * curClippingFactor) * curScalingFactor) < 1)
					curClippingFactor = 1.0 / (curScalingFactor * frameWidth);
			} else {
				if ((cvg_int)((cvg_int)(frameHeight * curClippingFactor) * curScalingFactor) < 1)
					curClippingFactor = 1.0 / (curScalingFactor * frameHeight);
			}

			lastScalingFactor = curScalingFactor;
			lastClippingFactor = curClippingFactor;

			// Pick a center clip. The clip's size is: image_size * IMAGE_CLIPPING_FACTOR
			CvRect roi = cvRect(frameWidth * (1.0 - curClippingFactor) / 2, frameHeight * (1.0 - curClippingFactor) / 2, frameWidth * curClippingFactor, frameHeight * curClippingFactor);
			// Just for safety, ensure that the roi inside image bounds
			if (roi.x + roi.width > frameWidth) roi.width = frameWidth - roi.x;
			if (roi.y + roi.height > frameHeight) roi.height = frameHeight - roi.y;

//std::cout << "Sc:" << curScalingFactor << "/" << prevScalingFactor << ";Cl:" << curClippingFactor << "/" << prevClippingFactor << ";roi:(" << roi.x << " " << roi.y << " " << roi.width << " " << roi.height << ")" << std::endl;

			cvSetImageROI(calImage, roi);
			// Scale image by a factor of IMAGE_SCALING_FACTOR
			cvResize(calImage, scaledImage, CV_INTER_NN);	// Nearest neighbor is faster than linear interpolation

			if (doDebug) {
				cvSetImageROI(calImage, cvRect(0, 0, scaledImage->width, scaledImage->height));
				cvCopy(scaledImage, calImage);
				cvResetImageROI(calImage);
				CvPoint p1, p2, p3, p4;
				cvg_int offX = curScalingFactor * curClippingFactor * frameWidth / 2;
				cvg_int offY = curScalingFactor * curClippingFactor * frameHeight / 2;
				p1 = cvPoint(DPIXEL_TO_IPIXEL_X_CUR(-offX), DPIXEL_TO_IPIXEL_Y_CUR(-offY));
				p2 = cvPoint(DPIXEL_TO_IPIXEL_X_CUR(offX), DPIXEL_TO_IPIXEL_Y_CUR(-offY));
				p3 = cvPoint(DPIXEL_TO_IPIXEL_X_CUR(offX), DPIXEL_TO_IPIXEL_Y_CUR(offY));
				p4 = cvPoint(DPIXEL_TO_IPIXEL_X_CUR(-offX), DPIXEL_TO_IPIXEL_Y_CUR(offY));
				cvLine(calImage, p1, p2, CV_RGB(255, 255, 255), 2);
				cvLine(calImage, p2, p3, CV_RGB(255, 255, 255), 2);
				cvLine(calImage, p3, p4, CV_RGB(255, 255, 255), 2);
				cvLine(calImage, p4, p1, CV_RGB(255, 255, 255), 2);
			}
			cvResetImageROI(calImage);

//std::cout << "Resize: " << perfTimer.getElapsedSeconds() << " s" << std::endl;
//perfTimer.restart(true);

//std::cout << "Altitude: " << curImuAltitude << std::endl;

			cvg_double elapsed = (cvg_double)(curTime - lastFrameTime) / frameTicksPerSecond;
			lastFrameTime = curTime;

			//optFlow.enableDebug(doDebug, calImage);

			// Get optical flow results
			cvg_int numFeatures;
			CvPoint2D32f *prevFeatures, *curFeatures;
			char *foundFeatures;
	//Vector3 avDisp;
			// Run the optical flow algorithm
//std::cout << scaledImage->width << ", " << scaledImage->height << std::endl;
			if (elapsed > 0 && optFlow.process(scaledImage, &numFeatures, &prevFeatures, &foundFeatures, &curFeatures)) {
//std::cout << "Opt. flow: " << perfTimer.getElapsedSeconds() << " s" << std::endl;
//perfTimer.restart(true);
				// Last image camera reference frame
				RotMatrix3 rot = 	RotMatrix3(Vector3(1, 0, 0), M_PI) *
									RotMatrix3(Vector3(0, 0, 1), prevImuEulerZYX.z * (M_PI / 180.0)) *
									RotMatrix3(Vector3(0, 1, 0), prevImuEulerZYX.y * (M_PI / 180.0)) *
									RotMatrix3(Vector3(1, 0, 0), prevImuEulerZYX.x * (M_PI / 180.0));
				HTMatrix4 droneFromWorld = HTMatrix4(rot, Vector3(0, 0, prevImuAltitude));
				HTMatrix4 prevCamFromWorld = droneFromWorld * CAMERA_TRANSFORM_FROM_DRONE_FRAME;
				HTMatrix4 prevWorldFromCam = prevCamFromWorld.inverse();
				Vector4 floorCenterFromCam4 = prevWorldFromCam * Vector4(0, 0, 0, 1);
				Vector4 floorNormalFromCam4 = prevWorldFromCam * Vector4(0, 0, 1, 1) - floorCenterFromCam4;
				Vector3 prevFloorNormalFromCam = Vector3(floorNormalFromCam4.x, floorNormalFromCam4.y, floorNormalFromCam4.z);
				Vector3 prevFloorCenterFromCam = Vector3(floorCenterFromCam4.x, floorCenterFromCam4.y, floorCenterFromCam4.z);

				// Current image camera reference frame
				rot = 	RotMatrix3(Vector3(1, 0, 0), M_PI) *
						RotMatrix3(Vector3(0, 0, 1), curImuEulerZYX.z * (M_PI / 180.0)) *
						RotMatrix3(Vector3(0, 1, 0), curImuEulerZYX.y * (M_PI / 180.0)) *
						RotMatrix3(Vector3(1, 0, 0), curImuEulerZYX.x * (M_PI / 180.0));
				droneFromWorld = HTMatrix4(rot, Vector3(0, 0, curImuAltitude));
				HTMatrix4 curCamFromWorld = droneFromWorld * CAMERA_TRANSFORM_FROM_DRONE_FRAME;
				HTMatrix4 curWorldFromCam = curCamFromWorld.inverse();
				floorCenterFromCam4 = curWorldFromCam * Vector4(0, 0, 0, 1);
				floorNormalFromCam4 = curWorldFromCam * Vector4(0, 0, 1, 1) - floorCenterFromCam4;
				Vector3 curFloorNormalFromCam = Vector3(floorNormalFromCam4.x, floorNormalFromCam4.y, floorNormalFromCam4.z);
				Vector3 curFloorCenterFromCam = Vector3(floorCenterFromCam4.x, floorCenterFromCam4.y, floorCenterFromCam4.z);
//std::cout << "Matrix setup: " << perfTimer.getElapsedSeconds() << " s" << std::endl;
//perfTimer.restart(true);

				// Reconstruct the point 3D coordinates assuming the floor is flat
				Vector3 avSpeed;
				cvg_int vectorCount = 0;
				Vector3 dispVectors[numFeatures];
				for (cvg_int i = 0; i < numFeatures; i++) {
					if (foundFeatures[i] == 0) continue;
					// 3D point of old feature position on the camera plane
					Vector3 s = Vector3((prevFeatures[i].x - FRAME_CENTER_X_PIXELS_PREV) * FRAME_PIXEL_WIDTH_METERS_PREV,
										(prevFeatures[i].y - FRAME_CENTER_Y_PIXELS_PREV) * FRAME_PIXEL_HEIGHT_METERS_PREV,
										FRAME_FOCAL_DISTANCE_X_PIXELS_PREV * FRAME_PIXEL_WIDTH_METERS_PREV);
					Vector4 prevPointOnFloorFromWorld4 = prevCamFromWorld * findLinePlaneIntersection(s.normalize(), prevFloorNormalFromCam, prevFloorCenterFromCam);
					Vector3 prevPointOnFloorFromWorld = Vector3(prevPointOnFloorFromWorld4.x, prevPointOnFloorFromWorld4.y, prevPointOnFloorFromWorld4.z);

					// 3D point of new feature position on the camera plane
					s = Vector3((curFeatures[i].x - FRAME_CENTER_X_PIXELS_CUR) * FRAME_PIXEL_WIDTH_METERS_CUR,
								(curFeatures[i].y - FRAME_CENTER_Y_PIXELS_CUR) * FRAME_PIXEL_HEIGHT_METERS_CUR,
								FRAME_FOCAL_DISTANCE_X_PIXELS_CUR * FRAME_PIXEL_WIDTH_METERS_CUR);
					Vector4 curPointOnFloorFromWorld4 = curCamFromWorld * findLinePlaneIntersection(s.normalize(), curFloorNormalFromCam, curFloorCenterFromCam);
					Vector3 curPointOnFloorFromWorld = Vector3(curPointOnFloorFromWorld4.x, curPointOnFloorFromWorld4.y, curPointOnFloorFromWorld4.z);

					// Both points are expressed from the current world frame
					Vector3 pointSpeedFromWorld = (prevPointOnFloorFromWorld - curPointOnFloorFromWorld);
					dispVectors[vectorCount] = pointSpeedFromWorld;
//					avSpeed += pointSpeedFromWorld;
					vectorCount++;

					//avDisp += Vector3(curFeatures[i].x - prevFeatures[i].x, curFeatures[i].y - prevFeatures[i].y);

					if (doDebug) {
						CvPoint p0, p1;
						Vector4 prevP = curWorldFromCam * prevPointOnFloorFromWorld;
						Vector4 curP = curWorldFromCam * curPointOnFloorFromWorld;
						p0 = cvPoint(GET_2D_IPIXEL_FROM_3D_DPIXEL_X_PREV(prevP.x, prevP.z), GET_2D_IPIXEL_FROM_3D_DPIXEL_Y_PREV(prevP.y, prevP.z));
						p1 = cvPoint(GET_2D_IPIXEL_FROM_3D_DPIXEL_X_CUR(curP.x, curP.z), GET_2D_IPIXEL_FROM_3D_DPIXEL_Y_CUR(curP.y, curP.z));
						cvLine(calImage, p0, p1, CV_RGB(128, 128, 0), 1, CV_AA);
						cvCircle(calImage, p0, 2, CV_RGB(128, 128, 0));
					}
				}
//std::cout << "3D reconstr. (" << numFeatures << "): " << perfTimer.getElapsedSeconds() << " s" << std::endl;
//perfTimer.restart(true);

	/*			RotMatrix3 rot = 	RotMatrix3(Vector3(1, 0, 0), M_PI) *
									RotMatrix3(Vector3(0, 0, 1), curImuEulerZYX.z * (M_PI / 180.0)) *
									RotMatrix3(Vector3(0, 1, 0), curImuEulerZYX.y * (M_PI / 180.0)) *
									RotMatrix3(Vector3(1, 0, 0), curImuEulerZYX.x * (M_PI / 180.0));
				HTMatrix4 droneFromWorld = HTMatrix4(rot, Vector3(0, 0, curImuAltitude));
				HTMatrix4 camFromWorld = droneFromWorld * CAMERA_TRANSFORM_FROM_DRONE_FRAME;
				HTMatrix4 worldFromCam = camFromWorld.inverse();
				Vector4 floorNormalFromCam4 = worldFromCam * Vector4(1, 0, 0, 1);
				Vector4 floorCenterFromCam4 = worldFromCam * Vector4(0, 0, 0, 1);
				Vector3 floorNormalFromCam = Vector3(floorNormalFromCam4.x, floorNormalFromCam4.y, floorNormalFromCam4.z);
				Vector3 floorCenterFromCam = Vector3(floorCenterFromCam4.x, floorCenterFromCam4.y, floorCenterFromCam4.z);
				CvPoint q0, q1;
				q0.x = (1.0 / IMAGE_SCALING_FACTOR) * (CAMERA_FOCAL_DISTANCE_X_PIXELS * floorCenterFromCam.x / floorCenterFromCam.z + CAMERA_CENTER_X_PIXELS);
				q0.y = (1.0 / IMAGE_SCALING_FACTOR) * (CAMERA_FOCAL_DISTANCE_Y_PIXELS * floorCenterFromCam.y / floorCenterFromCam.z + CAMERA_CENTER_Y_PIXELS);
				q1.x = (1.0 / IMAGE_SCALING_FACTOR) * (CAMERA_FOCAL_DISTANCE_X_PIXELS * (floorNormalFromCam.x) / (floorNormalFromCam.z) + CAMERA_CENTER_X_PIXELS);
				q1.y = (1.0 / IMAGE_SCALING_FACTOR) * (CAMERA_FOCAL_DISTANCE_Y_PIXELS * (floorNormalFromCam.y) / (floorNormalFromCam.z) + CAMERA_CENTER_Y_PIXELS);
				cvLine(calImage, q0, q1, CV_RGB(255, 255, 255), 1, CV_AA);
	*/
//std::cout << "Valid vectors: " << vectorCount << std::endl;
				if (vectorCount <= 0) {
					if (!mutex.lock()) throw cvgException("[SpeedEstimator] Cannot lock mutex");
					measureIsValid = false;
					mutex.unlock();
				} else {
					Vector3 bestModel;
					cvg_bool isInlier[vectorCount];
					cvg_bool newSpeedSample;
#ifndef DISABLE_RANSAC
					newSpeedSample = ransac.compute(dispVectors, vectorCount, bestModel, isInlier);
//std::cout << "RANSAC: " << perfTimer.getElapsedSeconds() << " s" << std::endl;
//perfTimer.restart(true);
#else
					newSpeedSample = true;
					for (cvg_int i = 0; i < vectorCount; i++) {
						bestModel += dispVectors[i];
					}
					bestModel = bestModel / vectorCount;
#endif
					if (!newSpeedSample) {
						if (!mutex.lock()) throw cvgException("[SpeedEstimator] Cannot lock mutex");
						measureIsValid = false;
						mutex.unlock();
					} else {
	//					Vector3 csfw = (avSpeed / vectorCount) / elapsed;
						Vector3 csfw = bestModel / elapsed;

						HTMatrix4 flatDroneFromWorld = HTMatrix4(
															RotMatrix3(Vector3(1, 0, 0), M_PI) *
															RotMatrix3(Vector3(0, 0, 1), curImuEulerZYX.z * (M_PI / 180.0)),
															Vector3(0, 0, curImuAltitude)
															);
						HTMatrix4 worldFromFlatDrone = flatDroneFromWorld.inverse();
						Vector4 dsfyw0 = worldFromFlatDrone * Vector3();
						Vector4 dsfyw = worldFromFlatDrone * csfw;
						dsfyw = dsfyw - dsfyw0;

						Vector4 csfc4 = curWorldFromCam * csfw;
						Vector4 wfc0 = curWorldFromCam * Vector3();
						csfc4.x -= wfc0.x; csfc4.y -= wfc0.y; csfc4.z -= wfc0.z;

						if (!mutex.lock()) throw cvgException("[SpeedEstimator] Cannot lock mutex");
						measureIsValid = true;
						cameraSpeedFromWorld = Vector3(csfw.x, csfw.y, csfw.z);
						camSpeedFromCam = Vector3(csfc4.x, csfc4.y, csfc4.z);
						droneSpeedFromYawedWorld = Vector3(dsfyw.x, dsfyw.y, dsfyw.z);
//std::cout << "(" << dsfyw.x << ", " << dsfyw.y << ", " << dsfyw.z << ")" << std::endl;
						mutex.unlock();
//std::cout << "Last transforms: " << perfTimer.getElapsedSeconds() << " s" << std::endl;
//perfTimer.restart(true);

						if (doDebug) {
							// Draw speed vector on debug image
							CvPoint p0, p1;
							p0 = cvPoint(frameWidth / 2, frameHeight / 2);
							p1 = cvPoint(frameWidth / 2 + 100 * csfc4.x, frameHeight / 2 + 100 * csfc4.y);
							cvLine(calImage, p0, p1, CV_RGB(200, 200, 0), 2, CV_AA);
							cvCircle(calImage, p0, 4, CV_RGB(200, 200, 0));

			/*				p0.x = CAMERA_CENTER_X_PIXELS;
							p0.y = CAMERA_CENTER_Y_PIXELS;
							avDisp = curImuAltitude * (((avDisp / vectorCount)/elapsed) / CAMERA_FOCAL_DISTANCE_X_PIXELS);
							p1.x = CAMERA_CENTER_X_PIXELS + 100 * avDisp.x;
							p1.y = CAMERA_CENTER_Y_PIXELS + 100 * avDisp.y;
							cvLine(calImage, p0, p1, CV_RGB(0, 0, 255), 2, CV_AA);
			*/
							// Draw world reference frame on the ground with the current (x, y) of the camera
							Vector4 q0 = curWorldFromCam * Vector3();
							Vector4 qx = curWorldFromCam * Vector3(0.1, 0, 0);
							Vector4 qy = curWorldFromCam * Vector3(0, 0.1, 0);
							Vector4 qz = curWorldFromCam * Vector3(0, 0, 0.1);
							p0 = cvPoint(GET_2D_IPIXEL_FROM_3D_DPIXEL_X_CUR(q0.x, q0.z), GET_2D_IPIXEL_FROM_3D_DPIXEL_Y_CUR(q0.y, q0.z));
							p1 = cvPoint(GET_2D_IPIXEL_FROM_3D_DPIXEL_X_CUR(qx.x, qx.z), GET_2D_IPIXEL_FROM_3D_DPIXEL_Y_CUR(qx.y, qx.z));
							cvLine(calImage, p0, p1, CV_RGB(255, 0, 0), 2, CV_AA);
							p1 = cvPoint(GET_2D_IPIXEL_FROM_3D_DPIXEL_X_CUR(qy.x, qy.z), GET_2D_IPIXEL_FROM_3D_DPIXEL_Y_CUR(qy.y, qy.z));
							cvLine(calImage, p0, p1, CV_RGB(0, 255, 0), 2, CV_AA);
							p1 = cvPoint(GET_2D_IPIXEL_FROM_3D_DPIXEL_X_CUR(qz.x, qz.z), GET_2D_IPIXEL_FROM_3D_DPIXEL_Y_CUR(qz.y, qz.z));
							cvLine(calImage, p0, p1, CV_RGB(0, 0, 255), 2, CV_AA);

//std::cout << "Debug: " << perfTimer.getElapsedSeconds() << " s" << std::endl;
//perfTimer.restart(true);
						}
					}
				}

				notifyEvent(Video::GotVideoFrameEvent(calImage->imageData, calImage->width, calImage->height, calImage->width * calImage->height * 3, 0, frameTime, frameTicksPerSecond));
//std::cout << "notifyEvent(): " << perfTimer.getElapsedSeconds() << " s" << std::endl;
//perfTimer.restart(true);
#ifdef LOCAL_DEBUG
				cvShowImage("SpeedEstimator", calImage);
				cvWaitKey(1);
#endif
			}
	#endif
		} else {
			// No frame was received
			usleep(1000);
		}
	} catch (cvgException &e) {
		std::cout << e.getMessage() << std::endl;
	}
}

void SpeedEstimator::updateCamera(cvg_double altitude, const Vector3 &eulerZYX) {
	if (!inputMutex.lock()) throw cvgException("[SpeedEstimator] Cannot lock mutex");
	imuAltitude = altitude;
	imuEulerZYX = eulerZYX;
	inputMutex.unlock();
	desiredFramePeriod = altitude / OPTFLOW_FPS_ALTITUDE_FACTOR;
}

Vector3 SpeedEstimator::getVisualSpeedInWorldFrame(cvg_bool *isValid) {
	if (!mutex.lock()) throw cvgException("[SpeedEstimator] Cannot lock mutex");
	Vector3 speed = cameraSpeedFromWorld;
	if (isValid != NULL) (*isValid) =  measureIsValid;
	mutex.unlock();
	return speed;
}

Vector3 SpeedEstimator::getVisualSpeedInCamFrame(cvg_bool *isValid) {
	if (!mutex.lock()) throw cvgException("[SpeedEstimator] Cannot lock mutex");
	Vector3 speed = camSpeedFromCam;
	if (isValid != NULL) (*isValid) =  measureIsValid;
	mutex.unlock();
	return speed;
}

Vector3 SpeedEstimator::getVisualSpeedInPseudoWorldFrame(cvg_bool *isValid) {
	if (!mutex.lock()) throw cvgException("[SpeedEstimator] Cannot lock mutex");
	Vector3 speed = droneSpeedFromYawedWorld;
	if (isValid != NULL) (*isValid) =  measureIsValid;
	mutex.unlock();
	return speed;
}

Vector3 SpeedEstimator::getSpeedInPseudoWorldFrame() {
	return filteredSpeedInPseudoWorldFrame;
}

Vector3 SpeedEstimator::findLinePlaneIntersection(const Vector3 &lineVector, const Vector3 &planeNormal, const Vector3 &planeCenter) {
	double lambda = (planeCenter * planeNormal) / (lineVector * planeNormal);
	return lambda * lineVector;
}

void SpeedEstimator::enableDebug(cvg_bool e) {
#ifdef LOCAL_DEBUG
	if (!doDebug && e) {
		cvNamedWindow("SpeedEstimator");
	} else if (doDebug && !e) {
		cvDestroyWindow("SpeedEstimator");
	}
#endif
	doDebug = e;
}

void SpeedEstimator::gotEvent(EventGenerator &source, const Event &e) {
	const Video::GotVideoFrameEvent *vfe = dynamic_cast<const Video::GotVideoFrameEvent *>(&e);
	if (vfe == NULL) return;

	cvg_double elapsed = frameDividerTimer.getElapsedSeconds();
	if (elapsed < desiredFramePeriod) return;
//std::cout << "SpeedEstimator FPS: " << (1.0 / elapsed) << " (" << (1.0 / desiredFramePeriod) << ")" << std::endl;
	frameDividerTimer.restart();

	if (!frameMutex.lock()) throw cvgException("[SpeedEstimator] Cannot lock frame mutex");
	frameBuffer2.copy(vfe->frameData, vfe->frameLength);
	frameWidth2 = vfe->width;
	frameHeight2 = vfe->height;
	frameTime2 = vfe->timestamp;
	frameTicksPerSecond2 = vfe->ticksPerSecond;
	frameCondition.signal();
	frameMutex.unlock();
}

cvg_bool SpeedEstimator::getVideoFrame() {
#ifdef FRAME_RATE_LIMIT
	cvg_double elapsed = fpsLimitTimer.getElapsedSeconds();
	if (elapsed < (1.0 / FRAME_RATE_LIMIT)) return false;
	fpsLimitTimer.restart(true);
#endif

	if (!frameMutex.lock()) throw cvgException("[SpeedEstimator] Cannot lock frame mutex");
	cvg_bool result = frameCondition.wait(&frameMutex, 0.25);
	if (result) {
		frameBuffer = frameBuffer2;
		frameWidth = frameWidth2;
		frameHeight = frameHeight2;
		frameTime = frameTime2;
		frameTicksPerSecond = frameTicksPerSecond2;
/*
static cvgTimer debugTime;
static cvg_int debugAcum = 0;
debugAcum++;
std::cout << "SpeedEstimator fps: " << (debugAcum / debugTime.getElapsedSeconds()) << std::endl;
*/
	}
	frameMutex.unlock();
	return result;
}

Vector3 SpeedEstimator::RansacVector3::modelFromSamples(Vector3 *totalSampleSet, cvg_int numSamplesInSet, cvg_bool *isSampleInSubset) {
	Vector3 mean;
	cvg_int count = 0;
	for (cvg_int i = 0; i < numSamplesInSet; i++)
		if (isSampleInSubset[i]) {
			mean += totalSampleSet[i];
			count++;
		}
	return mean / count;
}

void SpeedEstimator::RansacVector3::generateSubset(Vector3 *sampleSet, cvg_int numSamplesInSet, cvg_int subsetLength, cvg_bool *isInSubset) {
	if (subsetLength >= numSamplesInSet) {
		for (cvg_int i = 0; i < numSamplesInSet; i++) sampleSet[i] = true;
		return;
	}
	bzero(isInSubset, numSamplesInSet * sizeof(cvg_bool));
	cvg_int nonSelectedSamples = numSamplesInSet;
	for (cvg_int i = 0; i < subsetLength; i++) {
		cvg_int chosenIndex = (nonSelectedSamples - 1) * (rand() / (cvg_float)RAND_MAX);
		cvg_int j = 0, elemCount = 0;
		while(j < numSamplesInSet) {
			if (!isInSubset[j]) {
				if (elemCount == chosenIndex) {
					isInSubset[j] = true;
					break;
				}
				elemCount++;
			}
			j++;
		}
		nonSelectedSamples--;
	}
}

cvg_bool SpeedEstimator::RansacVector3::isInlier(const Vector3 &point, const Vector3 &model) {
	cvg_double p_mod = point.modulus();
	cvg_double m_mod = model.modulus();
	cvg_bool angleCriterion = (point * model) / (p_mod * m_mod) > cos(30.0 * M_PI / 180.0);
	cvg_bool modulusCriterion = (fabs(m_mod - p_mod) / m_mod) < 10.0;
	return angleCriterion && modulusCriterion;
}

cvg_float SpeedEstimator::RansacVector3::inlierFitness(const Vector3 &point, const Vector3 &model) {
	cvg_double p_mod = point.modulus();
	cvg_double m_mod = model.modulus();
	cvg_float angleCriterion = (point * model) / (p_mod * m_mod);
	cvg_float modulusCriterion = 1.0 - fabs(m_mod - p_mod) / m_mod;
	if (angleCriterion < 0.0f || modulusCriterion < 0.0f) return 0.0;
	else return angleCriterion * modulusCriterion;
}

cvg_float SpeedEstimator::RansacVector3::modelFitness(Vector3 *set, cvg_int numSamplesInSet, cvg_bool *subset, const Vector3 &model) {
	cvg_float fitness = 0.0f;
	cvg_int count = 0;
	for (cvg_int i = 0; i < numSamplesInSet; i++) {
		if (subset[i]) {
			fitness += inlierFitness(set[i], model);
			count++;
		}
	}
	return count > 0 ? (fitness / count) : 0.0f;
}

cvg_int SpeedEstimator::RansacVector3::testOutliersWithSubModel(Vector3 *set, cvg_int numSamplesInSet, Vector3 subModel, cvg_bool *outputInliers) {
	cvg_int numInliers = 0;
	for (cvg_int i = 0; i < numSamplesInSet; i++) {
		if (!outputInliers[i] && isInlier(set[i], subModel)) {
			numInliers++;
			outputInliers[i] = true;
		}
	}
	return numInliers;
}

cvg_bool SpeedEstimator::RansacVector3::compute(Vector3 *sampleSet, cvg_int numSamplesInSet, Vector3 &bestModel, cvg_bool *isInlier) {
/*bestModel = Vector3();
for (cvg_int i = 0; i < numSamplesInSet; i++) {
	bestModel += sampleSet[i];
}
bestModel = bestModel / numSamplesInSet;
return true;
*/
	cvg_double p = 0.99;
	cvg_double epsilon = 0.5;
	cvg_double sRatio = 0.1; //0.35;
	cvg_int s = numSamplesInSet * sRatio;
	cvg_int minInliersToConsiderModel = numSamplesInSet * 0.6;
	cvg_int N = log(1 - p) / log(1 - pow(1 - epsilon, s));

	cvg_bool subset[numSamplesInSet];
	cvg_float bestModelFitness = -1.0f;
	for (cvg_int i = 0; i < N; i++) {
		// Generate initial group
		generateSubset(sampleSet, numSamplesInSet, s, subset);
		Vector3 subModel = modelFromSamples(sampleSet, numSamplesInSet, subset);

		// Test potential outliers against submodel and add those that fit well
		cvg_int totalInliers = testOutliersWithSubModel(sampleSet, numSamplesInSet, subModel, subset);
//std::cout << "model: (" << subModel.x << ", " << subModel.y << ", " << subModel.z << ") -> " << totalInliers << std::endl;
		if (totalInliers >= minInliersToConsiderModel) {
			// The model is good; check its performance
			cvg_float curModelFitness = modelFitness(sampleSet, numSamplesInSet, subset, subModel);
			if (curModelFitness > bestModelFitness) {
				bestModelFitness = curModelFitness;
				bestModel = subModel;
				minInliersToConsiderModel = totalInliers;	// Maximize consensus (test with and without this)
				memcpy(isInlier, subset, numSamplesInSet * sizeof(cvg_bool));

				// TODO: Think about it (outside if??)
				epsilon = 1 - (totalInliers / numSamplesInSet);
				cvg_double arg = 1 - pow(1 - epsilon, s);
				if (arg > 0) N = log(1 - p) / log(arg);
				else N = 0;
			}
		}
	}

	return bestModelFitness > 0.0f;
}

void SpeedEstimator::setAccelerations(cvg_double accelX, cvg_double accelY, cvg_double accelZ) {
	// Transform acceleration vector in drone frame to pseudo-world frame (yawed world)
	RotMatrix3 rot = RotMatrix3(Vector3(0, 1, 0), imuEulerZYX.y * M_PI / 180.0) * RotMatrix3(Vector3(1, 0, 0), imuEulerZYX.x * M_PI / 180.0);
	Vector3 localAccel = rot * Vector3(accelX, accelY, accelZ);
	if (!firstAccelInt) {
		cvg_double elapsed = accelTimer.getElapsedSeconds();
		accelTimer.restart(true);
		speedFromAccel += (localAccel + lastAccel) * 0.5 * elapsed;
//std::cout << "SfromA: (" << speedFromAccel.x << ", " << speedFromAccel.y << ", " << speedFromAccel.z << "); A: (" << localAccel.x << ", " << localAccel.y << ", " << localAccel.z << ")" << std::endl;
	} else {
		speedFromAccel = Vector3();
		firstAccelInt = false;
		lastAccel = localAccel;
	}
	cvg_bool visualSpeedValid;
	Vector3 visualSpeed = getVisualSpeedInPseudoWorldFrame(&visualSpeedValid);
	filteredSpeedInPseudoWorldFrame = Vector3(	compFilterX.filter(visualSpeed.x, speedFromAccel.x),
												compFilterY.filter(visualSpeed.y, speedFromAccel.y),
												0.0);
}

}
}
