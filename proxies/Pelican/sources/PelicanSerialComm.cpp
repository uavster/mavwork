/*
 * PelicanSerialComm.cpp
 *
 *  Created on: 14/05/2012
 *      Author: Ignacio Mellado-Bataller
 */
#include <iostream>
#include "mav/pelican/serial/PelicanSerialComm.h"
#include "rs232.h"

namespace Mav {

cvg_bool PelicanSerialComm::open(cvg_int portNumber, cvg_int baudrate, cvg_double timeout) {
	this->portNumber = portNumber;
	this->timeout = timeout;
	return !OpenComport(portNumber, baudrate);
}

void PelicanSerialComm::close() {
	CloseComport(portNumber);
}

cvg_bool PelicanSerialComm::read(void *data, cvg_int len) {
    int i = 0;
    cvgTimer tmr;
    while(i < len) {
    	if (tmr.getElapsedSeconds() > timeout) break;

        int read = PollComport(portNumber, &((unsigned char *)data)[i], len - i);

        if (read > 0)
            i += read;
//        std::cout << "poll: " << tmr.getElapsedSeconds() << " (" << i << std::endl;
        if (i < len) usleep(10);
    }
    return i >= len;
}

cvg_bool PelicanSerialComm::write(const void *data, cvg_int len) {
	int i = 0;
	cvgTimer tmr;
	while(i < len) {
		if (tmr.getElapsedSeconds() > timeout) break;
		int written = SendBuf(portNumber, (unsigned char *)data, len - i);

		if (written > 0)
			i += written;
            if (i < len) usleep(10);
	}
	return i >= len;
}

}
