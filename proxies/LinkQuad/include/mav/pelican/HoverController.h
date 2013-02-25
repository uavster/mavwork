/*
 * HoverController.h
 *
 *  Created on: 19/06/2012
 *      Author: Ignacio Mellado-Bataller
 */

#ifndef HOVERCONTROLLER_H_
#define HOVERCONTROLLER_H_

#include "../../control/PID.h"

namespace Mav {
namespace Pelican {

class HoverController {
public:
	HoverController();
	void run(cvg_double Vx_measure, cvg_double Vy_measure, float *pitchCmdDeg, float *rollCmdDeg);
	void enable(cvg_bool e);
private:
	PID pidVx, pidVy;
	cvg_bool enabled;
};

}
}

#endif /* HOVERCONTROLLER_H_ */
