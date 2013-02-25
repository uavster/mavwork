/*
 * DC1394_driver.h
 *
 *  Created on: 27/09/2012
 *      Author: Ignacio Mellado-Bataller
 *      Description: This class offers an interface to dc1394 cameras. As it may implement much more
 *      functionalities than the strictly required by the IVideoSource, it is encapsulated in
 *      a separate class for clarity. The provided extra functionalities may be used in the future
 *      as soon as IVideoSource demands them.
 */

#ifndef DC1394_DRIVER_H_
#define DC1394_DRIVER_H_

#include <list>
#include "../IVideoSource.h"
#include <atlante.h>
#include <dc1394/dc1394.h>
#include "../../data/Buffer.h"

namespace Video {

#define DC1394_DRIVER_FPS_ANY	-1
#define DC1394_DRIVER_RES_ANY	-1

class DC1394_driver {
public:
	typedef struct {
		dc1394video_mode_t		id;
		cvg_int					width;
		cvg_int					height;
		dc1394color_coding_t	colorId;
		cvg_bool 				isColor;
	} VideoMode;

	DC1394_driver();
	~DC1394_driver();

	// Open a camera by GUID
	void open(cvg_ulong camGuid, cvg_bool takeMonoAsRaw = false);
	// Open the first camera in the list
	void open(cvg_bool takeMonoAsRaw = false);
	// Close the camera
	void close();
	// List all available camera GUIDs
	void getCameraList(std::list<cvg_ulong> &list);

	// ---- open() must be called before the following methods: ----
	// Get list of video modes
	void getSupportedVideoModes(std::list<VideoMode> &modes);
	// Sets a fixed resolution and format
	void setVideoMode(cvg_int hRes, cvg_int vRes, IVideoSource::VideoFormat format = IVideoSource::ANY_FORMAT, cvg_double minFps = DC1394_DRIVER_FPS_ANY, cvg_double maxFps = DC1394_DRIVER_FPS_ANY);
	// Returns the IVideoSource::Video format of the current video mode
	IVideoSource::VideoFormat getCurrentVideoFormat();
	// Sets resolution and format
	void setVideoMode(cvg_int minHRes, cvg_int minVRes, cvg_int maxHRes, cvg_int maxVRes, IVideoSource::VideoFormat format = IVideoSource::ANY_FORMAT, cvg_double minFps = DC1394_DRIVER_FPS_ANY, cvg_double maxFps = DC1394_DRIVER_FPS_ANY);
	// Get frame size
	void getFrameSize(cvg_int *width, cvg_int *height);
	// Set framerate
	void setFramerate(cvg_double fps);
	// Start capturing
	void startCapture();
	// Stop capturing
	void stopCapture();
	// Set number of memory buffers
	void setNumBuffers(cvg_int numBuffers);
	// Exposure
	void setAutoExposure();
	void setExposure(cvg_uint value);
	// Brightness
	void setAutoBrightness();
	void setBrightness(cvg_uint value);
	// Shutter
	void setAutoShutter();
	void setShutter(cvg_uint value);
	// Gain
	void setAutoGain();
	void setGain(cvg_uint value);

	// Wait until a frame is captured by the camera. It returns true, if a frame was captured successfully.
	cvg_bool waitForFrame(cvg_char *frameData);
	// Check without blocking if there is a new captured frame. It returns true if a new frame was received.
	cvg_bool pollFrame(cvg_char *frameData);

protected:
	void createContext();
	void destroyContext();
	void openByGuid(cvg_ulong guid, cvg_bool takeMonoAsRaw);
	void checkOpenCalled();
	cvg_bool isColorConversionValid(dc1394color_coding_t source, IVideoSource::VideoFormat dest);
	cvg_bool isColorVideoMode(dc1394color_coding_t colorMode);
	cvg_double fpsToDouble(dc1394framerate_t fps);
	cvg_double estimateVideo7Fps(dc1394video_mode_t video7Mode, cvg_int width, cvg_int height, dc1394color_coding_t colorCoding);
	cvg_double getColorCodingBytesPerPixel(dc1394color_coding_t coding);
	void checkSetVideoFormatCalled();
	void convertFrame(uint8_t *source, uint32_t width, uint32_t height, dc1394color_coding_t colorCoding, uint32_t byteOrder, uint8_t *dest);
	void convertFrame(dc1394video_frame_t *source, uint8_t *dest);
	cvgString getColorCodingName(dc1394color_coding_t coding);
	cvg_bool colorCodingToFormat(dc1394color_coding_t coding, IVideoSource::VideoFormat *format);

private:
	static cvg_int numInstances;
	static cvgMutex instanceFactoryMutex;
	static dc1394_t *context;
	cvg_bool contextCreatedInThisInstance;

	dc1394camera_t *cam;
	IVideoSource::VideoFormat desiredFormat;
	cvg_bool setVideoFormatCalled;
	cvg_int numBuffers;

	DynamicBuffer<uint16_t> debayer16Buffer;

	cvg_bool captureStarted;
	cvg_int frameWidth, frameHeight;
	cvg_bool monoModesAreRaw;

	IVideoSource::VideoFormat selectedModeFormat;
};

}

#endif /* DC1394_DRIVER_H_ */
