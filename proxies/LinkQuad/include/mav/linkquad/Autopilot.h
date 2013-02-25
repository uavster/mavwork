/*
 * LinkquadAutopilot.h
 *
 *  Created on: 12/09/2012
 *      Author: Ignacio Mellado-Bataller
 */

#ifndef LINKQUADAUTOPILOT_H_
#define LINKQUADAUTOPILOT_H_

#include "../IAsyncAutopilot.h"
#include "AltitudeSensor.h"
#include "AltSpeedController.h"
#include "HoverController.h"
#include "SpeedEstimator.h"
#include "YawController.h"
#include "../../base/EventListener.h"
#include "../../base/EventGenerator.h"

namespace Mav {
namespace Linkquad {

class LinkquadAutopilot :
	public virtual IAsyncAutopilot,
	public virtual EventListener,
	public virtual EventGenerator,
	public virtual cvgThread
{
public:
	LinkquadAutopilot();

	virtual void open();
	virtual void close();

	// setControlData() is synchronously called by the system to send commands to the MAV.
	// This method must be a non-blocking call.
	virtual void setCommandData(const CommandData &cd);
	virtual void setFeedbackRate(cvg_double pf);
	virtual void notifyLongTimeSinceLastStateInfoUpdate(StateInfo *si);
	inline virtual void trim() { trimming = true; }

	inline SpeedEstimator *getSpeedEstimator() { return &speedEstimator; }


protected:
	static void controlMCUCallback(void *data, void *userData);
	cvg_double mapAngleToMinusPlusPi(cvg_double angleRads);
	virtual void gotEvent(EventGenerator &source, const Event &e);
	virtual void run();
	void saveTrimToFile(const cvgString &fileName);
	void loadTrimFromFile(const cvgString &fileName);

private:
	cvg_double feedbackFrequency;
	GotStateInfoEvent feedbackEvent;
	cvg_bool isOpen;

	volatile cvg_bool firstSample;
	volatile cvg_bool trimming;
	cvg_float yawOffset, pitchOffset, rollOffset;

	AltitudeSensor altitudeSensor;
	SpeedEstimator speedEstimator;
	cvg_bool isLastSpeedMeasureValid;

	DroneMode droneMode;
	cvgMutex controllerThreadMutex;
	cvgCondition controllerThreadCondition;
	StateInfo lastStateInfo_bck;
	CommandData lastCommand;

	Mav::Linkquad::AltSpeedController altSpeedController;
	Mav::Linkquad::HoverController hoverController;
	Mav::Linkquad::YawController yawController;
	cvgTimer yawRateTimer;
	cvg_bool firstCommand;

	cvgTimer landingTimer;
	cvg_bool landingWithTime;

	cvgTimer takingoffTimer;

	// TODO: Remove this after testing
	volatile cvg_float curThrust;
	// TODO: -------------------------

};

}
}

#endif /* LINKQUADAUTOPILOT_H_ */
