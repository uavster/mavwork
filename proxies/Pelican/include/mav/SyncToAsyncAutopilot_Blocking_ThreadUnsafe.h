/*
 * SyncToAsyncAutopilot.h
 *
 *  Created on: 13/05/2012
 *      Author: Ignacio Mellado-Bataller
 */

#ifndef SYNCTOASYNCAUTOPILOT_BLOCKING_THREADUNSAFE_H_
#define SYNCTOASYNCAUTOPILOT_BLOCKING_THREADUNSAFE_H_

#include <atlante.h>
#include "IAsyncAutopilot.h"
#include "../base/EventGenerator.h"

namespace Mav {

template<class _SyncAutopilotT> class SyncToAsyncAutopilot_Blocking_ThreadUnsafe
	: public virtual IAsyncAutopilot,
	  public virtual EventGenerator
{
private:

	class Worker : public virtual cvgThread {
	private:
		cvg_double stateInfoPeriod;
		cvgMutex mutex;
		cvgCondition sendAvail;
		cvgTimer timer;
		cvg_bool firstRun;
		cvg_double remainingPeriod;
		StateInfo stateInfo;
		CommandData commandData;
	protected:
		void run();
	public:
		SyncToAsyncAutopilot_Blocking_ThreadUnsafe<_SyncAutopilotT> *parent;
		Worker();
		virtual ~Worker() { stop(); }
		void setStateInfoPeriod(cvg_double p);
		void setCommandData(const CommandData &cd);
		void start();
		inline void setParent(SyncToAsyncAutopilot_Blocking_ThreadUnsafe<_SyncAutopilotT> *p) { parent = p; }
	};
	Worker worker;
	_SyncAutopilotT autopilot;
	cvg_bool firstRun;

	friend class Worker;
	cvgTimer lastCommandTimer;

protected:
	virtual void gotEvent(EventGenerator &source, const Event &e);

public:
	SyncToAsyncAutopilot_Blocking_ThreadUnsafe();
	virtual ~SyncToAsyncAutopilot_Blocking_ThreadUnsafe();

	inline void setStatePollingFrequency(cvg_double pf) { worker.setStateInfoPeriod(1.0 / pf); }

	// IAsyncAutopilot interface (plus state info event notifications)
	virtual void open();
	virtual void close();
	virtual inline void setCommandData(const CommandData &cd) { worker.setCommandData(cd); }

	inline void getAvs(cvg_double *r, cvg_double *w, cvg_int *rc, cvg_int *wc) { *r = worker.parent->autopilot.readAverage; *w = worker.parent->autopilot.writeAverage; *rc = worker.parent->autopilot.rCount; *wc = worker.parent->autopilot.wCount; }

	inline _SyncAutopilotT *getAutopilot() { return &autopilot; }
};

}

#include "../../sources/SyncToAsyncAutopilot_Blocking_ThreadUnsafe.hh"

#endif /* SYNCTOASYNCAUTOPILOT_H_ */
