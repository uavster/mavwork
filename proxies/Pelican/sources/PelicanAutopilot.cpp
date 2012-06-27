/*
 * Autopilot.cpp
 *
 *  Created on: 11/05/2012
 *      Author: Ignacio Mellado-Bataller
 */

#include <iostream>
#include "mav/pelican/PelicanAutopilot.h"
#include "mav/pelican/serial/packets.h"

//#define EMERGENCY_MODE_ENABLE

#define TAKEOFF_SPEED				0.3		// [m/s]
#define LANDING_SPEED				0.4		// [m/s]
#define TAKEOFF_FINAL_ALTITUDE		0.5		// [m]
#define MAX_ALTITUDE				4.5		// [m]
#define ALTITUDE_FOR_LANDING_WITH_TIME	0.30	// [m]
#define LANDING_WITH_TIME_MAX_SECONDS	1.0	// [s]
#define LANDING_WITH_TIME_FIXED_THRUST	0.5
#define MAX_ALTITUDE_DESCENDING_SPEED	0.2 // [m/s]

#define MAX_PELICAN_EULER_ANGLE		32.1429	// [deg] Estimation from experimental flights
#define MAX_ALLOWED_EULER_ANGLE		9.0

#define MAX_PELICAN_YAW_SPEED		100.0	// [deg/s]
#define MAX_ALLOWED_YAW_SPEED		100.0	// [deg/s]
#define MAX_ALLOWED_ALTITUDE_SPEED	0.5		// [m/s]

#define BATTERY_NOMINAL_VOLTAGE		12.4
#define MIN_SAFE_BATT_VOLTAGE		10.0

#define DEFAULT_SERIALPORT	(0+16)	//"/dev/ttyUSB0"
#define DEFAULT_BAUDRATE	115200	//BAUD_57600
//#define DEFAULT_DATABITS	DATA8
//#define DEFAULT_PARITY		P_NONE
//#define DEFAULT_STOPBITS	STOP1
//#define DEFAULT_FLOWCTRL	FLOWOFF
#define DEFAULT_SERIALTIMEOUT		0.5

#define writeData	serial.write
#define readData	serial.read

namespace Mav {

using namespace Pelican;

PelicanAutopilot::PelicanAutopilot() {
	readAverage = writeAverage = 0.0;
	rCount = wCount = 0;

	commandSendOk = true;
	firstSample = true;
	droneMode = LANDED;
	landingWithTime = false;
}

PelicanAutopilot::~PelicanAutopilot() {
	close();
}

void PelicanAutopilot::open() {
	speedEstimator.open();
	addEventListener(&speedEstimator);
//	speedEstimator.enableDebug(true);
	altitudeSensor.open();
	if (!serial.open(DEFAULT_SERIALPORT, DEFAULT_BAUDRATE, DEFAULT_SERIALTIMEOUT))
    	throw cvgException("[PelicanAutopilot] Unable to initialize serial interface");

	commandSendOk = true;
	firstSample = true;
	landingWithTime = false;
}

void PelicanAutopilot::close() {
	serial.close();
	std::cout << "Read avg.: " << (readAverage / rCount) << ", write avg.: " << (writeAverage / wCount) << std::endl;

	removeAllEventListeners();
	speedEstimator.close();
	altitudeSensor.close();
}

#define KEEP_IN_RANGE(a, min, max) if (a < min) a = min; else if (a > max) a = max;

cvg_bool PelicanAutopilot::setCommandData(const CommandData &cd1) {
	// Wait for a first-time state reception
	if (firstSample) return true;

	static cvgTimer scd_time;
	cvg_double e = scd_time.getElapsedSeconds();
	scd_time.restart();
//	std::cout << "setCommandData() " << e << std::endl;
	writeAverage += e;
	wCount++;
//	usleep(5000);
//	return true;

	// Command copy and saturation
	CommandData cd = cd1;
	KEEP_IN_RANGE(cd.phi, -1.0, 1.0)
	KEEP_IN_RANGE(cd.theta, -1.0, 1.0)
	KEEP_IN_RANGE(cd.gaz, -1.0, 1.0)
	KEEP_IN_RANGE(cd.yaw, -1.0, 1.0)

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
			if (lastStateInfo.altitude <= -fabs(TAKEOFF_FINAL_ALTITUDE))
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
			if (cd.flyingMode == TAKEOFF)
				droneMode = TAKINGOFF;
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
		altSpeedController.enable(false);
		cd.theta = lastStateInfo.pitch / MAX_ALLOWED_EULER_ANGLE;
		cd.phi = lastStateInfo.roll / MAX_ALLOWED_EULER_ANGLE;
		cd.yaw = 0;		// TODO: when yaw speed is estimated en getStateInfo(), set this value to the last estimated yaw speed
		cd.gaz = 0;
		break;
#endif
	case FLYING:
		hoverController.enable(false);
		if (-lastStateInfo.altitude > fabs(MAX_ALTITUDE))
			cd.gaz = -fabs(MAX_ALTITUDE_DESCENDING_SPEED / MAX_ALLOWED_ALTITUDE_SPEED);
		break;
	case LANDED:
		altSpeedController.enable(false);
		hoverController.enable(false);
		cd.gaz = 0;
		cd.theta = lastStateInfo.pitch / MAX_ALLOWED_EULER_ANGLE;
		cd.phi = lastStateInfo.roll / MAX_ALLOWED_EULER_ANGLE;
		cd.yaw = 0;		// TODO: when yaw speed is estimated en getStateInfo(), set this value to the last estimated yaw speed
		break;
	case TAKINGOFF:
		altSpeedController.enable(true);
//		hoverController.enable(true);
//		hoverController.run(lastStateInfo.speedX, lastStateInfo.speedY, &cd.theta, &cd.phi);
		cd.gaz = fabs(TAKEOFF_SPEED / MAX_ALLOWED_ALTITUDE_SPEED);
		// TODO: pitch and roll are controlled by the speed controller
		cd.theta = 0;	// TODO
		cd.phi = 0;		// TODO
		cd.yaw = 0;		// This is the yaw speed
		break;
	case HOVERING:
		altSpeedController.enable(true);
		hoverController.enable(true);
		if (isLastSpeedMeasureValid) {
			hoverController.run(lastStateInfo.speedX, lastStateInfo.speedY, &cd.theta, &cd.phi);
			cd.theta /= MAX_PELICAN_EULER_ANGLE;
			cd.phi /= MAX_PELICAN_EULER_ANGLE;
			// Undo final transform
			cd.theta *= (MAX_PELICAN_EULER_ANGLE / MAX_ALLOWED_EULER_ANGLE);
			cd.phi *= (MAX_PELICAN_EULER_ANGLE / MAX_ALLOWED_EULER_ANGLE);
		} else {
			cd.theta = cd.phi = 0.0f;
		}
		cd.gaz = 0;
		break;
	case LANDING:
		altSpeedController.enable(true);
//		hoverController.enable(true);
//		hoverController.run(lastStateInfo.speedX, lastStateInfo.speedY, &cd.theta, &cd.phi);
		cd.gaz = -fabs(LANDING_SPEED / MAX_ALLOWED_ALTITUDE_SPEED);
		// TODO: pitch and roll are controlled by the speed controller
		cd.theta = 0;	// TODO
		cd.phi = 0;		// TODO
		cd.yaw = 0;		// This is the yaw speed
		break;
	}

