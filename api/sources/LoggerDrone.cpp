/*
 * LoggerDrone.cpp
 *
 *  Created on: 05/09/2011
 *      Author: Ignacio Mellado
 */

#include <LoggerDrone.h>
#include <cv.h>
#include <highgui.h>

using namespace DroneProxy;
using namespace DroneProxy::Logs;

LoggerDrone::LoggerDrone() {
	dataWriter.addLogConsumer(this);
	gatheringSources = 0;
	poseEstimationDataSources = ODOMETRY | COMMANDS;
	gotAnyViconPosition = false;
}

LoggerDrone::~LoggerDrone() {
	close();
	dataWriter.removeLogConsumer(this);
}

void LoggerDrone::processVideoFrame(cvg_int cameraId, cvg_ulong timeCode, VideoFormat format, cvg_uint width, cvg_uint height, cvg_char *frameData) {
	// If enabled, gather the video frame event
	if (gatheringConfigMutex.lock()) {
		if (canGatherFromSource(VIDEO)) {
			if (cameraId >= 0 && cameraId < LOGGERDRONE_MAX_CAMERAS_IN_LOG)
				dataWriter.addEvent(new DataWriter::VideoFrameEvent(timeCode, cvgString("cam") + cameraId + "_frame" + (frameIndex[cameraId]++) + ".png", width, height, frameData));
		}
		gatheringConfigMutex.unlock();
	}
	Drone::processVideoFrame(cameraId, timeCode, format, width, height, frameData);
}

void LoggerDrone::processFeedback(FeedbackData *feedbackData) {
	if (gatheringConfigMutex.lock()) {
		try {
			cvg_ulong timeCode = (((cvg_ulong)feedbackData->timeCodeH) << 32) | (cvg_ulong)feedbackData->timeCodeL;
			// If enabled, gather the drone information event
			if (canGatherFromSource(ODOMETRY))
				dataWriter.addEvent(new DataWriter::DroneInfoEvent(timeCode, feedbackData));

			// If enabled, gather the Vicon event
			timeval tv;
			gettimeofday(&tv, NULL);
			timeCode = (cvg_ulong)tv.tv_sec * 1e6 + tv.tv_usec;
			if (canGatherFromSource(VICON) && vicon.isConnected() && vicon.isCalibrated(viconDroneName)) {
				if (vicon.requestUpdate()) {
					Vector3 localSpeed;
					Vector3 currentGlobalPosition = vicon.getDroneGlobalPosition(viconDroneName);
					Vector3 globalSpeed;
					if (gotAnyViconPosition) {
						cvg_double elapsedSeconds = ((cvg_double)(timeCode - lastViconData.timeCode)) / 1e6;
						globalSpeed = (currentGlobalPosition - lastViconData.globalPosition) / elapsedSeconds;
						localSpeed = ((HTMatrix4)vicon.getDroneGlobalTransform(viconDroneName).inverse()).getRotationMatrix() * globalSpeed;
					} else {
						localSpeed = Vector3(0.0, 0.0, 0.0);
						globalSpeed = localSpeed;
					}

					if (viconMutex.lock()) {
						lastViconData.timeCode = timeCode;
						lastViconData.globalPosition = currentGlobalPosition;
						lastViconData.localTransform = vicon.getDroneLocalTransform(viconDroneName);
						lastViconData.globalTransform = vicon.getDroneGlobalTransform(viconDroneName);
						lastViconData.localSpeed = localSpeed;
						lastViconData.globalSpeed = globalSpeed;
						gotAnyViconPosition = true;
						viconMutex.unlock();
					}

					dataWriter.addEvent(new DataWriter::ViconDataEvent(timeCode, lastViconData.localTransform, localSpeed));
				}
			}
		} catch(cvgException e) {
			log(e.getMessage());
		}
		gatheringConfigMutex.unlock();
	}
}

void LoggerDrone::open(const cvgString &droneHost, AccessMode accessMode, cvg_uchar priority, const cvgString &viconHost, const cvgString &viconDroneName) {
	close();
	log("Opening drone at " + droneHost + "...");
	memset(frameIndex, 0, sizeof(frameIndex));
	Drone::open(droneHost, accessMode, priority);
	this->viconHost = viconHost;
	this->viconDroneName = viconDroneName;
	gotAnyViconPosition = false;
}

