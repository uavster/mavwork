/*
 * Vicon.cpp
 *
 *  Created on: 10/09/2011
 *      Author: Ignacio Mellado
 */

#include <stdio.h>
#include <Vicon.h>
#include <ViconClient.h>
#include <Timer.h>

namespace DroneProxy {
namespace Positioning {

using namespace ViconDataStreamSDK::CPP;
using namespace DroneProxy::Timing;

Vicon::Vicon() {
}

Vicon::~Vicon() {
}

void Vicon::open(const cvgString &host) {
	if (viconClient.IsConnected().Connected) return;

	// Connect to Vicon server
	if (viconClient.Connect(host).Result != Result::Success)
		throw cvgException("[Vicon::open] Cannot connect to Vicon server");

	// Enable some different data types
	if (viconClient.EnableSegmentData().Result != Result::Success)
		throw cvgException("[Vicon::open] Cannot enable segment data");

	// Set the streaming mode
	if (viconClient.SetStreamMode(StreamMode::ClientPull).Result != Result::Success)
		throw cvgException("[Vicon::open] Cannot set the stream mode");

	// Set the global axis mapping as Z-up
	if (viconClient.SetAxisMapping(Direction::Forward, Direction::Left, Direction::Up).Result != Result::Success)
		throw cvgException("[Vicon::open] Cannot set the axis mapping");
}

void Vicon::close() {
	if (viconClient.IsConnected().Connected)
		viconClient.Disconnect();
}

String Vicon::getDroneSegmentName(const cvgString &droneName) {
	// Pick the first segment of the subject
	Output_GetSegmentName outputGSN = viconClient.GetSegmentName(String(droneName), 0);
	if (outputGSN.Result != Result::Success)
		throw cvgException("[Vicon::getDroneSegmentName] Error getting segment name");
	return outputGSN.SegmentName;
}

RotMatrix3 Vicon::getDroneGlobalRotationMatrix(const cvgString &droneName) {
	Output_GetSegmentGlobalRotationMatrix outputGSGRM = viconClient.GetSegmentGlobalRotationMatrix(String(droneName), getDroneSegmentName(droneName));
	if (outputGSGRM.Result != Result::Success)
		throw cvgException("[Vicon::getDroneRotation] Error getting segment's global rotation matrix");
	return RotMatrix3(outputGSGRM.Rotation);
}

Vector3 Vicon::getDroneGlobalEulerXYZ(const cvgString &droneName) {
	Output_GetSegmentGlobalRotationEulerXYZ output = viconClient.GetSegmentGlobalRotationEulerXYZ(String(droneName), getDroneSegmentName(droneName));
	if (output.Result != Result::Success)
		throw cvgException("[Vicon::getDroneEulerXYZ] Error getting segment's global rotation Euler XYZ angles");
	return Vector3(output.Rotation);
}

Vector3 Vicon::getDroneGlobalPosition(const cvgString &droneName) {
	Output_GetSegmentGlobalTranslation outputGSGT = viconClient.GetSegmentGlobalTranslation(String(droneName), getDroneSegmentName(droneName));
	if (outputGSGT.Result != Result::Success)
		throw cvgException("[Vicon::getDronePosition] Error getting segment's global rotation matrix");
	return Vector3(outputGSGT.Translation);
}

HTMatrix4 Vicon::getDroneGlobalTransform(const cvgString &droneName) {
	return HTMatrix4(getDroneGlobalRotationMatrix(droneName), getDroneGlobalPosition(droneName));
}

cvg_bool Vicon::requestUpdate() {
	Output_GetFrame output = viconClient.GetFrame();
	return output.Result == Result::Success;
}

Vector4 Vicon::getDroneGlobalQuaternion(const cvgString &droneName) {
	Output_GetSegmentGlobalRotationQuaternion output = viconClient.GetSegmentGlobalRotationQuaternion(String(droneName), getDroneSegmentName(droneName));
	if (output.Result != Result::Success)
		throw cvgException("[Vicon::getDroneQuaternion] Error getting segment's global rotation quaternion");
	return Vector4(output.Rotation);

}

cvg_bool Vicon::isConnected() {
	return viconClient.IsConnected().Connected;
}

HTMatrix4 Vicon::calibrateDroneLocalFrame(const cvgString &droneName, double calibrationSeconds, cvg_int minSamples, double rotStdThreshold, double transStdThreshold) {
	if (calibrationSeconds < 0.0) calibrationSeconds = VICON_DEFAULT_CALIBRATION_SECONDS;
	if (minSamples < 0) minSamples = VICON_DEFAULT_CALIBRATION_MINSAMPLES;
	if (rotStdThreshold < 0.0) rotStdThreshold = VICON_DEFAULT_CALIBRATION_ROTSTDTHRESHOLD;
	if (transStdThreshold < 0.0) transStdThreshold = VICON_DEFAULT_CALIBRATION_TRANSSTDTHRESHOLD;
	// Monitor location stability: if there is too much noise, the calibration cannot finish
	HTMatrix4 drone0FromVicon_mean = HTMatrix4::zero();
	std::vector<HTMatrix4> drone0FromVicon;
	Timer timer;
	cvg_uint numSamples = 0;
	while(timer.getElapsedSeconds() < calibrationSeconds) {
		if (requestUpdate()) {
			HTMatrix4 m = getDroneGlobalTransform(droneName);
			drone0FromVicon.push_back(m);
			drone0FromVicon_mean = drone0FromVicon_mean + m;
			numSamples++;
		};
		usleep(20000);
	}
	if (numSamples < minSamples) throw cvgException("[Vicon::calibrateDroneLocalFrame] Could not get enough samples to calibrate (" + cvgString(numSamples) + " samples)");

	// Calculate mean
	drone0FromVicon_mean = drone0FromVicon_mean / numSamples;
	// Calculate standard deviation
	HTMatrix4 drone0FromVicon_std = HTMatrix4::zero();
	for (std::vector<HTMatrix4>::iterator it = drone0FromVicon.begin(); it != drone0FromVicon.end(); it++) {
		drone0FromVicon_std = drone0FromVicon_std + (((*it) - drone0FromVicon_mean).scalarPow(2));
	}
	drone0FromVicon_std = (drone0FromVicon_std / numSamples).scalarSqrt();

	// If the standard deviation is higher than a threshold for any value in the matrix, calibration cannot be done
	// Different thresholds are used for the rotation and the translation
	if ((drone0FromVicon_std.getRotationMatrix() > rotStdThreshold).countValues(0.0) < 3*3 ||
		(drone0FromVicon_std.getTranslation() > transStdThreshold).countValues(0.0) < 3)
		throw cvgException("[Vicon::calibrate] The standard deviation of the samples is too high");

	// As it is an HT matrix, the inverse is the transpose
	if (!calibrationMutex.lock()) throw cvgException("[Vicon] Cannot lock calibration mutex");

	try {
		DroneInfo di;
		di.viconFromDroneLocal = drone0FromVicon_mean.inverse();
		di.droneLocalFromVicon_stdev = drone0FromVicon_std;
		droneInfo[droneName] = di;
	} catch(cvgException e) {
		calibrationMutex.unlock();
		throw e;
	}
	calibrationMutex.unlock();

	return drone0FromVicon_std;
}

HTMatrix4 Vicon::getDroneLocalTransform(const cvgString &droneName) {
	if (!calibrationMutex.lock()) throw cvgException("[Vicon] Cannot lock calibration mutex");

	std::map<cvgString, DroneInfo>::iterator it = droneInfo.find(droneName);
	if (it != droneInfo.end()) {
		calibrationMutex.unlock();
		return it->second.viconFromDroneLocal * getDroneGlobalTransform(droneName);
	} else {
		calibrationMutex.unlock();
		throw cvgException("[Vicon::getDroneLocalTransform] The drone " + droneName + " was not calibrated");
	}
}

cvg_bool Vicon::isCalibrated(const cvgString &droneName) {
	if (!calibrationMutex.lock()) throw cvgException("[Vicon] Cannot lock calibration mutex");
	std::map<cvgString, DroneInfo>::iterator it = droneInfo.find(droneName);
	cvg_bool result = it != droneInfo.end();
	calibrationMutex.unlock();
	return result;
}

}
}
