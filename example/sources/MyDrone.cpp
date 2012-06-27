/*
 * MyDrone.cpp
 *
 *  Created on: 05/09/2011
 *      Author: Ignacio Mellado
 */

#include <MyDrone.h>
#include <cv.h>
#include <highgui.h>

using namespace DroneProxy;

MyDrone::MyDrone() {
	cvStartWindowThread();
	cvNamedWindow("test");
	frame = NULL;

	followTarget = false;
	autoMode = false;
	forwardSpeed = 0.0;

	pidYaw.enableMaxOutput(true, 1.0);
	pidForwardSpeed.enableMaxOutput(true, 1.0);
	pidHeight.enableMaxOutput(true, 1.0);
	pidDrift.enableMaxOutput(true, 1.0);

	// Kp = 1/Ts, Normalized Kp = 1/Ts / (MAX_YAW_SPEED * M_PI / 180)
	pidYaw.setGains(1.0, 0.0, 0.0);

	// Kp = 1/Ts, Normalized Kp = 1/Ts / (MAX_HEIGHT_SPEED)
	pidHeight.setGains(1.0, 0.0, 0.0);

	pidDrift.setGains(0.25, 0.25, 0.01);
	pidDrift.enableAntiWindup(true, 1.0);

	pidForwardSpeed.setGains(0.25, 0.25, 0.01);
	pidForwardSpeed.enableAntiWindup(true, 1.0);

	target = Vector3(0,0,0);

	pidDrift.setReference(0.0);

	positioningSource = POSITIONING_ODOMETRY;

	trajectory = NULL;
}

MyDrone::~MyDrone() {
	close();

	cvReleaseImageHeader(&frame);
	frame = NULL;
	cvDestroyWindow("test");
}

void MyDrone::processVideoFrame(cvg_int cameraId, cvg_ulong timeCode, VideoFormat format, cvg_uint width, cvg_uint height, cvg_char *frameData) {
	if (cameraId == 0) {
		if (frame == NULL) {
			frame = cvCreateImageHeader(cvSize(width, height), IPL_DEPTH_8U, 3);
		}
		cvSetData(frame, frameData, width * 3);
		cvShowImage("test", frame);
	}

	// Save data to log
	LoggerDrone::processVideoFrame(cameraId, timeCode, format, width, height, frameData);
}

void MyDrone::processFeedback(FeedbackData *feedbackData) {	
	// Save data to log (including Vicon)
	LoggerDrone::processFeedback(feedbackData);

	if (!autoMode || feedbackData->droneMode != FLYING || getControlMode() != MOVE) {
		resetPIDs();
		return;
	}

	if (!positioningMutex.lock()) throw cvgException("Cannot lock positioning mutex");

	Vector3 globalPosition;
	Vector3 localSpeed, globalSpeed;

	ViconData vd;
	getViconData(&vd);

	if (positioningSource == POSITIONING_VICON) {
		globalPosition = vd.globalPosition * 1e-3;
		localSpeed = vd.localSpeed * 1e-3;
		globalSpeed = vd.globalSpeed * 1e-3;
	} else {
		globalPosition = Vector3(vd.globalPosition.x * 1e-3, vd.globalPosition.y * 1e-3, feedbackData->altitude);
		localSpeed = Vector3(feedbackData->speedX, feedbackData->speedY, localSpeed.z * 1e-3);
		globalSpeed = vd.globalSpeed * 1e-3;
	}
	positioningMutex.unlock();

	if (trajectoryMutex.lock()) {
		if (this->trajectory != NULL) {
			Trajectory::Waypoint wp = this->trajectory->update(globalPosition, globalSpeed);
			goToWaypoint(wp.position);
		}
		trajectoryMutex.unlock();
	}

	// Estimate (either from the odometry or the Vicon):
	// 		Current position from inertial frame
	// 		Speed from drone's local frame
	// 		Current yaw from drone's local frame
	// 		Current altitude from intertial frame

	// Feed measures to PIDs
	pidYaw.setFeedback(0.0); //feedbackData->yaw * M_PI / 180.0);
	pidForwardSpeed.setFeedback(localSpeed.x);
	pidDrift.setFeedback(localSpeed.y);
	pidHeight.setFeedback(globalPosition.z);

	if (!targetMutex.lock()) throw cvgException("Cannot lock target mutex");

	if (!followTarget) {
		target = globalPosition;
		forwardSpeed = 0.0;
		followTarget = true;
	}

	// Calculate new references for the controllers:
	// 		Altitude reference is the target Z
	// 		Yaw reference is the angle of the (target - current_position) in the drone's local XY plane
	// 		Forward speed reference is set externally as is
	//		Lateral speed (drift) is always 0.0
	pidHeight.setReference(target.z);
	pidDrift.setReference(0.0);
	pidForwardSpeed.setReference(forwardSpeed);
	Vector3 trajectory = target - globalPosition;
	if (sqrt(trajectory.x * trajectory.x + trajectory.y * trajectory.y) >= 0.1) {
		Vector3 pointer = ((HTMatrix4)vd.globalTransform.inverse()).getRotationMatrix() * trajectory;
		pidYaw.setReference(atan2(pointer.y, pointer.x));
	}

	// Send PIDs' outputs to the drone
	setControlData(	Comm::ControlChannel::THETA | Comm::ControlChannel::PHI | Comm::ControlChannel::YAW | Comm::ControlChannel::GAZ,
					pidDrift.getOutput(),
					-pidForwardSpeed.getOutput(),
					pidHeight.getOutput(),
					pidYaw.getOutput());

	targetMutex.unlock();
}