void LoggerDrone::close() {
	if (isOpen()) {
		log("Closing drone at " + getHost() + "...");
		Drone::close();
	}
	if (vicon.isConnected()) {
		log("Closing Vicon at " + viconHost + "...");
		vicon.close();
	}
}

void LoggerDrone::manageViconStatus() {
	if (isViconNeeded()) {
		// Start Vicon if it is needed and it's not running yet
		if (!vicon.isConnected()) {
			if (viconHost != "") {
				log("Opening Vicon at " + viconHost + "...");
				vicon.open(viconHost);
				log("Vicon at " + viconHost + " is ready. Remember to calibrate it.");
			}
			else
				throw cvgException("[LoggerDrone::manageViconStatus] Vicon is needed as a data source but the host is unknown. Please specify the Vicon host when you call LoggerDrone::open().");
		}
	} else {
		// If it is not needed and still running, close it
		if (vicon.isConnected()) {
			log("[LoggerDrone::manageViconStatus] Vicon is no more needed and it will be closed");
			vicon.close();
		}
	}
}

void LoggerDrone::logDataFrom(cvg_uint dataSources) {
	if (gatheringConfigMutex.lock()) {
		cvg_uint gatheringSources_backup;
		try {
			if (gatheringSources != dataSources) {
				gatheringSources_backup = gatheringSources;
				gatheringSources = dataSources;
				// Log data sources have changed and the Vicon status might need an update
				manageViconStatus();
				if (dataSources == 0) {
					dataWriter.destroy();
				} else {
					// Start the data writer if it is not running yet
					if (!dataWriter.isStarted()) {
						time_t rawtime;
						time(&rawtime);
						struct tm *timeInfo = localtime(&rawtime);
						cvgString timeStr = asctime(timeInfo);
						dataWriter.create(cvgString(LOGGERDRONE_GATHERED_DATA_BASE_PATH) + '/' + (timeStr - '\n'));
					}
				}
			}
		} catch(cvgException e) {
			// If something went wrong, keep sources like before
			gatheringSources = gatheringSources_backup;
			gatheringConfigMutex.unlock();
			throw e;
		}
		gatheringConfigMutex.unlock();
	} else throw cvgException("[LoggerDrone::logDataFrom] Unable to lock gatheringConfigMutex");
}

cvg_bool LoggerDrone::canGatherFromSource(DataSource source) {
	return gatheringSources & source;
}

void LoggerDrone::logCommands() {
	timeval tv;
	gettimeofday(&tv, NULL);
	cvg_ulong timeCode = tv.tv_usec + tv.tv_sec * 1e6;
	if (gatheringConfigMutex.lock()) {
		CommandPacket commandPacket;
		getChannelManager().getControlChannel().getControlData(&commandPacket);
		// If enabled, gather the drone information event
		if (canGatherFromSource(COMMANDS))
			dataWriter.addEvent(new DataWriter::CommandsEvent(timeCode, &commandPacket));
		gatheringConfigMutex.unlock();
	}
}

cvg_bool LoggerDrone::setControlData(cvg_int changes, cvg_float phi, cvg_float theta, cvg_float gaz, cvg_float yawSpeed) {
	cvg_bool result = Drone::setControlData(changes, phi, theta, gaz, yawSpeed);
	// Add a commands event to the data writer
	logCommands();
	return result;
}

cvg_bool LoggerDrone::setControlData(cvg_float phi, cvg_float theta, cvg_float gaz, cvg_float yawSpeed) {
	cvg_bool result = Drone::setControlData(phi, theta, gaz, yawSpeed);
	// Add a commands event to the data writer
	logCommands();
	return result;
}

cvg_bool LoggerDrone::setControlMode(FlyingMode m) {
	cvg_bool result = Drone::setControlMode(m);
	// Add a commands event to the data writer
	logCommands();
	return result;
}

cvg_bool LoggerDrone::getViconData(ViconData *vd) {
	if (!viconMutex.lock()) return false;
	cvg_bool result = gotAnyViconPosition;
	if (result)
		(*vd) = lastViconData;
	viconMutex.unlock();
	return result;
}
