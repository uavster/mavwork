/*
 * LogProducer.h
 *
 *  Created on: 23/08/2011
 *      Author: Ignacio Mellado
 */

#ifndef LOGPRODUCER_H_
#define LOGPRODUCER_H_

#include <stdarg.h>
#include <list>
#include <atlante.h>
#include "LogConsumer.h"
#include "Mutex.h"

namespace DroneProxy {
namespace Logs {

class LogProducer {
private:
	std::list<LogConsumer *> consumers;
	DroneProxy::Threading::Mutex consumerListMutex;
	volatile cvg_bool isOpen;
	cvgString producerName;

public:
	LogProducer();
	virtual ~LogProducer();

	void openLog(const char *producerName);
	void closeLog();

	void addLogConsumer(LogConsumer *c);
	void removeLogConsumer(LogConsumer *c);
	void removeAllLogConsumers();

	void log(const char *s, ...);
	void log(const cvgString &s);
};

}
}

#endif /* LOGPRODUCER_H_ */