void MyDrone::setYaw(cvg_double yawRads) {
	cvg_double yawMapped = mapAngleToMinusPlusPi(yawRads);
	if (!targetMutex.lock()) throw cvgException("Cannot lock yaw mutex");
	pidYaw.setReference(yawMapped);
	targetMutex.unlock();
}

void MyDrone::setForwardSpeed(cvg_double speed) {
	if (!targetMutex.lock()) throw cvgException("Cannot lock yaw mutex");
	pidForwardSpeed.setReference(speed);
	targetMutex.unlock();
}

void MyDrone::setAltitude(cvg_double altitude) {
	if (!targetMutex.lock()) throw cvgException("Cannot lock yaw mutex");
	pidHeight.setReference(altitude);
	targetMutex.unlock();
}

cvg_double MyDrone::mapAngleToMinusPlusPi(cvg_double angleRads) {
	cvg_double mapped = fmod(angleRads, M_PI);
	cvg_int turns = (cvg_int)floor(angleRads / M_PI);
	if (turns & 1) {
		if (turns > 0)
			mapped -= M_PI;
		else
			mapped += M_PI;
	}
	return mapped;
}

void MyDrone::resetPIDs() {
	pidYaw.reset();
	pidForwardSpeed.reset();
	pidDrift.reset();
	pidHeight.reset();
}

void MyDrone::setSpeed(cvg_double fs) {
	if (targetMutex.lock()) {
		forwardSpeed = fs;
		targetMutex.unlock();
	}
}

void MyDrone::goToWaypoint(const Vector3 &target) {
	if (targetMutex.lock()) {
		cvg_bool changed = this->target != target;
		this->target = target;
		followTarget = true;
		targetMutex.unlock();
		if (changed) log(cvgString("Heading to waypoint (") + target.x + ", " + target.y + ", " + target.z + ")");
	}
}
void MyDrone::stopWhereYouAre() {
	if (targetMutex.lock()) {
		followTarget = false;
		targetMutex.unlock();
	}
}

void MyDrone::setControlMode(FlyingMode m) {
	if (m != MOVE) {
		followTarget = false;
	}
	LoggerDrone::setControlMode(m);
}

void MyDrone::accelerate(cvg_double a) {
	if (targetMutex.lock()) {
		forwardSpeed += a;
		targetMutex.unlock();
	}
}

void MyDrone::setPositioningSource(PositioningSource ps) {
	if (positioningMutex.lock()) {
		positioningSource = ps;
		positioningMutex.unlock();
	}
}

MyDrone::PositioningSource MyDrone::getPositioningSource() {
	PositioningSource ps = POSITIONING_ODOMETRY;
	if (positioningMutex.lock()) {
		ps = positioningSource;
		positioningMutex.unlock();
	}
	return ps;
}

void MyDrone::setConfigParams() {
	LedsAnimation anim;
	anim.typeId = LEDS_RED_SNAKE;
	anim.frequencyHz = 2.0f;
	anim.durationSec = 0;
	writeParam(CONFIGPARAM_LEDS_ANIMATION, (cvg_char *)&anim, sizeof(LedsAnimation));
	writeParam(CONFIGPARAM_MAX_EULER_ANGLES, 2.0f);
	writeParam(CONFIGPARAM_VIDEO_ENCODING_TYPE, JPEG);
	writeParam(CONFIGPARAM_VIDEO_ENCODING_QUALITY, 1.0f);
}
