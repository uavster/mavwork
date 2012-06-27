/*
 * LogConsumer.h
 *
 *  Created on: 23/08/2011
 *      Author: Ignacio Mellado
 */

#ifndef LOGCONSUMER_H_
#define LOGCONSUMER_H_

#include <atlante.h>

namespace DroneProxy {
namespace Logs {

class LogProducer;

class LogConsumer {
public:
	LogConsumer();
	virtual ~LogConsumer();
	virtual void logConsume(LogProducer *producer, const cvgString &str);
};

}
}

#endif /* LOGCONSUMER_H_ */
