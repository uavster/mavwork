#include <iostream>

#include "mav/linkquad/Autopilot.h"
#include <link_api.h>

// ---- Tunable ----
#define DEFAULT_FEEDBACK_RATE			60.0	// [Hz]

#define CONTROLLER_THREAD_MIN_FREQ		60.0	// [Hz]

#define USE_ACCELEROMETERS_FOR_SPEED_ESTIMATION

//#define EMERGENCY_MODE_ENABLE

#define ON_GROUND_ALTITUDE				0.32

#define TAKEOFF_SPEED					1.0		// [m/s]
#define TAKEOFF_FINAL_ALTITUDE			1.1		// 1.3		// [m]
#define TAKINGOFF_ALT_SPEED_COS_GAMMA	2.15	// 2.5
#define MAX_TAKINGOFF_TIME_S			5.0		// [s]
//#define TAKINGOFF_ALT_SPEED_GAMMA		0.8

#define LANDING_SPEED					0.4		// [m/s]
#define ALTITUDE_FOR_LANDING_WITH_TIME	0.40	// [m]
#define LANDING_WITH_TIME_MAX_SECONDS	1.0	// [s]
#define LANDING_WITH_TIME_FIXED_THRUST	0.5

#define MAX_ALTITUDE					4.5		// [m]
#define MAX_ALTITUDE_DESCENDING_SPEED	0.2 // [m/s]
#define MAX_ALTITUDE_TO_CONSIDER_UNKNOWN_STATE_AS_LANDED	ALTITUDE_FOR_LANDING_WITH_TIME	// [m]

#define MAX_ALLOWED_EULER_ANGLE			27.0		// [deg]

#define MAX_ALLOWED_YAW_SPEED			100.0	// [deg/s]
#define MAX_ALLOWED_ALTITUDE_SPEED		0.5		// [m/s]

#define BATTERY_NOMINAL_VOLTAGE			12.4	// [V]
#define MIN_SAFE_BATT_VOLTAGE			10.0	// [V]

#define US_SENSOR_DISTANCE_TO_IMU		0.095	// [m]

#define TRIMMING_FILE_NAME				"autopilot_trims.cfg"
// ----

// ---- System defined ----
#define MAXIMUM_CONTROL_MCU_FB_RATE		250
#define MAXIMUM_SENSOR_MCU_FB_RATE		500

#define CONTROL_MCU_SERIAL_PORT			"/dev/ttyS2"

// Feedback ids
#define ID_TIME							1
#define ID_YAW							5
#define ID_PITCH						4
#define ID_ROLL							3
#define ID_YAW_RATE						12
#define ID_SONAR_VOLTAGE				92	// ADC0:91, ADC1: 92, ADC2: 93
#define ID_BATTERY_VOLTAGE				74
#define ID_FLIGHT_MODE					84
#define ID_SYSTEM_STATUS				89
#define ID_ACCEL_X						16
#define ID_ACCEL_Y						17
#define ID_ACCEL_Z						18

// Command ids
#define ID_CMD_YAW_RATE					2
#define ID_CMD_PITCH					0
#define ID_CMD_ROLL						1
#define ID_CMD_ALTITUDE_RATE			3

// ----

