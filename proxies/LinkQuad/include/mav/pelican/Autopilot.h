/*
 * Autopilot.h
 *
 *  Created on: 11/05/2012
 *      Author: Ignacio Mellado-Bataller
 */

#ifndef AUTOPILOT_H_
#define AUTOPILOT_H_

#include "../ISyncAutopilot_Blocking_ThreadUnsafe.h"
#include <atlante.h>
#include "serial/PelicanSerialComm.h"
#include "SpeedEstimator.h"
#include "AltitudeSensor.h"
#include "../../base/EventListener.h"
#include "../../base/EventGenerator.h"
#include "AltSpeedController.h"
#include "HoverController.h"

namespace Mav {
namespace Pelican {

class PelicanAutopilot
	: public virtual ISyncAutopilot_Blocking_ThreadUnsafe,
	  public virtual EventListener,
	  public virtual EventGenerator
{
private:
	cvg_bool commandSendOk;
	cvg_bool firstSample;
	cvg_double yawOffset;
	PelicanSerialComm serial;
	DroneMode droneMode;
	StateInfo lastStateInfo;
	cvg_bool isLastSpeedMeasureValid;
	Vector3 lastRefSpeedMeasure, firstIMUSpeed;
	SpeedEstimator speedEstimator;
	Mav::Pelican::AltitudeSensor altitudeSensor;
	AltSpeedController altSpeedController;
	cvgTimer landingTimer;
	cvg_bool landingWithTime;
	HoverController hoverController;

	class Integrator {
	private:
		cvg_double value;
		cvg_double lastSample;
		cvg_bool first;
		cvgTimer timer;
	public:
		Integrator() : value(0), lastSample(0), first(true) {}
		void operator += (cvg_double newSample) {
			if (!first) {
				value += (newSample + lastSample) / 2 * timer.getElapsedSeconds();
				timer.restart();
			} else {
				timer.restart(false);
				first = false;
			}
			lastSample = newSample;
		}
		inline void reset() { value = 0; first = true; }
		inline cvg_double getValue() { return value; }
	};

	Integrator speedX, speedY;

protected:
	cvg_double mapAngleToMinusPlusPi(cvg_double angleRads);
	virtual void gotEvent(EventGenerator &source, const Event &e);

public:
	cvg_double readAverage, writeAverage;
	cvg_int rCount, wCount;
	PelicanAutopilot();
	virtual ~PelicanAutopilot();

	// IAutopilot interface
	virtual void open();
	virtual void close();
	virtual cvg_bool setCommandData(const CommandData &cd);
	virtual cvg_bool getStateInfo(StateInfo &si);
	virtual void connectionLost(cvg_bool cl);

	inline SpeedEstimator *getSpeedEstimator() { return &speedEstimator; }
};

}
}

#endif /* AUTOPILOT_H_ */
