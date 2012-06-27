/*
 * DataWriter.cpp
 *
 *  Created on: 12/09/2011
 *      Author: Ignacio Mellado
 */

#include <DataWriter.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>
#include <cv.h>
#include <highgui.h>

#define EVENTS_FILE_NAME	"events.txt"
#define FRAMES_DIRECTORY	"video"

namespace DroneProxy {
namespace Logs {

DataWriter::DataWriter()
	: cvgThread("dataWriter") {
	openLog("DataWriter");
	eventsFile = NULL;
}

DataWriter::~DataWriter() {
	destroy();
	closeLog();
}

void DataWriter::create(const cvgString &baseFolder) {
	destroy();
	basePath = baseFolder;
	if (basePath[basePath.length() - 1] != '/') basePath += "/";
	videoPath = basePath + FRAMES_DIRECTORY + "/";

	// Create base path
	if (mkdir(basePath.c_str(), 0777) != 0)
		throw cvgException("[DataWriter] Error creating base directory: '" + basePath + "'");

	// Create path for video frames
	if (mkdir(videoPath.c_str(), 0777) != 0)
		throw cvgException("[DataWriter] Error creating video directory: '" + videoPath + "'");

	// Create events file
	eventsFile = fopen((basePath + EVENTS_FILE_NAME).c_str(), "w");
	if (eventsFile == NULL) {
		// Remove directories before throwing the exception
		rmdir(videoPath.c_str());
		rmdir(basePath.c_str());
		throw cvgException("[DataWriter] Unable to open file for writing: '" + (basePath + EVENTS_FILE_NAME) + "'");
	}
	start();
}

void DataWriter::destroy() {
	if (isStarted()) {
		selfStop();
		if (newEventMutex.lock()) {
			newEventCondition.signal();
			newEventMutex.unlock();
		}
		stop();
		// Delete any events still in the queue
		while(!eventQueue.empty()) {
			Event *e = eventQueue.front();
			eventQueue.pop();
			delete e;
		}
	}
	if (eventsFile != NULL) {
		fclose(eventsFile);
		eventsFile = NULL;
	}
}

void DataWriter::run() {
	if (newEventMutex.lock()) {
		try {
			newEventCondition.wait(&newEventMutex);
			while(!eventQueue.empty()) {
				Event *event = eventQueue.front();
				if (event->type == Event::VIDEOFRAME) {
					((VideoFrameEvent *)event)->imageFileName = cvgString(FRAMES_DIRECTORY) + "/" + ((VideoFrameEvent *)event)->imageFileName;
				}
				fprintf(eventsFile, "%016"CVG_PRINTF_PREFIX_LONG"d %s\n", event->timeCode, event->toString().c_str());
				if (event->type == Event::VIDEOFRAME) {
					((VideoFrameEvent *)event)->imageFileName = basePath + ((VideoFrameEvent *)event)->imageFileName;
				}
				event->save();
				eventQueue.pop();
				delete event;
			}
		} catch(cvgException e) {
			log(e.getMessage());
		}
		newEventMutex.unlock();
	}
}

cvgString DataWriter::DroneInfoEvent::toString() {
	char modeStr[32];
	droneModeToString(modeStr, data.droneMode);

	return cvgString(cvgString(	"[info]") +
								" comm: " + (data.commWithDrone ? "ok" : "error") +
								" mode: " + modeStr + " nat.mode: " + data.nativeDroneState +
								" batt: " + data.batteryLevel +
								" yaw:" + data.yaw + " pitch:" + data.pitch + " roll:" + data.roll +
								" z:" + data.altitude +
								" Vx:" + data.speedX + " Vy:" + data.speedY + " Vyaw:" + data.speedYaw
								);
}

cvgString DataWriter::VideoFrameEvent::toString() {
	return 	"[frame] " + imageFileName;
}

void DataWriter::VideoFrameEvent::save() {
	IplImage *image = cvCreateImageHeader(cvSize(width, height), IPL_DEPTH_8U, 3);
	cvSetData(image, imageData, width * 3);
	cvSaveImage(imageFileName.c_str(), image);
	cvReleaseImageHeader(&image);
}

void DataWriter::addEvent(Event *event) {
	if (newEventMutex.lock()) {
		eventQueue.push(event);
		newEventCondition.signal();
		newEventMutex.unlock();
	}
}

cvgString DataWriter::ViconDataEvent::toString() {
	Vector3 pos = droneTransformFromLocalFrame.getTranslation();
	Vector3 rot = droneTransformFromLocalFrame.getRotationMatrix().getEulerAnglesZYX();
	return cvgString(	"[vicon]") +
						" x:" + pos.x * 1e-3 + " y:" + pos.y * 1e-3 + " z:" + pos.z * 1e-3 +
						" yaw:" + (rot.z * 180.0) / M_PI + " pitch:" + (rot.y * 180.0) / M_PI + " roll:" + (rot.x * 180.0) / M_PI +
						" vx:" + localSpeed.x * 1e-3 + " vy:" + localSpeed.y * 1e-3 + " vz:" + localSpeed.z * 1e-3;
}

cvgString DataWriter::CommandsEvent::toString() {
	char fmStr[32];
	flyingModeToString(fmStr, commands.controlData.flyingMode);
	return cvgString(	"[cmd]") +
						" mode:" + fmStr +
						" yawSpeed:" + commands.controlData.yaw + " pitch:" + commands.controlData.theta + " roll:" + commands.controlData.phi +
						" altSpeed:" + commands.controlData.gaz;
}

}
}
