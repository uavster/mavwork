/*
 * LogProducer.cpp
 *
 *  Created on: 23/08/2011
 *      Author: Ignacio Mellado
 */

#include <LogProducer.h>
#include <stdio.h>
#include <pthread.h>
#include <atlante.h>

namespace DroneProxy {
namespace Logs {

LogProducer::LogProducer() {
	isOpen = false;
}

LogProducer::~LogProducer() {
	closeLog();
}

void LogProducer::openLog(const char *producerName) {
	closeLog();
	this->producerName = producerName;
	isOpen = true;
}

void LogProducer::closeLog() {
	if (!isOpen) return;
	removeAllLogConsumers();
	isOpen = false;
}

void LogProducer::addLogConsumer(LogConsumer *c) {
	if (!isOpen) throw cvgException("Cannot add log consumer because the LogProducer was not open yet");
	if (consumerListMutex.lock()) {
		consumers.push_back(c);
		consumerListMutex.unlock();
	}
}

void LogProducer::removeLogConsumer(LogConsumer *c) {
	if (!isOpen) return;
	if (consumerListMutex.lock()) {
		consumers.remove(c);
		consumerListMutex.unlock();
	}
}

void LogProducer::removeAllLogConsumers() {
	if (!isOpen) return;
	if (consumerListMutex.lock()) {
		consumers.clear();
		consumerListMutex.unlock();
	}
}

void LogProducer::log(const char *s, ...) {
	if (!isOpen) return;
	va_list vl;
	va_start(vl, s);
	char buffer[1024];
	vsnprintf(buffer, sizeof(buffer), s, vl);
	va_end(vl);
	cvgString str = "[" + producerName + "] " + buffer;

	if (consumerListMutex.lock()) {
		std::list<LogConsumer *>::iterator i;
		for (i = consumers.begin(); i != consumers.end(); i++)
			(*i)->logConsume(this, str);
		consumerListMutex.unlock();
	}
}

void LogProducer::log(const cvgString &s) {
	log(s.c_str());
}

}
}
