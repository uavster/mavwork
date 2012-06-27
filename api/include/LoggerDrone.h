/*
 * LoggerDrone.h
 *
 *  Created on: 05/09/2011
 *      Author: Ignacio Mellado
 */

#ifndef LOGGERDRONE_H_
#define LOGGERDRONE_H_

#include <cv.h>

#include "Drone.h"
#include "LogConsumer.h"
#include "Vicon.h"
#include "DataWriter.h"

#define LOGGERDRONE_MAX_CAMERAS_IN_LOG			32
#define LOGGERDRONE_GATHERED_DATA_BASE_PATH		"logs"

namespace DroneProxy {

class LoggerDrone : public virtual Drone, public Logs::LogConsumer {
private:
	Positioning::Vicon vicon;
	cvgString viconHost, viconDroneName;

	Logs::DataWriter dataWriter;
	cvg_uint gatheringSources;
	Threading::Mutex gatheringConfigMutex;
	cvg_uint frameIndex[LOGGERDRONE_MAX_CAMERAS_IN_LOG];

	volatile cvg_uint poseEstimationDataSources;

public:
	class ViconData {
	public:
		cvg_ulong timeCode;
		Vector3 globalPosition;
		Vector3 localSpeed, globalSpeed;
		HTMatrix4 localTransform, globalTransform;

		inline ViconData &operator = (const ViconData &vd) {
			timeCode = vd.timeCode;
			globalPosition = vd.globalPosition;
			localSpeed = vd.localSpeed;
			localTransform = vd.localTransform;
			globalTransform = vd.globalTransform;
			globalSpeed = vd.globalSpeed;
			return *this;
		}
	};

private:
	ViconData lastViconData;
	Threading::Mutex viconMutex;
	cvg_bool gotAnyViconPosition;

public:
	typedef enum { ODOMETRY = 0x1, VIDEO = 0x2, VICON = 0x4, COMMANDS = 0x8 } DataSource;

protected:
	cvg_bool canGatherFromSource(DataSource source);
	inline cvg_bool isSourceUsedForPoseEstimation(DataSource source) { return poseEstimationDataSources & source; }
	inline cvg_bool isViconNeeded() { return isSourceUsedForPoseEstimation(VICON) || canGatherFromSource(VICON); }
	void manageViconStatus();
	void logCommands();

public:

	LoggerDrone();
	~LoggerDrone();

	void open(const cvgString &droneHost, AccessMode accessMode = COMMANDER, cvg_uchar priority = 0, const cvgString &viconHost = "", const cvgString &viconDroneName = "");
	void close();

	void logDataFrom(cvg_uint dataSources);
	inline void estimatePoseWith(cvg_uint dataSources) {
		poseEstimationDataSources = dataSources;
		manageViconStatus();
	}

	cvg_bool getViconData(ViconData *vd);

	inline virtual void logConsume(Logs::LogProducer *producer, const cvgString &str) { log(str); }

	// This methods are overriden to log commands if needed
	cvg_bool setControlData(cvg_int changes, cvg_float phi, cvg_float theta, cvg_float gaz, cvg_float yawSpeed);
	cvg_bool setControlData(cvg_float phi, cvg_float theta, cvg_float gaz, cvg_float yawSpeed);
	cvg_bool setControlMode(FlyingMode m);

	inline HTMatrix4 calibrateVicon() {
		if (vicon.isConnected()) return vicon.calibrateDroneLocalFrame(viconDroneName);
		else throw cvgException("[LoggerDrone::calibrateVicon] Vicon is not connected");
	}

	inline Positioning::Vicon &getVicon() { return vicon; }

protected:
	virtual void processVideoFrame(cvg_int cameraId, cvg_ulong timeCode, VideoFormat format, cvg_uint width, cvg_uint height, cvg_char *frameData);
	virtual void processFeedback(FeedbackData *feebackData);
};

}

#endif /* LOGGERDRONE_H_ */