	char buffer[sizeof(HEADER_COMMAND) + sizeof(NAUTICAL) + sizeof(unsigned short)];
	HEADER_COMMAND *header = (HEADER_COMMAND *)buffer;
	header->startString[0] = '#';
	header->startString[1] = '>';
	header->type = 'N';
	NAUTICAL *nautical = (NAUTICAL *)&buffer[sizeof(HEADER_COMMAND)];

	nautical->control = NAUTICAL_THRUST | NAUTICAL_YAW | NAUTICAL_PITCH | NAUTICAL_ROLL;

	// Convert euler angles from ZYX
	// The IMU gives euler angles in ZYX order. Let's assume the commands are accepted in the same format.
	// However, the proxy reference frame X and Y axes are inverted in relation to the Pelican frame: 180 deg. rotation around the Z axis
	nautical->pitch = -cd.theta * 2047 * (MAX_ALLOWED_EULER_ANGLE / MAX_PELICAN_EULER_ANGLE);
	nautical->roll = -cd.phi * 2047 * (MAX_ALLOWED_EULER_ANGLE / MAX_PELICAN_EULER_ANGLE);
	nautical->yaw = -cd.yaw * 2047 * (MAX_ALLOWED_YAW_SPEED / MAX_PELICAN_YAW_SPEED);

	altSpeedController.setReference(cd.gaz * MAX_ALLOWED_ALTITUDE_SPEED);
	if (!landingWithTime)
		nautical->thrust = altSpeedController.getOutput() * 4095;
	else
		nautical->thrust = LANDING_WITH_TIME_FIXED_THRUST * 4095;

	unsigned short *checksum = (unsigned short *)&buffer[sizeof(HEADER_COMMAND) + sizeof(NAUTICAL)];
	for (cvg_int i = 0; i < sizeof(buffer) - sizeof(unsigned short); i++)
		(*checksum) += buffer[i];
	(*checksum) += 0xAAAA;

	commandSendOk = writeData(buffer, sizeof(buffer));

	return commandSendOk;
}

