/*
 * main.cpp
 *
 *  Created on: 12/08/2011
 *      Author: Ignacio Mellado
 */

#include <ConsoleUI.h>
#include <stdio.h>
#include <atlante.h>
#include <cv.h>
#include <highgui.h>
#include <MyDrone.h>

#include <Trajectory.h>

#include <droneproxy.h>

using namespace DroneProxy;

int main(void) {
	MyDrone myDrone;
	ConsoleUI ui(&myDrone);
	
	try {

		myDrone.setAuto(false);
//		myDrone.setPositioningSource(MyDrone::POSITIONING_VICON);
		myDrone.open("127.0.0.1", COMMANDER, 1, cvgString("arcaa-vicon-server:801"), cvgString("Nacho-ARDrone2"));
		myDrone.logDataFrom(MyDrone::ODOMETRY | MyDrone::VIDEO | MyDrone::COMMANDS); // | MyDrone::VICON);

		ui.init();
		ui.doLoop();
		ui.destroy();

		myDrone.close();
	} catch(cvgException e) {
		ui.destroy();
		myDrone.close();
		fprintf(stderr, "[Program exception] %s\n", e.getMessage().c_str());
	}

	return 0;
}




