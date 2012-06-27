/*
 * main.cpp
 *
 *  Created on: 27/04/2012
 *      Author: Ignacio Mellado-Bataller
 */

#include <iostream>
#include <atlante.h>
#include "Proxy.h"

#include <signal.h>

volatile cvg_bool loop = true;

void _endSignalCatcher(int s) {
	loop = false;
}

int main() {
	// Set termination signals
	struct sigaction sa;
	bzero(&sa, sizeof(sa));
	sa.sa_handler = &_endSignalCatcher;
	sigaction(SIGINT, &sa, NULL);
	sigaction(SIGTERM, &sa, NULL);

	try {
		Proxy proxy;
		proxy.open();
		while(loop) {
			usleep(30000);
		}
		proxy.close();
	} catch(cvgException &e) {
		std::cout << "cvgException: " << e.getMessage() << std::endl;
	} catch(std::exception &e) {
		std::cout << "std::exception: " << e.what() << std::endl;
	}

	return 0;
}



