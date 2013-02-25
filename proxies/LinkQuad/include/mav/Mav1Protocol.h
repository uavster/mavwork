/*
 * MavProtocol.h
 *
 *  Created on: 13/05/2012
 *      Author: Ignacio Mellado-Bataller
 */

#ifndef MAVPROTOCOL_H_
#define MAVPROTOCOL_H_

#ifndef __PACKED
#define __PACKED	__attribute__((__packed__))
#endif

#include "../comm/presentation/Presentation.h"
#include <string.h>

namespace Mav {

enum AccessMode { COMMANDER = 0, LISTENER = 1, UNKNOWN_MODE = 2 };
enum FlyingMode { IDLE = 0, TAKEOFF = 1, HOVER = 2, MOVE = 3, LAND = 4, INIT = 5, EMERGENCYSTOP = 6 };

struct CommandData__ {
	unsigned int	timeCodeH;
	unsigned int	timeCodeL;
	unsigned char	flyingMode;
	float			phi;
	float			theta;
	float			gaz;
	float			yaw;

	void hton() {
		Comm::Presentation::hton(&timeCodeH); Comm::Presentation::hton(&timeCodeL);
		Comm::Presentation::hton(&flyingMode); Comm::Presentation::hton(&phi);
		Comm::Presentation::hton(&theta); Comm::Presentation::hton(&gaz); Comm::Presentation::hton(&yaw);
	}

	void ntoh() {
		Comm::Presentation::ntoh(&timeCodeH); Comm::Presentation::ntoh(&timeCodeL);
		Comm::Presentation::ntoh(&flyingMode); Comm::Presentation::ntoh(&phi);
		Comm::Presentation::ntoh(&theta); Comm::Presentation::ntoh(&gaz); Comm::Presentation::ntoh(&yaw);
	}

	CommandData__ &operator = (const CommandData__ &cd) {
		memcpy(this, &cd, sizeof(CommandData__));
	}

} __PACKED;
typedef struct CommandData__ CommandData;

enum DroneMode { LANDED = 0, FLYING = 1, HOVERING = 2, TAKINGOFF = 3, LANDING = 4, EMERGENCY = 5, UNKNOWN = 6 };

struct StateInfo__ {
	unsigned int	timeCodeH;
	unsigned int	timeCodeL;
	char			commWithDrone;
	char		 	droneMode;
	int				nativeDroneState;
	float			batteryLevel;
	float			roll;
	float			pitch;
	float			yaw;
	float			altitude;
	float			speedX;
	float			speedY;
	float			speedYaw;
	//float			userData[8];

	void hton() {
		Comm::Presentation::hton(&timeCodeH); Comm::Presentation::hton(&timeCodeL);
		Comm::Presentation::hton(&commWithDrone); Comm::Presentation::hton(&droneMode);
		Comm::Presentation::hton(&nativeDroneState); Comm::Presentation::hton(&batteryLevel);
		Comm::Presentation::hton(&roll); Comm::Presentation::hton(&pitch); Comm::Presentation::hton(&yaw);
		Comm::Presentation::hton(&altitude); Comm::Presentation::hton(&speedX); Comm::Presentation::hton(&speedY);
		Comm::Presentation::hton(&speedYaw);
	}

	void ntoh() {
		Comm::Presentation::ntoh(&timeCodeH); Comm::Presentation::ntoh(&timeCodeL);
		Comm::Presentation::ntoh(&commWithDrone); Comm::Presentation::ntoh(&droneMode);
		Comm::Presentation::ntoh(&nativeDroneState); Comm::Presentation::ntoh(&batteryLevel);
		Comm::Presentation::ntoh(&roll); Comm::Presentation::ntoh(&pitch); Comm::Presentation::ntoh(&yaw);
		Comm::Presentation::ntoh(&altitude); Comm::Presentation::ntoh(&speedX); Comm::Presentation::ntoh(&speedY);
		Comm::Presentation::ntoh(&speedYaw);
	}

	StateInfo__ &operator = (const StateInfo__ &cd) {
		memcpy(this, &cd, sizeof(StateInfo__));
	}

} __PACKED;
typedef struct StateInfo__ StateInfo;

}

#endif /* MAVPROTOCOL_H_ */
