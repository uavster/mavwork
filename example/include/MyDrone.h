/*
 * MyDrone.h
 *
 *  Created on: 05/09/2011
 *      Author: Ignacio Mellado
 */

#ifndef MYDRONE_H_
#define MYDRONE_H_

#include <droneproxy.h>
#include <cv.h>
#include "PID.h"
#include <atlante.h>
#include "Trajectory.h"

class MyDrone : public virtual DroneProxy::LoggerDrone {
private:
	PID pidYaw, pidForwardSpeed, pidDrift, pidHeight;
	DroneProxy::Threading::Mutex targetMutex, positioningMutex, trajectoryMutex;

	IplImage *frame;

	cvg_double forwardSpeed;
	cvg_bool gotTarget;
	Vector3 target;
	cvg_bool followTarget;

	volatile cvg_bool autoMode;

	Trajectory *trajectory;

protected:
	cvg_double mapAngleToMinusPlusPi(cvg_double angleRads);
	void resetPIDs();

public:
	MyDrone();
	~MyDrone();

	void setYaw(cvg_double yaw);
	void setForwardSpeed(cvg_double speed);
	void setAltitude(cvg_double altitude);

	inline void setAuto(cvg_bool enable = true) { autoMode = enable; }
	inline void setManual(cvg_bool enable = true) { setAuto(!enable); }

	void setSpeed(cvg_double fs);
	void accelerate(cvg_double speedInc);
	void goToWaypoint(const Vector3 &target);
	void stopWhereYouAre();

	inline void setTrajectory(Trajectory *traj) {
		if (trajectoryMutex.lock()) {
			trajectory = traj;
			trajectoryMutex.unlock();
		}
	}
	inline Trajectory *getTrajectory() {
		Trajectory *tr;
		if (trajectoryMutex.lock()) {
			tr = trajectory;
			trajectoryMutex.unlock();
		}
		return tr;
	}

	typedef enum { POSITIONING_VICON, POSITIONING_ODOMETRY } PositioningSource ;
	void setPositioningSource(PositioningSource ps);
	PositioningSource getPositioningSource();

	// Overriden from LoggerDrone
	void setControlMode(FlyingMode m);

protected:
	virtual void processVideoFrame(cvg_int cameraId, cvg_ulong timeCode, VideoFormat format, cvg_uint width, cvg_uint height, cvg_char *frameData);
	virtual void processFeedback(FeedbackData *feebackData);
	virtual void setConfigParams();

private:
	PositioningSource positioningSource;
};

#endif /* MYDRONE_H_ */
