/**
 * Description: Objects of class cvgThread open a separate thread which constantly calls a virtual run() method 
 * that may be overloaded by the programmer to perform custom processing. 
 * start() and stop() methods are provided to control the execution of the thread.
 * Author: Ignacio Mellado (CVG)
 * Last revision: 22/11/2010
 */

#include "../include/cvgThread.h"

#define DEFAULT_STOP_TIMEOUT	CVG_LITERAL_INT(5000)
#define WORKER_LOOP_WAIT	CVG_LITERAL_INT(0)

// By the time it's being written, ETIMEDOUT is not defined on Cygwin. Instead, WSAETIMEDOUT is used.
#ifndef ETIMEDOUT
#define ETIMEDOUT	116
#endif

#ifndef CVG_WINDOWS
#include <signal.h>
#endif

cvgThread::cvgThread(cvgString *name) {
	created = false;
	started = false;
	setName(name);
	setStopTimeout(DEFAULT_STOP_TIMEOUT);
}

cvgThread::cvgThread(const cvg_char *name) {
	started = false;
	created = false;
	setName(name);
	setStopTimeout(DEFAULT_STOP_TIMEOUT);
}

cvgThread::~cvgThread() {
	destroy();
}

void cvgThread::create() {
	if (created) return;
#ifdef CVG_WINDOWS
	if ((endEvent = CreateEvent(NULL, FALSE, FALSE, NULL)) == NULL)
		throw cvgException("Unable to create end event");
	if ((endedEvent = CreateEvent(NULL, FALSE, FALSE, NULL)) == NULL) {
		CloseHandle(endEvent);
		throw cvgException("Unable to create ended event");
	}
#endif
	created = true;
}

void cvgThread::destroy() {
	if (!created) return;
	stop();
#ifdef CVG_WINDOWS
	CloseHandle(endEvent);
	CloseHandle(endedEvent);
#endif
	created = false;
}

void cvgThread::start() {
	create();
#ifdef CVG_WINDOWS
	ResetEvent(endedEvent);
	ResetEvent(endedEvent);
	if ((handle = CreateThread(NULL, 0, &worker, this, 0, &threadId)) == NULL)
		throw cvgException("Unable to create thread");
#else
	loop = true;
	if (pthread_create(&handle, NULL, worker, this) != 0)
		throw cvgException("Unable to create thread");
#endif
	started = true;
}

void cvgThread::stop() {
	if (!started) return;
#ifdef CVG_WINDOWS
	if (GetCurrentThreadId() == threadId)
#else
	if (pthread_self() == handle)
#endif
		selfStop();	// This will stopped the current thread, but it must be joined from another thread with stop() to free the resources 
	else {
		bool error = false;
#ifdef CVG_WINDOWS
		SetEvent(endEvent);
		error = WaitForSingleObject(endedEvent, stopTimeout) != WAIT_OBJECT_0;
#else
		loop = false;
		timespec ts;
		setAbsoluteTimeout(&ts, stopTimeout);
		void *retval;
		error = pthread_timedjoin_np(handle, &retval, &ts) == ETIMEDOUT;
#endif
		if (error) {
			throw cvgException("The thread \"" + name + "\" did not finish in " + stopTimeout + " ms");
		}
		started = false;
	}
}

#ifdef CVG_WINDOWS
DWORD WINAPI cvgThread::worker(void *p) {
#else
void *cvgThread::worker(void *p) {
#endif
#define cvg_this	((cvgThread *)p)
#ifndef CVG_WINDOWS
	try {
		// sigtimedwait() is not used to catch SIGKILL synchronously because it is not implemented in Cygwin
		while(cvg_this->loop) {
			cvg_this->run();
#if defined(WORKER_LOOP_WAIT) && (WORKER_LOOP_WAIT > 0)
			usleep(WORKER_LOOP_WAIT * CVG_LITERAL_INT(1000));
#endif
		}
	} catch(cvgException e) {
		// Manage the error
	}
	cvg_this->started = false;
	return NULL;
#else
	try {
		while(WaitForSingleObject(cvg_this->endEvent, WORKER_LOOP_WAIT) != WAIT_OBJECT_0) {
			cvg_this->run();
#if defined(WORKER_LOOP_WAIT) && (WORKER_LOOP_WAIT > 0)
			Sleep(WORKER_LOOP_WAIT);
#endif
		} 
	} catch(cvgException e) {
		// Manage the error
	}
	SetEvent(cvg_this->endedEvent);
	cvg_this->started = false;
	return 0;
#endif
#undef cvg_this
}

#ifndef CVG_WINDOWS
#include <sys/time.h>

void cvgThread::setAbsoluteTimeout(timespec *ts, cvg_uint absTimeoutMs) {
	timeval tv;
	gettimeofday(&tv, NULL);
	cvg_ulong absTimeUs = (cvg_ulong)tv.tv_sec * CVG_LITERAL_INT(1000000) + tv.tv_usec;
	absTimeUs += (cvg_ulong)absTimeoutMs * CVG_LITERAL_LONG(1000);
	ts->tv_sec = absTimeUs / CVG_LITERAL_INT(1000000);
	ts->tv_nsec = (absTimeUs % CVG_LITERAL_INT(1000000)) * CVG_LITERAL_INT(1000);
}
#endif
