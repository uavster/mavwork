/*
 * DC1394Cam.h
 *
 *  Created on: 02/10/2012
 *      Author: Ignacio Mellado-Bataller
 */

#ifndef DC1394CAM_H_
#define DC1394CAM_H_

#include "../IVideoSource.h"
#include "DC1394_driver.h"

namespace Video {

class DC1394Cam : public virtual IVideoSource {
public:
	inline DC1394Cam() { monoModesAreRaw = false; }
	inline void takeMonoModesAsRaw(cvg_bool e) { monoModesAreRaw = e; }

	virtual void open();
	virtual void close();

	virtual cvg_int getWidth();
	virtual cvg_int getHeight();
	virtual cvg_int getFrameLength();
	virtual VideoFormat getFormat();
	virtual cvg_ulong getTicksPerSecond();

	virtual cvg_bool captureFrame(void *frameBuffer, cvg_ulong *timestamp = NULL);

private:
	DC1394_driver driver;
	cvg_bool camStarted;
	cvg_bool monoModesAreRaw;
};

}

#endif /* DC1394CAM_H_ */
