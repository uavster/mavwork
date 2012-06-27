/*
 * Vicon.h
 *
 *  Created on: 10/09/2011
 *      Author: Ignacio Mellado
 */

#ifndef VICON_H_
#define VICON_H_

#include <atlante.h>
#include "ViconClient.h"
#include <map>
#include "Mutex.h"

#define VICON_DEFAULT_CALIBRATION_SECONDS			2.0
#define VICON_DEFAULT_CALIBRATION_MINSAMPLES		50
#define VICON_DEFAULT_CALIBRATION_ROTSTDTHRESHOLD	0.1
#define VICON_DEFAULT_CALIBRATION_TRANSSTDTHRESHOLD	1.0

namespace DroneProxy {
namespace Positioning {

class Vicon {
private:
	ViconDataStreamSDK::CPP::Client	viconClient;

	class DroneInfo {
	public:
		HTMatrix4 viconFromDroneLocal;
		HTMatrix4 droneLocalFromVicon_stdev;

		inline DroneInfo() { }
		inline DroneInfo(const DroneInfo &di) {
			(*this) = di;
		}
		inline const DroneInfo &operator = (const DroneInfo &di) {
			viconFromDroneLocal = di.viconFromDroneLocal;
			droneLocalFromVicon_stdev = di.droneLocalFromVicon_stdev;
			return *this;
		}
	};
	std::map<cvgString, DroneInfo> droneInfo;

	DroneProxy::Threading::Mutex calibrationMutex;

protected:
	ViconDataStreamSDK::CPP::String getDroneSegmentName(const cvgString &droneName);

public:
	Vicon();
	virtual ~Vicon();

	void open(const cvgString &host);
	void close();

	cvg_bool isConnected();

	HTMatrix4 calibrateDroneLocalFrame(	const cvgString &droneName,
									double calibrationSeconds = VICON_DEFAULT_CALIBRATION_SECONDS,
									cvg_int minSamples = VICON_DEFAULT_CALIBRATION_MINSAMPLES,
									double rotStdThreshold = VICON_DEFAULT_CALIBRATION_ROTSTDTHRESHOLD,
									double transStdThreshold = VICON_DEFAULT_CALIBRATION_TRANSSTDTHRESHOLD
									);

	cvg_bool requestUpdate();

	RotMatrix3 getDroneGlobalRotationMatrix(const cvgString &droneName);
	Vector3 getDroneGlobalEulerXYZ(const cvgString &droneName);
	Vector4 getDroneGlobalQuaternion(const cvgString &droneName);
	Vector3 getDroneGlobalPosition(const cvgString &droneName);
	HTMatrix4 getDroneGlobalTransform(const cvgString &droneName);

	HTMatrix4 getDroneLocalTransform(const cvgString &droneName);

	cvg_bool isCalibrated(const cvgString &droneName);
};

}
}

#endif /* VICON_H_ */
