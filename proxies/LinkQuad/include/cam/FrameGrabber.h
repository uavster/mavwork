/*
 * FrameGrabber.h
 *
 *  Created on: 04/06/2012
 *      Author: Ignacio Mellado-Bataller
 */

#ifndef FRAMEGRABBER_H_
#define FRAMEGRABBER_H_

#include <atlante.h>
#include "../base/Event.h"
#include "../base/EventGenerator.h"

namespace Video {

template<class _VidSourceT> class FrameGrabber
	: public virtual _VidSourceT,
	  public virtual cvgThread,
	  public virtual EventGenerator
{
private:
		DynamicBuffer<char> frameBuffer;
protected:
		void run();
public:
		inline FrameGrabber() : cvgThread("FrameGrabber") {}
		inline virtual ~FrameGrabber() {}

		virtual void open();
		virtual void close();
};

}

#include "../../sources/FrameGrabber.hh"

#endif /* FRAMEGRABBER_H_ */
