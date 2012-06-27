/*
 * FrameGrabber.hh
 *
 *  Created on: 04/06/2012
 *      Author: Ignacio Mellado-Bataller
 */

#define template_def	template<class _VidSourceT>
#define FrameGrabber_	FrameGrabber<_VidSourceT>

namespace Video {

template_def void FrameGrabber_::open() {
	_VidSourceT::open();
	frameBuffer.resize(_VidSourceT::getFrameLength());
	start();
}

template_def void FrameGrabber_::close() {
	stop();
	_VidSourceT::close();
}

template_def void FrameGrabber_::run() {
	try {
		cvg_ulong timestamp;
		if (_VidSourceT::captureFrame(&frameBuffer, &timestamp)) {
			notifyEvent(GotVideoFrameEvent(&frameBuffer, _VidSourceT::getWidth(), _VidSourceT::getHeight(), _VidSourceT::getFrameLength(), _VidSourceT::getFormat(), timestamp, _VidSourceT::getTicksPerSecond()));
		}
	} catch (cvgException &e) {
		std::cout << e.getMessage() << std::endl;
	}
}

}

#undef FrameGrabber_
#undef template_def

