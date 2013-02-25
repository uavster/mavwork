/*
 * SpeedEstimator.h
 *
 *  Created on: 25/05/2012
 *      Author: Ignacio Mellado-Bataller
 */

#ifndef SPEEDESTIMATOR_H_
#define SPEEDESTIMATOR_H_

#include "OpticalFlow.h"
#include <atlante.h>
#include <cv.h>
#include "../../cam/CameraModel.h"
#include "../../base/EventListener.h"
#include "../../base/EventGenerator.h"
#include "../../data/Buffer.h"
#include "../../control/CompFilter1.h"
#include "../../control/Filter.h"

namespace Mav {
namespace Linkquad {

class SpeedEstimator : public virtual cvgThread, public virtual EventListener, public virtual EventGenerator {
	friend class LinkquadAutopilot;
private:
	OpticalFlow optFlow;
	Video::CameraModel cameraModel;
	IplImage *image, *scaledImage;

	cvg_int frameWidth, frameHeight, frameWidth2, frameHeight2;
	cvg_ulong frameTime, frameTime2;
	cvg_ulong frameTicksPerSecond, frameTicksPerSecond2;
	DynamicBuffer<char> frameBuffer, frameBuffer2;

	cvgMutex mutex;
	cvg_bool measureIsValid;
	Vector3 cameraSpeedFromWorld, camSpeedFromCam, droneSpeedFromYawedWorld;

	cvgMutex inputMutex;
	cvg_double imuAltitude, lastImuAltitude;
	Vector3 imuEulerZYX, lastImuEulerZYX;

	cvg_double lastScalingFactor, lastClippingFactor;

	cvgTimer timer;
	cvg_double lastFrameTime;

	volatile cvg_bool doDebug;

	cvgCondition frameCondition;
	cvgMutex frameMutex;

	volatile cvg_float desiredFramePeriod;
	cvg_double lastFrameNotificationTime;
	cvgTimer frameDividerTimer;

	Filter altitudeFilter;

	class RansacVector3 {
	public:
		Vector3 modelFromSamples(Vector3 *samples, cvg_int numSamples, cvg_bool *isSampleInSubset);
		void generateSubset(Vector3 *sampleSet, cvg_int numSamplesInSet, cvg_int subsetLength, cvg_bool *isSampleInSubset);
		cvg_int testOutliersWithSubModel(Vector3 *set, cvg_int numSamplesInSet, Vector3 subModel, cvg_bool *outputInliers);
		cvg_bool isInlier(const Vector3 &point, const Vector3 &model);
		cvg_float inlierFitness(const Vector3 &point, const Vector3 &model);
		cvg_float modelFitness(Vector3 *set, cvg_int numSamplesInSet, cvg_bool *subset, const Vector3 &model);
	public:
		cvg_bool compute(Vector3 *sampleSet, cvg_int numSamplesInSet, Vector3 &bestModel, cvg_bool *isInlier);
	};
	RansacVector3 ransac;

	cvgTimer accelTimer;
	cvg_bool firstAccelInt;
	Vector3 speedFromAccel, lastAccel;

	CompFilter1 compFilterX, compFilterY;
	Vector3 filteredSpeedInPseudoWorldFrame;

	cvgTimer fpsLimitTimer;

protected:
	Vector3 findLinePlaneIntersection(const Vector3 &lineVector, const Vector3 &planeNormal, const Vector3 &planeCenter);
	void run();
	cvg_bool getVideoFrame();
	virtual void gotEvent(EventGenerator &source, const Event &e);

public:
	SpeedEstimator();
	virtual ~SpeedEstimator() {}

	void open(cvg_float accelSamplingFreq);
	void close();

	void updateCamera(cvg_double altitude, const Vector3 &eulerZYX);
	Vector3 getVisualSpeedInWorldFrame(cvg_bool *isValid);
	Vector3 getVisualSpeedInCamFrame(cvg_bool *isValid);
	Vector3 getVisualSpeedInPseudoWorldFrame(cvg_bool *isValid);

	Vector3 getSpeedInPseudoWorldFrame();

	void setAccelerations(cvg_double accelX, cvg_double accelY, cvg_double accelZ);

	void enableDebug(cvg_bool e);
};

}
}

#endif /* SPEEDESTIMATOR_H_ */
