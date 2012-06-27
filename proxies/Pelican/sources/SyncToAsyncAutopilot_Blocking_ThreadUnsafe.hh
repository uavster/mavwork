/*
 * SyncToAsyncAutopilot.hh
 *
 *  Created on: 13/05/2012
 *      Author: Ignacio Mellado-Bataller
 */

#define MAX_TIME_WITHOUT_COMMANDS_BEFORE_HOVERING		0.5

namespace Mav {

#define DEFAULT_STATE_INFO_PERIOD	(1.0/20.0)

template<class _SyncAutopilotT> SyncToAsyncAutopilot_Blocking_ThreadUnsafe<_SyncAutopilotT>::SyncToAsyncAutopilot_Blocking_ThreadUnsafe() {
	worker.setParent(this);
	// If the provided SyncAutopilot type is not blocking and thread-unsafe, this will give a compilation error
	autopilot.isBlockingThreadUnsafe();
}

template<class _SyncAutopilotT> SyncToAsyncAutopilot_Blocking_ThreadUnsafe<_SyncAutopilotT>::~SyncToAsyncAutopilot_Blocking_ThreadUnsafe() {
	close();
}

template<class _SyncAutopilotT> void SyncToAsyncAutopilot_Blocking_ThreadUnsafe<_SyncAutopilotT>::open() {
	autopilot.open();
	addEventListener(&autopilot);
	worker.start();
}

template<class _SyncAutopilotT> void SyncToAsyncAutopilot_Blocking_ThreadUnsafe<_SyncAutopilotT>::close() {
	worker.stop();
	removeAllEventListeners();
	autopilot.close();
}

template<class _SyncAutopilotT> SyncToAsyncAutopilot_Blocking_ThreadUnsafe<_SyncAutopilotT>::Worker::Worker()
	: cvgThread("SyncToAsyncAutopilot::worker") {
	stateInfoPeriod = DEFAULT_STATE_INFO_PERIOD;
}

template<class _SyncAutopilotT> void SyncToAsyncAutopilot_Blocking_ThreadUnsafe<_SyncAutopilotT>::Worker::setCommandData(const CommandData &cd) {
	if (!mutex.lock()) throw cvgException("[SyncToAsyncAutopilot_Blocking_ThreadUnsafe::Worker] Cannot lock mutex in setControlData()");
	memcpy(&commandData, &cd, sizeof(CommandData));
	parent->lastCommandTimer.restart(false);
	sendAvail.signal();
	mutex.unlock();
}

template<class _SyncAutopilotT> void SyncToAsyncAutopilot_Blocking_ThreadUnsafe<_SyncAutopilotT>::Worker::start() {
	// State info will be request on the first run()
	remainingPeriod = 0.0;
	firstRun = true;
	cvgThread::start();
}

template<class _SyncAutopilotT> void SyncToAsyncAutopilot_Blocking_ThreadUnsafe<_SyncAutopilotT>::Worker::setStateInfoPeriod(cvg_double p) {
	if (!mutex.lock()) throw cvgException("[SyncToAsyncAutopilot_Blocking_ThreadUnsafe::Worker] Cannot lock mutex in setStateInfoPeriod()");
	stateInfoPeriod = p;
	mutex.unlock();
}

template<class _SyncAutopilotT> void SyncToAsyncAutopilot_Blocking_ThreadUnsafe<_SyncAutopilotT>::Worker::run() {
//	std::cout << "elapsed: " << timer.getElapsedSeconds() << std::endl;
//	std::cout << "period: " << stateInfoPeriod << std::endl;
	try {
		if (!firstRun) remainingPeriod -= timer.getElapsedSeconds();
		else { firstRun = false; remainingPeriod = 0; timer.restart(false); timer.getElapsedSeconds(); }
	//std::cout << "remaining: " << remainingPeriod << std::endl;
		if (remainingPeriod <= 0.0) {
			// Read state info
			parent->autopilot.getStateInfo(stateInfo);
			GotStateInfoEvent event(stateInfo);
			parent->notifyEvent(event);
			// Next getElapsedSeconds() will return the elapsed time since the last getElapsedSeconds()
			timer.restart();
			// Restart period
			remainingPeriod += stateInfoPeriod;
			if (remainingPeriod < 0.0) remainingPeriod = 1e-3;
		} else timer.restart();
	//std::cout << "elapsed: " << timer.getElapsedSeconds() << std::endl;

		if (!mutex.lock()) throw cvgException("[SyncToAsyncAutopilot_Blocking_ThreadUnsafe::Worker] Cannot lock mutex in run()");
		cvg_bool timeout;
		if (remainingPeriod > 0.0) timeout = !sendAvail.wait(&mutex, remainingPeriod);
		else timeout = true;
		CommandData writeBuffer;
		if (!timeout) memcpy(&writeBuffer, &commandData, sizeof(CommandData));
		cvg_double timeSinceLastCommand = parent->lastCommandTimer.getElapsedSeconds();
		mutex.unlock();

		if (timeSinceLastCommand > MAX_TIME_WITHOUT_COMMANDS_BEFORE_HOVERING)
			parent->autopilot.connectionLost(true);
		else {
			parent->autopilot.connectionLost(false);

			if (!timeout) {
				// Send commands
				parent->autopilot.setCommandData(writeBuffer);
		//		std::cout << "elapsed: " << timer.getElapsedSeconds() << std::endl;
			}
		}
	} catch (cvgException &e) {
		std::cout << e.getMessage() << std::endl;
	}
}

template<class _SyncAutopilotT> void SyncToAsyncAutopilot_Blocking_ThreadUnsafe<_SyncAutopilotT>::gotEvent(EventGenerator &source, const Event &e) {
//std::cout << "SynctoAsyncAutopilot_Blocking_ThreadUnsafe::gotEvent : " << typeid(e).name() << std::endl;
	notifyEvent(e);
}

}
