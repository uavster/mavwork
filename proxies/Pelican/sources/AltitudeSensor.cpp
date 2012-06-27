/*
 * AltitudeSensor.cpp
 *
 *  Created on: 01/06/2012
 *      Author: Ignacio Mellado-Bataller
 */
#include <iostream>
#define NUM_SENSORS			2
#define CORRECTION_FACTOR	(0.8751426989411 * 1.296e-2)
#define CORRECTION_OFFSET	-0.00713542556749374
#define GATE_INPUT_ID		0
#define GATE_ENABLE_ID		1

#define GATE_INPUT_ON		1
#define GATE_INPUT_OFF		0

#define GATE_ENABLE_ON		0
#define GATE_ENABLE_OFF		1

#include "mav/pelican/AltitudeSensor.h"
#include <atlante.h>

AltitudeSensor::AltitudeSensor() {
	isOpen = false;
}

AltitudeSensor::~AltitudeSensor() {
	close();
}

void AltitudeSensor::open() {
	close();
	handle = 0;

	// No error codes are managed in the example given by the manufacturer
	CPhidgetInterfaceKit_create(&handle);
	CPhidget_open((CPhidgetHandle)handle, -1);

	// Wait 0.5 s for attachment
	int err;
	if((err = CPhidget_waitForAttachment((CPhidgetHandle)handle, 500)) != EPHIDGET_OK )
	{
		const char *errStr;
		CPhidget_getErrorDescription(err, &errStr);
		throw cvgException(cvgString("Error waiting for attachment: (") + err + ") " + errStr);
	}
	// Disable trigger thresholds
	for (int i = 0; i < NUM_SENSORS; i++)
		CPhidgetInterfaceKit_setSensorChangeTrigger(handle, i, 0);
	// Let's assume it's been more than 250 ms since power-up
	// Kick-start: set RX high for at least 20us; then, high impedance
	// For this purpose, an custom circuit with a 3-state gate has been implemented
	CPhidgetInterfaceKit_setOutputState(handle, GATE_INPUT_ID, GATE_INPUT_ON);	// A = 0
	CPhidgetInterfaceKit_setOutputState(handle, GATE_ENABLE_ID, GATE_ENABLE_ON);	// Normal Z
	usleep(100);
	CPhidgetInterfaceKit_setOutputState(handle, GATE_ENABLE_ID, GATE_ENABLE_OFF);	// High Z
	isOpen = true;
}

void AltitudeSensor::close() {
	if (isOpen) {
		CPhidget_close((CPhidgetHandle)handle);
		CPhidget_delete((CPhidgetHandle)handle);
		isOpen = false;
	}
}

double AltitudeSensor::getAltitude() {
	cvg_double acum = 0.0;
	for(int i = 0; i < NUM_SENSORS; i++) {
		int value;
		CPhidgetInterfaceKit_getSensorValue(handle, i, &value);
		acum += (CORRECTION_FACTOR * value + CORRECTION_OFFSET);
	}
	return (double)acum / NUM_SENSORS;
}

