/*
 * Presentation.cpp
 *
 *  Created on: 21/05/2012
 *      Author: Ignacio Mellado-Bataller
 */
#include <iostream>
#include <arpa/inet.h>
#include "comm/presentation/Presentation.h"

namespace Comm {

void Presentation::hton(float *f) {
	*(unsigned int *)f = htonl(*(unsigned int *)f);
}

void Presentation::hton(unsigned int *i) {
	*i = htonl(*i);
}

void Presentation::hton(int *i) {
	*(unsigned int *)i = htonl(*(unsigned int *)i);
}

void Presentation::hton(short *s) {
	*(unsigned short *)s = htons(*(unsigned short *)s);
}

void Presentation::hton(unsigned short *s) {
	*s = htons(*s);
}

void Presentation::hton(char *c) {
}

void Presentation::hton(unsigned char *c) {
}

void Presentation::ntoh(float *f) {
	*(unsigned int *)f = ntohl(*(unsigned int *)f);
}

void Presentation::ntoh(unsigned int *i) {
	*i = ntohl(*i);
}

void Presentation::ntoh(int *i) {
	*(unsigned int *)i = ntohl(*(unsigned int *)i);
}

void Presentation::ntoh(short *s) {
	*(unsigned short *)s = ntohs(*(unsigned short *)s);
}

void Presentation::ntoh(unsigned short *s) {
	*s = ntohs(*s);
}

void Presentation::ntoh(char *c) {
}

void Presentation::ntoh(unsigned char *c) {
}

}

