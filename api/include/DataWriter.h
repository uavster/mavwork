/*
 * DataWriter.h
 *
 *  Created on: 12/09/2011
 *      Author: Ignacio Mellado
 */

#ifndef DATAWRITER_H_
#define DATAWRITER_H_

#include <atlante.h>
#include <stdio.h>
#include <queue>
#include "LogProducer.h"
#include <string.h>
#include "Drone.h"
#include "Protocol.h"

namespace DroneProxy {
namespace Logs {

class DataWriter : public virtual cvgThread, public DroneProxy::Logs::LogProducer {
public:
	class Event {
	public:
		enum EventType { DRONEINFO, VIDEOFRAME, VICONDATA, COMMANDS };
		EventType type;
		cvg_ulong timeCode;

		inline Event(cvg_ulong timeCode) { this->timeCode = timeCode; }
		inline virtual ~Event() { }

		inline virtual cvgString toString() { }
		inline virtual void save() { }
	};

	class DroneInfoEvent : public Event {
	private:
		FeedbackData data;
	public:
		inline DroneInfoEvent(cvg_ulong timeCode, FeedbackData *fd)
			: Event(timeCode) {
			type = DRONEINFO; memcpy(&data, fd, sizeof(FeedbackData));
		}
		virtual cvgString toString();
	};

	class VideoFrameEvent : public Event {
		friend class DataWriter;
	private:
		cvgString imageFileName;
		cvg_uint width, height;
		cvg_char *imageData;
	public:
		inline VideoFrameEvent(cvg_ulong timeCode, const cvgString &imageFileName, cvg_uint width, cvg_uint height, cvg_char *imageData)
			: Event(timeCode) {
			type = VIDEOFRAME;
			this->imageFileName = imageFileName;
			this->width = width;
			this->height = height;
			this->imageData = new cvg_char[width * height * 3];
			memcpy(this->imageData, imageData, width * height * 3);
		}
		inline virtual ~VideoFrameEvent() { if (imageData != NULL) { delete [] imageData; imageData = NULL; } }
		virtual cvgString toString();
		virtual void save();
	};

	class ViconDataEvent : public Event {
	private:
		HTMatrix4 droneTransformFromLocalFrame;
		Vector3 localSpeed;
	public:
		inline ViconDataEvent(cvg_ulong timeCode, const HTMatrix4 &droneTransformFromLocalFrame, const Vector3 &localSpeed)
			: Event(timeCode) {
			type = VICONDATA;
			this->droneTransformFromLocalFrame = droneTransformFromLocalFrame;
			this->localSpeed = localSpeed;
		}
		virtual inline ~ViconDataEvent() { }
		virtual cvgString toString();
		inline ViconDataEvent &operator = (const ViconDataEvent &ve) {
			droneTransformFromLocalFrame = ve.droneTransformFromLocalFrame;
			localSpeed = ve.localSpeed;
		}
	};

	class CommandsEvent : public Event {
	private:
		CommandPacket commands;
	public:
		inline CommandsEvent(cvg_ulong timeCode, CommandPacket *cp)
			: Event(timeCode) {
			type = COMMANDS; memcpy(&commands, cp, sizeof(CommandPacket));
		}
		inline virtual ~CommandsEvent() { }
		virtual cvgString toString();
	};

private:
	FILE *eventsFile;
	std::queue<Event *> eventQueue;
	DroneProxy::Threading::Mutex newEventMutex;
	DroneProxy::Threading::Condition newEventCondition;
	cvgString basePath, videoPath;

public:
	DataWriter();
	~DataWriter();

	void create(const cvgString &baseFolder);
	void destroy();

	void addEvent(Event *event);

protected:
	virtual void run();
};

}
}

#endif /* DATAWRITER_H_ */
