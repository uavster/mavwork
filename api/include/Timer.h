#ifndef TIMER_INCLUDED__
#define TIMER_INCLUDED__

#include <sys/time.h>
#include <stdlib.h>

namespace DroneProxy {
namespace Timing {

class Timer {
private:
	timeval t0, t1;
public:
	inline Timer() { restart(false); }
	virtual ~Timer();

	inline void restart(bool useLastSample = true) { if (useLastSample) t0 = t1; else gettimeofday(&t0, NULL); }
	inline double getElapsedSeconds() {
		gettimeofday(&t1, NULL);
		return (t1.tv_sec - t0.tv_sec) + (t1.tv_usec - t0.tv_usec) / 1e6;
	}
};

}
}

#endif
