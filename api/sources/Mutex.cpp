/*
 * Mutex.cpp
 *
 *  Created on: 02/09/2011
 *      Author: Ignacio Mellado
 *      Description: This class implements a recursive mutex
 *      Notes: The class only supports Linux by now. As soon as it also supports Windows, it may be included
 *      in the Atlante library.
 */

#include <Mutex.h>

namespace DroneProxy {
namespace Threading {

Mutex::Mutex(cvg_bool recursive) {
	pthread_mutexattr_t mutexAttr;
	if (recursive) {
		if (pthread_mutexattr_init(&mutexAttr) != 0)
			throw cvgException("[Mutex] Unable to initialize mutex attributes");
		if (pthread_mutexattr_settype(&mutexAttr, PTHREAD_MUTEX_RECURSIVE_NP) != 0) {
			pthread_mutexattr_destroy(&mutexAttr);
			throw cvgException("[Mutex] Unable to set mutex type to recursive");
		}
	}
	if (pthread_mutex_init(&handler, recursive ? &mutexAttr : NULL) != 0) {
		if (recursive) pthread_mutexattr_destroy(&mutexAttr);
		throw cvgException("[Mutex] Unable to create mutex");
	}
	if (recursive) pthread_mutexattr_destroy(&mutexAttr);
}

Mutex::~Mutex() {
	pthread_mutex_destroy(&handler);
}

cvg_bool Mutex::lock() {
	return pthread_mutex_lock(&handler) == 0;
}

cvg_bool Mutex::unlock() {
	return pthread_mutex_unlock(&handler) == 0;
}

}
}
