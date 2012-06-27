/*
 * Mutex.h
 *
 *  Created on: 02/09/2011
 *      Author: Ignacio Mellado
 *      Notes: This is class is currently only implemented for Linux. When it also supports Windows, it may be included
 *      in Atlante library.
 */

#ifndef MUTEX_H_
#define MUTEX_H_

#include <pthread.h>
#include <atlante.h>

namespace DroneProxy {
namespace Threading {

class Mutex {
	friend class Condition;
private:
	pthread_mutex_t handler;

public:
	Mutex(cvg_bool recursive = true);
	virtual ~Mutex();

	cvg_bool lock();
	cvg_bool unlock();
};

}
}

#endif /* MUTEX_H_ */
