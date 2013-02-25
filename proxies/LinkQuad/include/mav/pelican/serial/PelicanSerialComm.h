/*
 * PelicanSerialComm.h
 *
 *  Created on: 14/05/2012
 *      Author: Ignacio Mellado-Bataller
 */

#ifndef PELICANSERIALCOMM_H_
#define PELICANSERIALCOMM_H_

#include <atlante.h>

namespace Mav {

class PelicanSerialComm {
private:
	cvg_int portNumber;
	cvg_double timeout;
public:
	PelicanSerialComm();

	cvg_bool open(cvg_int portNumber, cvg_int baudrate, cvg_double timeout);
	void close();

	cvg_bool write(const void *data, cvg_int length);
	cvg_bool read(void *data, cvg_int length);
};

}
#endif /* PELICANSERIALCOMM_H_ */
