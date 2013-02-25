/*
 * Presentation.h
 *
 *  Created on: 21/05/2012
 *      Author: Ignacio Mellado-Bataller
 */

#ifndef PRESENTATION_H_
#define PRESENTATION_H_

namespace Comm {

class Presentation {
public:
	static void hton(float *f);
	static void hton(unsigned int *i);
	static void hton(int *i);
	static void hton(short *s);
	static void hton(unsigned short *s);
	static void hton(char *c);
	static void hton(unsigned char *c);

	static void ntoh(float *f);
	static void ntoh(unsigned int *i);
	static void ntoh(int *i);
	static void ntoh(short *s);
	static void ntoh(unsigned short *s);
	static void ntoh(char *c);
	static void ntoh(unsigned char *c);
};

}

#endif /* PRESENTATION_H_ */
