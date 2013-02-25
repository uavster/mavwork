/*
 * Hokuyo.h
 *
 *  Created on: 18/06/2012
 *      Author: Ignacio Mellado-Bataller
 */

#ifndef HOKUYO_H_
#define HOKUYO_H_

extern "C" {
#include <urg_ctrl.h>
}

#include <atlante.h>

class Hokuyo {
public:
	Hokuyo();
	virtual ~Hokuyo();

	class SampleBuffer {
		friend class Hokuyo;
	public:
		SampleBuffer(cvg_int numBins);
		virtual ~SampleBuffer();
		inline long getSample(int i) const { return buffer[i]; }
		inline cvg_int getLength() { return currentLength; }
		inline long *getDataPtr() { return buffer; }
	private:
		long *buffer;
		cvg_int numBins;
	protected:
		cvg_int currentLength;
		inline cvg_int getMaxLength() { return numBins; }
		inline void setCurrentLength(cvg_int cl) { currentLength = cl; }
	};

	void open(const char *deviceString = "/dev/ttyACM0", cvg_int bps = 115200);
	void close();
	SampleBuffer *createSampleBuffer();
	void destroySampleBuffer(SampleBuffer *sb);
	void read(SampleBuffer *sb, cvg_long *timestampMs);
	cvg_double getFrequency();

protected:
	void outError(const char *str);

private:
	urg_t urg;
	cvg_bool started;
};

#endif /* HOKUYO_H_ */
