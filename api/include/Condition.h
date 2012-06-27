/*
 * Condition.h
 *
 *  Created on: 04/09/2011
 *      Author: Ignacio Mellado
 */

#ifndef CONDITION_H_
#define CONDITION_H_

#include "Mutex.h"
#include <atlante.h>
#include <math.h>
#include <sys/time.h>
#include <time.h>

namespace DroneProxy {
namespace Threading {

class Condition {
private:
	pthread_cond_t handler;
	volatile bool conditionFired;
public:
	inline Condition() {
		conditionFired = false;
		if (pthread_cond_init(&handler, NULL) != 0)
			throw cvgException("[Condition] Cannot initialize condition handler");
	}

	virtual ~Condition() {
		pthread_cond_destroy(&handler);
	}

	inline void wait(Mutex *mutex) {
		while(!conditionFired) {
			if (pthread_cond_wait(&handler, &mutex->handler) != 0)
				throw cvgException("[Condition] Error waiting for condition");
		}
		conditionFired = false;
	}

	inline cvg_bool wait(Mutex *mutex, cvg_double timeoutSeconds) {
		timeval tv;
		gettimeofday(&tv, NULL);
		timespec ts;
		ts.tv_sec = tv.tv_sec + (cvg_ulong)floor(timeoutSeconds);
		ts.tv_nsec = 1000 * tv.tv_usec + (cvg_ulong)(1e9 * (timeoutSeconds - floor(timeoutSeconds)));
		if (ts.tv_nsec >= CVG_LITERAL_LONG(1000000000)) {
			ts.tv_sec++;  
			ts.tv_nsec -= CVG_LITERAL_LONG(1000000000);
		}
		int res = 0;
		while(!conditionFired && res != ETIMEDOUT) {		
			res = pthread_cond_timedwait(&handler, &mutex->handler, &ts);
			if (res != 0 && res != ETIMEDOUT) {
				mutex->unlock();
				throw cvgException("[Condition] Error waiting for condition");
			}
		}
		cvg_bool result = conditionFired;
		conditionFired = false;
		return result;
	}

	inline void signal() {
		conditionFired = true;
		pthread_cond_signal(&handler);
	}
};

}
}

#endif /* CONDITION_H_ */
