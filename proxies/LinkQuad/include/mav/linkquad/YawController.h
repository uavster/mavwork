/*
 * YawController.h
 *
 *  Created on: 24/10/2012
 *      Author: Ignacio Mellado-Bataller
 */

#ifndef YAWCONTROLLER_H_
#define YAWCONTROLLER_H_

#include <atlante.h>
#include "../../control/PID.h"

namespace Mav {
namespace Linkquad {

class YawController {
public:
	YawController();
	void setFeedback(cvg_double yawMeasure_deg);
	cvg_double getOutput();
	void setReference(cvg_double yawRef_deg);
	void enable(cvg_bool e);
	inline cvg_bool isEnabled() { return enabled; }
	inline void setMaxAllowedOutput(cvg_double maxOutput) { yawPid.enableMaxOutput(true, maxOutput); }
	inline cvg_double getReference() { return reference; }
	cvg_double mapAngleToMinusPlusPi(cvg_double angleRads);

protected:
	void calcError();

private:
	PID yawPid;
	cvg_bool enabled;
	cvg_double reference, feedback;
};

}
}

#endif /* YAWCONTROLLER_H_ */