namespace Mav {
namespace  Linkquad {

LinkquadAutopilot::LinkquadAutopilot() : cvgThread("Autopilot") {
	isOpen = false;
	firstSample = true;
	droneMode = UNKNOWN;
	landingWithTime = false;
}

void LinkquadAutopilot::open() {
	close();

	loadTrimFromFile(TRIMMING_FILE_NAME);

	feedbackFrequency = DEFAULT_FEEDBACK_RATE;
	isLastSpeedMeasureValid = false;
	lastStateInfo_bck.speedX = lastStateInfo_bck.speedY = 0.0f;

	speedEstimator.open(DEFAULT_FEEDBACK_RATE);
	addEventListener(&speedEstimator);
	altitudeSensor.open();

	yawController.setMaxAllowedOutput(MAX_ALLOWED_YAW_SPEED);

	// Init API
	lb_init();

	// Connect to MCUs
	if (lb_connectToControlMCU((char *)CONTROL_MCU_SERIAL_PORT) != LB_OK)
		throw cvgException("[LinkquadAutopilot] Cannot connect to control MCU");

	isOpen = true;
	// Request data IDs and frequency
	setFeedbackRate(DEFAULT_FEEDBACK_RATE);
	isOpen = false;

	firstSample = true;
	trimming = false;
	landingWithTime = false;

	// Add callback
	if (lb_subscribeControlMCUData(&controlMCUCallback, this) != LB_OK)
		throw cvgException("[LinkquadAutopilot] Cannot subscribe to control MCU");

	droneMode = UNKNOWN;
	lastCommand.flyingMode = INIT;
	// In case the initial mode is determined as HOVERING, set yaw and altitude rate to 0
	lastCommand.yaw = lastCommand.gaz = lastCommand.phi = lastCommand.theta = 0.0f;
	start();

	isOpen = true;
}

void LinkquadAutopilot::close() {
	if (!isOpen) return;

	stop();

	// Request 0 variables
	uint8_t controlIds;
	lb_requestFromControlMCU(&controlIds, 0, (int)(MAXIMUM_CONTROL_MCU_FB_RATE / feedbackFrequency));

	removeAllEventListeners();
	speedEstimator.close();
	altitudeSensor.close();

	isOpen = false;
}

void LinkquadAutopilot::setFeedbackRate(cvg_double pf) {
	if (!isOpen) return;

	// Specify feedback data IDs to get from control MCU
	uint8_t controlIds[SCMCUDATA_IDS_COUNT];

	uint8_t controlIdCount = 0;

//	controlIds[controlIdCount++] = ID_TIME;
	controlIds[controlIdCount++] = ID_YAW;
	controlIds[controlIdCount++] = ID_PITCH;
	controlIds[controlIdCount++] = ID_ROLL;
	controlIds[controlIdCount++] = ID_YAW_RATE;
	controlIds[controlIdCount++] = ID_SONAR_VOLTAGE;
	controlIds[controlIdCount++] = ID_BATTERY_VOLTAGE;
//	controlIds[controlIdCount++] = ID_FLIGHT_MODE;
	controlIds[controlIdCount++] = ID_SYSTEM_STATUS;

#ifdef USE_ACCELEROMETERS_FOR_SPEED_ESTIMATION
	controlIds[controlIdCount++] = ID_ACCEL_X;
	controlIds[controlIdCount++] = ID_ACCEL_Y;
	controlIds[controlIdCount++] = ID_ACCEL_Z;
#endif

	// Request data IDs with some rate from control MCU
	feedbackFrequency = pf;
	if (lb_requestFromControlMCU(controlIds, controlIdCount, (int)(MAXIMUM_CONTROL_MCU_FB_RATE / pf)) != LB_OK)
		throw cvgException("[LinkquadAutopilot] Cannot request data from control MCU");

}

#define KEEP_IN_RANGE(a, min, max) if (a < min) a = min; else if (a > max) a = max;

void LinkquadAutopilot::setCommandData(const CommandData &cd1) {
	if (!isOpen) return;

	// Keep commands in range and notify the controller thread
	if (controllerThreadMutex.lock()) {
		lastCommand = cd1;
		KEEP_IN_RANGE(lastCommand.phi, -1.0, 1.0)
		KEEP_IN_RANGE(lastCommand.theta, -1.0, 1.0)
		KEEP_IN_RANGE(lastCommand.gaz, -1.0, 1.0)
		KEEP_IN_RANGE(lastCommand.yaw, -1.0, 1.0)

		controllerThreadCondition.signal();
		controllerThreadMutex.unlock();
	}
}

void LinkquadAutopilot::controlMCUCallback(void *data, void *userData) {
	cvg_ulong timestamp = (cvg_ulong)(cvgTimer::getSystemSeconds() * 1e6);

	SCMCUData *d = (SCMCUData *)data;
	LinkquadAutopilot *o = (LinkquadAutopilot *)userData;

	// Fill our packet up with converted data
	StateInfo *i = &o->feedbackEvent.stateInfo;
	i->timeCodeH = (timestamp >> 32) & 0xFFFFFFFF;
	i->timeCodeL = timestamp & 0xFFFFFFFF;
	i->commWithDrone = 1;	// If no state updates by the autopilot after a long time,
							// the navigation server will send a packet with this flag
							// set to 0 through notifyLongTimeSinceLastStateInfoUpdate()
	i->nativeDroneState = d->system_status;	// TODO: maybe flight mode?

	if (!o->firstSample && !o->trimming) {
		i->yaw = o->mapAngleToMinusPlusPi((d->psi * 0.1f + 360.0 - o->yawOffset) * M_PI / 180.0) * 180.0 / M_PI;
		i->pitch = d->theta * 0.1f - o->pitchOffset;
		i->roll = d->phi * 0.1f - o->rollOffset;
	} else {
		o->yawOffset = d->psi * 0.1f;
		i->yaw = 0.0f;
		i->pitch = d->theta * 0.1f - o->pitchOffset;
		i->roll = d->phi * 0.1f - o->rollOffset;
		if (o->trimming) {
			o->pitchOffset = d->theta * 0.1f;
			o->rollOffset = d->phi * 0.1f;
			o->saveTrimToFile(TRIMMING_FILE_NAME);
			i->pitch = 0.0f;
			i->roll = 0.0f;
			o->trimming = false;
		}
	}
	i->speedYaw = d->psi_dot * 0.01f;
	i->altitude = -(o->altitudeSensor.getAltitude(d->adc_1) + US_SENSOR_DISTANCE_TO_IMU);

	o->speedEstimator.updateCamera(-i->altitude, Vector3(i->roll, i->pitch, i->yaw));
#ifdef USE_ACCELEROMETERS_FOR_SPEED_ESTIMATION
	o->speedEstimator.setAccelerations(d->accel_x_raw * 0.0001 * 9.81, d->accel_y_raw * 0.0001 * 9.81, d->accel_z_raw * 0.0001 * 9.81);
#endif

	o->altSpeedController.updateAltitude(-i->altitude, MAX_ALLOWED_ALTITUDE_SPEED);

	// Battery: INPUT: 0.001 V/unit -> OUTPUT: [0, 1] where 0=empty, 1=full
	i->batteryLevel = (d->battery_voltage * 0.001f - MIN_SAFE_BATT_VOLTAGE) / (BATTERY_NOMINAL_VOLTAGE - MIN_SAFE_BATT_VOLTAGE);
	if (i->batteryLevel > 1.0f) i->batteryLevel = 1.0f;
	else if (i->batteryLevel < 0.0f) i->batteryLevel = 0.0f;

	// Forward and lateral speeds
	Vector3 speed = o->speedEstimator.getSpeedInPseudoWorldFrame(); //&o->isLastSpeedMeasureValid);
	o->isLastSpeedMeasureValid = true;

 	i->speedX = speed.x;
	i->speedY = speed.y;

	cvg_bool visualIsValid;
	Vector3 visualSpeed = o->speedEstimator.getVisualSpeedInPseudoWorldFrame(&visualIsValid);

	// TODO: remove this when tested
/*	i->userData[0] = visualIsValid ? visualSpeed.x : 1e6;
	i->userData[1] = visualIsValid ? visualSpeed.y : 1e6;
	i->userData[2] = o->curThrust;*/
	// TODO: -----------------------

	if (o->controllerThreadMutex.lock()) {
		if (o->droneMode == UNKNOWN) {
			// This is the the first time feedback and mode must be determined. Are we landed or flying?
			// If we are flying, make it hover for safety.
			if (-i->altitude <= MAX_ALTITUDE_TO_CONSIDER_UNKNOWN_STATE_AS_LANDED) o->droneMode = LANDED;
			else o->droneMode = HOVERING;
		}

		i->droneMode = o->droneMode;
		if (!o->isLastSpeedMeasureValid) {
			i->speedX = o->lastStateInfo_bck.speedX;
			i->speedY = o->lastStateInfo_bck.speedY;
		}
		o->lastStateInfo_bck = *i;

		o->firstSample = false;

		o->controllerThreadCondition.signal();
		o->controllerThreadMutex.unlock();
	}

	// Notify listeners
	o->notifyEvent(o->feedbackEvent);
}

void LinkquadAutopilot::notifyLongTimeSinceLastStateInfoUpdate(StateInfo *si) {
	si->commWithDrone = 0;
}

cvg_double LinkquadAutopilot::mapAngleToMinusPlusPi(cvg_double angleRads) {
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

void LinkquadAutopilot::gotEvent(EventGenerator &source, const Event &e) {
//std::cout << "LinkquadAutopilot::gotEvent : " << typeid(e).name() << std::endl;
	notifyEvent(e);
}

/*
 * This method is run asynchronously to update the controllers continuously
 */
void LinkquadAutopilot::run() {
	if (!isOpen || firstSample) { usleep(100000); return; }

	// Wait until the thread is waken up by an event. Otherwise, it'll wake up periodically.
	if (!controllerThreadMutex.lock()) throw cvgException("[LinkquadAutopilot] Unable to lock controller thread mutex");
	controllerThreadCondition.wait(&controllerThreadMutex, 1.0 / CONTROLLER_THREAD_MIN_FREQ);
	CommandData cd = this->lastCommand;
	StateInfo lastStateInfo = this->lastStateInfo_bck;
	DroneMode droneMode = this->droneMode;
	controllerThreadMutex.unlock();

	// Mode change
#ifdef EMERGENCY_MODE_ENABLE
	if (cd.flyingMode == EMERGENCYSTOP)
		droneMode = EMERGENCY;
	else {
#endif
		switch(droneMode) {
		case FLYING:
			if (cd.flyingMode == HOVER)
				droneMode = HOVERING;
			else if (cd.flyingMode == LAND)
				droneMode = LANDING;
			break;
		case TAKINGOFF:
			if (	lastStateInfo.altitude <= -fabs(TAKEOFF_FINAL_ALTITUDE) ||
					takingoffTimer.getElapsedSeconds() >= MAX_TAKINGOFF_TIME_S
				)
				droneMode = HOVERING;
			if (cd.flyingMode == LAND)
				droneMode = LANDING;
			else if (cd.flyingMode == HOVER)
				droneMode = HOVERING;
			break;
		case HOVERING:
			if (cd.flyingMode == MOVE)
				droneMode = FLYING;
			else if (cd.flyingMode == LAND)
				droneMode = LANDING;
			break;
		case LANDED:
			if (cd.flyingMode == TAKEOFF) {
				droneMode = TAKINGOFF;
				takingoffTimer.restart(false);
			}
			break;
		case LANDING:
			if (!landingWithTime) {
				if (-lastStateInfo.altitude < fabs(ALTITUDE_FOR_LANDING_WITH_TIME)) {
					landingWithTime = true;
					landingTimer.restart(false);
				}
			} else {
				if (landingTimer.getElapsedSeconds() > LANDING_WITH_TIME_MAX_SECONDS) {
					droneMode = LANDED;
					landingWithTime = false;
				}
			}
			if (cd.flyingMode == TAKEOFF) {
				droneMode = TAKINGOFF;
				takingoffTimer.restart(false);
				landingWithTime = false;
			}
			break;
		}
#ifdef EMERGENCY_MODE_ENABLE
	}
#endif

	// Mode commands
	switch(droneMode) {
#ifdef EMERGENCY_MODE_ENABLE
	case EMERGENCY:
		hoverController.enable(false);
		altSpeedController.enable(false);
		yawController.enable(false);
		cd.theta = lastStateInfo.pitch / MAX_ALLOWED_EULER_ANGLE;
		cd.phi = lastStateInfo.roll / MAX_ALLOWED_EULER_ANGLE;
		cd.yaw = 0;		// TODO: when yaw speed is estimated en getStateInfo(), set this value to the last estimated yaw speed
		cd.gaz = 0;
		break;
#endif
	case FLYING:
		hoverController.enable(false);
		altSpeedController.enable(true);
		yawController.enable(true);
		if (-lastStateInfo.altitude > fabs(MAX_ALTITUDE))
			cd.gaz = -fabs(MAX_ALTITUDE_DESCENDING_SPEED / MAX_ALLOWED_ALTITUDE_SPEED);
		break;
	case LANDED:
		altSpeedController.enable(false);
		hoverController.enable(false);
		yawController.enable(false);
		cd.gaz = 0;
		cd.theta = lastStateInfo.pitch / MAX_ALLOWED_EULER_ANGLE;
		cd.phi = lastStateInfo.roll / MAX_ALLOWED_EULER_ANGLE;
		cd.yaw = 0;		// TODO: when yaw speed is estimated en getStateInfo(), set this value to the last estimated yaw speed
		break;
	case TAKINGOFF:
		altSpeedController.enable(true);
		hoverController.enable(true);
		yawController.enable(true);
		if (isLastSpeedMeasureValid) {
			hoverController.run(lastStateInfo.speedX, lastStateInfo.speedY, -lastStateInfo.altitude, &cd.theta, &cd.phi);
			cd.theta /= MAX_ALLOWED_EULER_ANGLE;
			cd.phi /= MAX_ALLOWED_EULER_ANGLE;
		} else {
			cd.theta = cd.phi = 0.0f;
		}
		if (-lastStateInfo.altitude < fabs(TAKEOFF_FINAL_ALTITUDE)) {
			if (-lastStateInfo.altitude > ON_GROUND_ALTITUDE) {
				cvg_double altFactor = ((lastStateInfo.altitude + ON_GROUND_ALTITUDE) / (-TAKEOFF_FINAL_ALTITUDE + ON_GROUND_ALTITUDE));
//				cd.gaz = TAKEOFF_SPEED * (1.0 - pow(altFactor, TAKINGOFF_ALT_SPEED_GAMMA)) / MAX_ALLOWED_ALTITUDE_SPEED;
				cvg_double altFactorCos = 1.0 - 0.5 * (1 + cos(altFactor * M_PI));
				cd.gaz = TAKEOFF_SPEED * pow(1.0 -  altFactorCos, TAKINGOFF_ALT_SPEED_COS_GAMMA) / MAX_ALLOWED_ALTITUDE_SPEED;
			} else
				cd.gaz = TAKEOFF_SPEED / MAX_ALLOWED_ALTITUDE_SPEED;
		} else
			cd.gaz = 0.0;
		cd.yaw = 0;
		break;
	case HOVERING:
		altSpeedController.enable(true);
		hoverController.enable(true);
		yawController.enable(true);
		if (isLastSpeedMeasureValid) {
			hoverController.run(lastStateInfo.speedX, lastStateInfo.speedY, -lastStateInfo.altitude, &cd.theta, &cd.phi);
			cd.theta /= MAX_ALLOWED_EULER_ANGLE;
			cd.phi /= MAX_ALLOWED_EULER_ANGLE;
		} else {
			cd.theta = cd.phi = 0.0f;
		}
		// Alt. rate is as commanded by the app
//		cd.gaz = 0;
		break;
	case LANDING:
		altSpeedController.enable(true);
		hoverController.enable(true);
		yawController.enable(true);
		if (isLastSpeedMeasureValid) {
			hoverController.run(lastStateInfo.speedX, lastStateInfo.speedY, -lastStateInfo.altitude, &cd.theta, &cd.phi);
			cd.theta /= MAX_ALLOWED_EULER_ANGLE;
			cd.phi /= MAX_ALLOWED_EULER_ANGLE;
		} else {
			cd.theta = cd.phi = 0.0f;
		}
		cd.gaz = -fabs(LANDING_SPEED / MAX_ALLOWED_ALTITUDE_SPEED);
		cd.yaw = 0;		// This is the yaw speed
		break;
	}

	altSpeedController.setReference(cd.gaz * MAX_ALLOWED_ALTITUDE_SPEED);
	cvg_float thrust;
	if (!landingWithTime)
		thrust = altSpeedController.getOutput() * 1000;
	else
		thrust = LANDING_WITH_TIME_FIXED_THRUST * 1000;

	cvg_float yawRate;
	if (yawController.isEnabled()) {
		// If there's a recent measure, feed it back to the control loop
		yawController.setFeedback(lastStateInfo.yaw);
		// What is the expected yaw position, according to the commanded yaw rate?
		cvg_double elapsed = yawRateTimer.getElapsedSeconds();
		yawRateTimer.restart(true);
		cvg_double newYawRef = yawController.getReference() + elapsed * (cd.yaw * MAX_ALLOWED_YAW_SPEED);
		yawController.setReference(newYawRef);
		yawRate = yawController.getOutput() / MAX_ALLOWED_YAW_SPEED;
	} else {
		yawRateTimer.restart(false);
		yawRate = cd.yaw;
		yawController.setReference(lastStateInfo.yaw);
	}

//char str[512];
//sprintf(str, "y: %.2f, p: %.2f (%.2f), r: %.2f (%.2f), t: %.2f\n", cd.yaw * MAX_ALLOWED_YAW_SPEED, cd.theta * MAX_ALLOWED_EULER_ANGLE, lastStateInfo.speedX, cd.phi * MAX_ALLOWED_EULER_ANGLE, lastStateInfo.speedY, thrust);
//std::cout << str;

	// TODO: Remove this after testing
	curThrust = thrust;
	// TODO: -------------------------

	// Set up packet for lqapi and send it
	if (	lb_userParamsSetFloat(ID_CMD_PITCH, cd.theta * MAX_ALLOWED_EULER_ANGLE, 0) != LB_OK ||
			lb_userParamsSetFloat(ID_CMD_ROLL, cd.phi * MAX_ALLOWED_EULER_ANGLE, 0) != LB_OK ||
			lb_userParamsSetFloat(ID_CMD_YAW_RATE, -yawRate * MAX_ALLOWED_YAW_SPEED, 0) != LB_OK ||
			lb_userParamsSetFloat(ID_CMD_ALTITUDE_RATE, thrust, 0) != LB_OK ||
			lb_sendUserParams(0) != LB_OK
		)
		throw cvgException("[LinkquadAutopilot] Cannot send command data to MCU");

	// Update object's drone mode after local drone mode was updated
	if (!controllerThreadMutex.lock()) throw cvgException("[LinkquadAutopilot] Unable to lock controller thread mutex");
	this->droneMode = droneMode;
	controllerThreadMutex.unlock();

}

void LinkquadAutopilot::saveTrimToFile(const cvgString &fileName) {
	FILE *f = fopen(fileName.c_str(), "w");
	if (f != NULL) {
		fprintf(f, "%f %f", pitchOffset, rollOffset);
		fclose(f);
	}
}

void LinkquadAutopilot::loadTrimFromFile(const cvgString &fileName) {
	FILE *f = fopen(fileName.c_str(), "r");
	if (f != NULL) {
		if (fscanf(f, "%f %f", &pitchOffset, &rollOffset) != 2) {
			fclose(f);
			throw cvgException("[LinkquadAutopilot] Cannot read the saved trimmings from file \"" + fileName + "\"");
		}
		fclose(f);
	} else {
		pitchOffset = rollOffset = 0.0f;
	}
}

}
}