cvg_bool PelicanAutopilot::getStateInfo(StateInfo &si) {
	static cvgTimer gsi_time;
	cvg_double e = gsi_time.getElapsedSeconds();
	gsi_time.restart();
//	std::cout << "getStateInfo() " << e << std::endl;
	readAverage += e;
	rCount++;
//	usleep(8000);
//	return true;

	si.commWithDrone = commandSendOk ? 1 : 0;
	if (!si.commWithDrone) return false;

	si.droneMode = droneMode;

	char cmdState[] = "$>b";
	si.commWithDrone = writeData(cmdState, sizeof(cmdState) - 1) ? (char)1 : (char)0;
	if (!si.commWithDrone) return false;

	cvg_ulong elapsed = timer.getElapsedSeconds() * 1e6;
	si.timeCodeH = (elapsed >> 32) & 0xFFFFFFFF;
	si.timeCodeL = elapsed & 0xFFFFFFFF;

	char respState[sizeof(HEADER) + sizeof(STATE) + sizeof(FOOTER)];
	si.commWithDrone = readData(respState, sizeof(respState)) ? (char)1 : (char)0;
	if (!si.commWithDrone) return false;
	HEADER *header = (HEADER *)&respState;
	STATE *state = (STATE *)&respState[sizeof(HEADER)];
	si.commWithDrone = (header->id == 'b') ? 1 : 0;
	if (!si.commWithDrone) return false;

	//	speedX += state->x_acc * 0.001 * 9.81;
	//	speedY += state->y_acc * 0.001 * 9.81;

	// Pelican gives angles in XYZ order; convert them to ZYX
	// XYZ axes in Pelican are equivalent to (-X)(-Y)Z in the proxy
	RotMatrix3 rotation = 	RotMatrix3(Vector3(0, 0, 1), state->yaw_ang * 0.01f * M_PI / 180.0) *
							RotMatrix3(Vector3(0, 1, 0), state->pitch_ang * 0.01f * M_PI / 180.0) *
							RotMatrix3(Vector3(1, 0, 0), state->roll_ang * 0.01f * M_PI / 180.0) *
							RotMatrix3(Vector3(0, 0, 1), M_PI);

	Vector3 eulerZYX = rotation.getEulerAnglesZYX();
	si.yaw = eulerZYX.z * 180.0 / M_PI;		// [deg] (In this line the 0 deg. reading means south)
	si.pitch = eulerZYX.y * 180.0 / M_PI;	// [deg]
	si.roll = eulerZYX.x * 180.0 / M_PI;	// [deg]
	// For the proxy protocol, 0 deg. means "initial position"
	if (!firstSample) {
		si.yaw = mapAngleToMinusPlusPi(((si.yaw + 360.0f) - yawOffset) * M_PI / 180.0) * 180.0 / M_PI;
	} else {
		yawOffset = si.yaw;
		firstSample = false;
		si.yaw = 0.0f;
	}
	double altitude = altitudeSensor.getAltitude();
	speedEstimator.updateCamera(altitude, Vector3(si.roll, si.pitch, si.yaw));

	si.nativeDroneState = state->flight_mode;
	si.altitude = -altitude; //-state->height * 0.001;	// [m] The proxy frame Z axis points down (higher = more negative)
	altSpeedController.updateAltitude(altitude, MAX_ALLOWED_ALTITUDE_SPEED);

	Vector3 speed = speedEstimator.getSpeedInPseudoWorldFrame(&isLastSpeedMeasureValid); //speedEstimator.getSpeedInWorldFrame(); // speedEstimator.getSpeedInCamFrame();
	si.speedX = speed.x;	// [m/s]
	si.speedY = speed.y;	// [m/s]
	si.speedYaw = state->yaw_vel * 0.015f;	// [deg/s] 0.015 deg/s per unit, according to Asctec documentation

	char cmdSensor[] = "$>c";
	si.commWithDrone = writeData(cmdSensor, sizeof(cmdSensor) - 1) ? (char)1 : (char)0;
	if (!si.commWithDrone) return false;

	char respSensor[sizeof(HEADER) + sizeof(SENSOR) + sizeof(FOOTER)];
	si.commWithDrone = readData(respSensor, sizeof(respSensor)) ? (char)1 : (char)0;
	if (!si.commWithDrone) return false;
	SENSOR *sensor = (SENSOR *)&respSensor[sizeof(HEADER)];

	si.batteryLevel = (sensor->battery * 0.001f - MIN_SAFE_BATT_VOLTAGE) / (BATTERY_NOMINAL_VOLTAGE - MIN_SAFE_BATT_VOLTAGE);	// [0, 1]
	if (si.batteryLevel > 1.0f) si.batteryLevel = 1.0f;
	else if (si.batteryLevel < 0.0f) si.batteryLevel = 0.0f;

	lastStateInfo = si;

	return true;
}

cvg_double PelicanAutopilot::mapAngleToMinusPlusPi(cvg_double angleRads) {
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

void PelicanAutopilot::gotEvent(EventGenerator &source, const Event &e) {
//std::cout << "PelicanAutopilot::gotEvent : " << typeid(e).name() << std::endl;
	notifyEvent(e);
}

void PelicanAutopilot::connectionLost(cvg_bool cl) {
	if (cl) {
		CommandData command;
		command.flyingMode = HOVERING;
		command.yaw = 0.0;
		setCommandData(command);
	}
}

}
