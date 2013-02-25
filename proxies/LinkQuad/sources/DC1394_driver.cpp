/*
 * DC1394_driver.cpp
 *
 *  Created on: 27/09/2012
 *      Author: Ignacio Mellado-Bataller
 */
#include <iostream>
#include "cam/dc1394/DC1394_driver.h"

#define DEFAULT_NUM_BUFFERS			10
#define BAYER_COLOR_FILTER_TILE		DC1394_COLOR_FILTER_RGGB
#define BAYER_METHOD				DC1394_BAYER_METHOD_NEAREST
#define DEFAULT_YUV_BYTE_ORDER		DC1394_BYTE_ORDER_UYVY

namespace Video {

cvg_int DC1394_driver::numInstances = 0;
cvgMutex DC1394_driver::instanceFactoryMutex;
dc1394_t *DC1394_driver::context = NULL;

DC1394_driver::DC1394_driver() {
	cam = NULL;
	contextCreatedInThisInstance = false;
	setVideoFormatCalled = false;
	numBuffers = DEFAULT_NUM_BUFFERS;
	captureStarted = false;
}

DC1394_driver::~DC1394_driver() {
	close();
}

void DC1394_driver::createContext() {
	// Create context to access the driver
	if (instanceFactoryMutex.lock()) {
		if (numInstances == 0) {
			context = dc1394_new();
		}
		if (!contextCreatedInThisInstance) {
			numInstances++;
			contextCreatedInThisInstance = true;
		}
		instanceFactoryMutex.unlock();
	} else throw cvgException("[DC1394_driver] Unable to lock instance factory mutex");
}

void DC1394_driver::destroyContext() {
	// Destroy context
	if (instanceFactoryMutex.lock()) {
		if (contextCreatedInThisInstance && numInstances > 0) {
			numInstances--;
			contextCreatedInThisInstance = false;
		}
		if (numInstances == 0 && context != NULL) {
			dc1394_free(context);
			context = NULL;
		}
		instanceFactoryMutex.unlock();
	} else throw cvgException("[DC1394_driver] Unable to lock instance factory mutex");
}

void DC1394_driver::openByGuid(cvg_ulong guid, cvg_bool takeMonoAsRaw) {
	monoModesAreRaw = takeMonoAsRaw;
	cam = dc1394_camera_new(context, guid);
	if (cam == NULL) throw cvgException(cvgString("[DC1394_driver] Unable to open camera ") + guid);
}

// Open a camera by GUID
void DC1394_driver::open(cvg_ulong camGuid, cvg_bool takeMonoAsRaw) {
	close();
	createContext();
	openByGuid(camGuid, takeMonoAsRaw);
}

// Open the first camera in the list
void DC1394_driver::open(cvg_bool takeMonoAsRaw) {
	close();
	createContext();
	// Get camera list
	std::list<cvg_ulong> camList;
	getCameraList(camList);
	if (camList.size() == 0)
		throw cvgException("[DC1394_driver] No available cameras to open");
	// Open the first camera
	openByGuid(camList.front(), takeMonoAsRaw);
}

// Close the camera
void DC1394_driver::close() {
	if (cam != NULL) {
		stopCapture();
		dc1394_camera_free(cam);
		cam = NULL;
	}
	destroyContext();
}

void DC1394_driver::checkOpenCalled() {
	if (cam == NULL) throw cvgException("[DC1394_driver] open() must be called before any other operation");
}

void DC1394_driver::checkSetVideoFormatCalled() {
	if (!setVideoFormatCalled) throw cvgException("[DC1394_driver] setVideoFormat() must be called before this operation");
}

// List all available camera GUIDs
void DC1394_driver::getCameraList(std::list<cvg_ulong> &outList) {
	createContext();	// createContext() is only done if it was never called from this class before
	dc1394camera_list_t *cList;
	if (dc1394_camera_enumerate(context, &cList) != DC1394_SUCCESS)
		throw cvgException("[DC1394_driver] Unable to get camera list");
	outList.clear();
	// Copy GUIDs to user list
	for (cvg_int i = 0; i < cList->num; i++)
		outList.push_back(cList->ids[i].guid);
	dc1394_camera_free_list(cList);
	// destroyContext() is always done per-class if there was some previous call to openContext().
	// So, we don't call it here, as we could harm the object if it was open()ed
}

// Get list of video modes
void DC1394_driver::getSupportedVideoModes(std::list<VideoMode> &modeList) {
	checkOpenCalled();
    dc1394video_modes_t modes;
    if (dc1394_video_get_supported_modes(cam, &modes) != DC1394_SUCCESS)
    	throw cvgException("[DC1394_driver] Unable to get video mode list");

    modeList.clear();
    for (cvg_int i = 0; i < modes.num; i++) {
    	VideoMode vm;
    	vm.id = modes.modes[i];
    	uint32_t w, h;
    	if (dc1394_get_image_size_from_video_mode(cam, modes.modes[i], &w, &h) != DC1394_SUCCESS) continue;
    	vm.width = w; vm.height = h;
    	if (dc1394_get_color_coding_from_video_mode(cam, modes.modes[i], &vm.colorId) != DC1394_SUCCESS) continue;
    	vm.isColor = isColorVideoMode(vm.colorId);
    	modeList.push_back(vm);
    }
}

cvg_bool DC1394_driver::isColorVideoMode(dc1394color_coding_t colorMode) {
	switch(colorMode) {
	case DC1394_COLOR_CODING_MONO8:
	case DC1394_COLOR_CODING_MONO16:
	case DC1394_COLOR_CODING_MONO16S:
		return monoModesAreRaw;
	case DC1394_COLOR_CODING_YUV411:
	case DC1394_COLOR_CODING_YUV422:
	case DC1394_COLOR_CODING_YUV444:
	case DC1394_COLOR_CODING_RGB8:
	case DC1394_COLOR_CODING_RGB16:
	case DC1394_COLOR_CODING_RGB16S:
	case DC1394_COLOR_CODING_RAW8:
	case DC1394_COLOR_CODING_RAW16:
	default:
		return true;
	}
}

cvg_bool DC1394_driver::isColorConversionValid(dc1394color_coding_t source, IVideoSource::VideoFormat dest) {
	if (!isColorVideoMode(source)) {
		// All possible destination color formats right now are colored and there's
		// no possible conversion from BW to color.
		return false;
	} else {
		// Check if there is a possible conversion from source to destination color codings
		// No JPEG compression or RAW_D16 supported here
		if (dest == IVideoSource::JPEG || dest == IVideoSource::RAW_D16) return false;
		// From here, dest is RAW_RGB24, RAW_BGR24 or RAW_BAYER8
		// Dummy 1-pixel images
		uint8_t srcData[32]; // No image format has 32 bytes per pixel
		uint8_t destData[3]; // Destination format is RGB8: 3 bytes per pixel
		// Check if the conversion works
		try {
			// TODO: get actual capture mode yuv order
			convertFrame(srcData, 1, 1, source, DEFAULT_YUV_BYTE_ORDER, destData);
		} catch (cvgException &e) {
			return false;
		}
		return true;
	}
}

cvg_double DC1394_driver::fpsToDouble(dc1394framerate_t fps) {
	switch(fps) {
	case DC1394_FRAMERATE_1_875: return 1.875;
	case DC1394_FRAMERATE_3_75: return 3.75;
	case DC1394_FRAMERATE_7_5: return 7.5;
	case DC1394_FRAMERATE_15: return 15.0;
	case DC1394_FRAMERATE_30: return 30.0;
	case DC1394_FRAMERATE_60: return 60.0;
	case DC1394_FRAMERATE_120: return 120.0;
	case DC1394_FRAMERATE_240: return 240.0;
	default: return -1;
	}
}

cvg_double DC1394_driver::getColorCodingBytesPerPixel(dc1394color_coding_t coding) {
	switch(coding) {
	case DC1394_COLOR_CODING_MONO8:
	case DC1394_COLOR_CODING_RAW8:
		return 1.0;
	case DC1394_COLOR_CODING_MONO16:
	case DC1394_COLOR_CODING_MONO16S:
	case DC1394_COLOR_CODING_RAW16:
	case DC1394_COLOR_CODING_YUV422:
		return 2.0;
	case DC1394_COLOR_CODING_YUV411:
		return 6.0 / 4.0;
	case DC1394_COLOR_CODING_YUV444:
	case DC1394_COLOR_CODING_RGB8:
		return 3.0;
	case DC1394_COLOR_CODING_RGB16:
	case DC1394_COLOR_CODING_RGB16S:
		return 6.0;
	default:
		return -1.0;
	}
}

cvgString DC1394_driver::getColorCodingName(dc1394color_coding_t coding) {
	switch(coding) {
	case DC1394_COLOR_CODING_MONO8: return "MONO8";
	case DC1394_COLOR_CODING_RAW8: return "RAW8";
	case DC1394_COLOR_CODING_MONO16: return "MONO16";
	case DC1394_COLOR_CODING_MONO16S: return "MONO16S";
	case DC1394_COLOR_CODING_RAW16: return "RAW16";
	case DC1394_COLOR_CODING_YUV422: return "YUV422";
	case DC1394_COLOR_CODING_YUV411: return "YUV411";
	case DC1394_COLOR_CODING_YUV444: return "YUV444";
	case DC1394_COLOR_CODING_RGB8: return "RGB8";
	case DC1394_COLOR_CODING_RGB16: return "RGB16";
	case DC1394_COLOR_CODING_RGB16S: return "RGB16S";
	default: return "UNKNOWN";
	}
}

cvg_double DC1394_driver::estimateVideo7Fps(dc1394video_mode_t video7Mode, cvg_int width, cvg_int height, dc1394color_coding_t colorCoding) {
	// Taken from http://forums.ni.com/t5/Machine-Vision/Calculating-image-acquisition-frame-rate-using-Format-7/td-p/1540596
	// TransferredFramesPerSecond = (BytesPerPacket * 8000) / (ImageWidth * ImageHeight * BytesPerPixel)
	// The TransferredFramesPerSecond is slightly higher than the time it takes to capture the frame.
	// So, the framerate should be a bit lower than TransferredFramesPerSecond.
	uint32_t packetBytes;
	if (dc1394_format7_get_packet_size(cam, video7Mode, &packetBytes) != DC1394_SUCCESS) return 0.0;
	return (packetBytes * 8000) / (cvg_double)(width * height * getColorCodingBytesPerPixel(colorCoding));
}

// Get frame size
void DC1394_driver::getFrameSize(cvg_int *width, cvg_int *height) {
	checkOpenCalled();
	checkSetVideoFormatCalled();
	if (width != NULL) (*width) = frameWidth;
	if (height != NULL) (*height) = frameHeight;
}

// Sets a fixed resolution and format
void DC1394_driver::setVideoMode(cvg_int hRes, cvg_int vRes, IVideoSource::VideoFormat format, cvg_double minFps, cvg_double maxFps) {
	setVideoMode(hRes, vRes, hRes, vRes, format, minFps, maxFps);
}

// open() must be called before all these methods
// Sets resoution and format
void DC1394_driver::setVideoMode(cvg_int minHRes, cvg_int minVRes, cvg_int maxHRes, cvg_int maxVRes, IVideoSource::VideoFormat format, cvg_double minFps, cvg_double maxFps) {
	checkOpenCalled();

	// Find a suitable video mode
	std::list<VideoMode> modes;
	getSupportedVideoModes(modes);
	// Remove video mode that don't meet the requirements
	std::list<VideoMode>::iterator it;
	for (it = modes.begin(); it != modes.end(); ) {
		// Reasons to discard a mode: the resolution is out of the allowed range or
		// there is no possible conversion between mode color format and desired color format
		if (it->width < minHRes || it->height < minVRes ||
			it->width > maxHRes || it->height > maxVRes ||
			(format != IVideoSource::ANY_FORMAT && !isColorConversionValid(it->colorId, format)) ||
			(format == IVideoSource::ANY_FORMAT && !colorCodingToFormat(it->colorId, NULL))
			)
		{
			it = modes.erase(it);
		} else it++;
	}
	if (modes.size() == 0)
		throw cvgException("[DC1394_driver] No available modes meet the required resolution range and color format");

	// Select one of the compliant video modes: the one with highest possible framerate inside the valid range
	cvg_double highestFps = 0;
	dc1394video_mode_t selectedMode;
	dc1394color_coding_t selectedModeColor;
	for (it = modes.begin(); it != modes.end(); it++) {
		if (it->id == DC1394_VIDEO_MODE_FORMAT7_0 ||
			it->id == DC1394_VIDEO_MODE_FORMAT7_1 ||
			it->id == DC1394_VIDEO_MODE_FORMAT7_2 ||
			it->id == DC1394_VIDEO_MODE_FORMAT7_3 ||
			it->id == DC1394_VIDEO_MODE_FORMAT7_4 ||
			it->id == DC1394_VIDEO_MODE_FORMAT7_5 ||
			it->id == DC1394_VIDEO_MODE_FORMAT7_6
			)
		{
/*			float timeInterval = 0.0f;
			if (dc1394_format7_get_frame_interval(cam, it->id, &timeInterval) == DC1394_SUCCESS) {
				cvg_double format7Fps = 1.0 / timeInterval;
			*/
			cvg_double format7Fps = estimateVideo7Fps(it->id, it->width, it->height, it->colorId);
			if (format7Fps >= 0.0) {
				if (format7Fps > highestFps &&
					(minFps == DC1394_DRIVER_FPS_ANY || format7Fps >= minFps) &&
					(maxFps == DC1394_DRIVER_FPS_ANY || format7Fps <= maxFps)
					)
				{
					highestFps = format7Fps;
					selectedMode = it->id;
					selectedModeColor = it->colorId;
					frameWidth = it->width;
					frameHeight = it->height;
				}
			}
		} else {
			dc1394framerates_t framerates;
			if (dc1394_video_get_supported_framerates(cam, it->id, &framerates) == DC1394_SUCCESS) {
				for (cvg_int i = 0; i < framerates.num; i++) {
					cvg_double modeFps = fpsToDouble(framerates.framerates[i]);
					if (modeFps >= 0 &&
						modeFps > highestFps &&
						(minFps == DC1394_DRIVER_FPS_ANY || modeFps >= minFps) &&
						(maxFps == DC1394_DRIVER_FPS_ANY || modeFps <= maxFps)
						)
					{
						highestFps = modeFps;
						selectedMode = it->id;
						selectedModeColor = it->colorId;
						frameWidth = it->width;
						frameHeight = it->height;
					}
				}
			}
		}
	}
	if (highestFps == 0)
		throw cvgException("[DC1394_driver] No video modes with the required resolution and color format are in the required FPS range");

	// Set selected mode
    if (dc1394_video_set_mode(cam, selectedMode) != DC1394_SUCCESS)
    	throw cvgException("[DC1394_driver] Unable to set the selected video mode");

std::cout << "Video mode: " << selectedMode << std::endl;
    desiredFormat = format;

    if (desiredFormat != IVideoSource::ANY_FORMAT) {
    	selectedModeFormat = desiredFormat;
    } else {
    	colorCodingToFormat(selectedModeColor, &selectedModeFormat);
    }

    setVideoFormatCalled = true;
}

// Set framerate
void DC1394_driver::setFramerate(cvg_double fps) {
	checkOpenCalled();

    cvg_double rates[] = { 1.875, 3.75, 7.5, 15, 30, 60, 120, 240 };
    dc1394framerate_t dc1394Rates[] = {
    						DC1394_FRAMERATE_1_875, DC1394_FRAMERATE_3_75, DC1394_FRAMERATE_7_5,
    						DC1394_FRAMERATE_15, DC1394_FRAMERATE_30, DC1394_FRAMERATE_60,
    						DC1394_FRAMERATE_120, DC1394_FRAMERATE_240
    						};

    dc1394framerate_t dc1394Fps;
    cvg_int n = sizeof(rates) / sizeof(cvg_double);
    for (cvg_int i = 0; i < n; i++) {
    	cvg_double lowerBound = i > 0 ? ((rates[i - 1] + rates[i]) / 2) : 0.0;
    	cvg_double higherBound = i < n - 1 ? ((rates[i + 1] + rates[i]) / 2) : 1e6;
    	if (fps >= lowerBound && fps < higherBound) dc1394Fps = dc1394Rates[i];
    }

    if (dc1394_video_set_framerate(cam, dc1394Fps) != DC1394_SUCCESS)
    	throw cvgException("[DC1394_driver] Unable to set framerate");
}

// Set number of memory buffers
void DC1394_driver::setNumBuffers(cvg_int numBuffers) {
	checkOpenCalled();
	this->numBuffers = numBuffers;
}

// Exposure
void DC1394_driver::setAutoExposure() {
	checkOpenCalled();

    if (dc1394_feature_set_power(cam, DC1394_FEATURE_EXPOSURE, DC1394_ON) != DC1394_SUCCESS)
    	throw cvgException("[DC1394_driver] Cannot enable exposure feature");
    if (dc1394_feature_set_mode(cam, DC1394_FEATURE_EXPOSURE, DC1394_FEATURE_MODE_AUTO) != DC1394_SUCCESS)
    	throw cvgException("[DC1394_driver] Unable to set automatic exposure");
}

void DC1394_driver::setExposure(cvg_uint value) {
	checkOpenCalled();

    if (dc1394_feature_set_power(cam, DC1394_FEATURE_EXPOSURE, DC1394_ON) != DC1394_SUCCESS)
    	throw cvgException("[DC1394_driver] Cannot enable exposure feature");
    if (dc1394_feature_set_mode(cam, DC1394_FEATURE_EXPOSURE, DC1394_FEATURE_MODE_MANUAL) != DC1394_SUCCESS)
    	throw cvgException("[DC1394_driver] Unable to set manual exposure");
    if (dc1394_feature_set_value(cam, DC1394_FEATURE_EXPOSURE, value) != DC1394_SUCCESS)
    	throw cvgException("[DC1394_driver] Cannot set exposure value");
}

// Brightness
void DC1394_driver::setAutoBrightness() {
	checkOpenCalled();

    if (dc1394_feature_set_power(cam, DC1394_FEATURE_BRIGHTNESS, DC1394_ON) != DC1394_SUCCESS)
    	throw cvgException("[DC1394_driver] Cannot enable brightness feature");
    if (dc1394_feature_set_mode(cam, DC1394_FEATURE_BRIGHTNESS, DC1394_FEATURE_MODE_AUTO) != DC1394_SUCCESS)
    	throw cvgException("[DC1394_driver] Unable to set automatic brightness");
}

void DC1394_driver::setBrightness(cvg_uint value) {
	checkOpenCalled();

    if (dc1394_feature_set_power(cam, DC1394_FEATURE_BRIGHTNESS, DC1394_ON) != DC1394_SUCCESS)
    	throw cvgException("[DC1394_driver] Cannot enable brightness feature");
    if (dc1394_feature_set_mode(cam, DC1394_FEATURE_BRIGHTNESS, DC1394_FEATURE_MODE_MANUAL) != DC1394_SUCCESS)
    	throw cvgException("[DC1394_driver] Unable to set manual brightness");
    if (dc1394_feature_set_value(cam, DC1394_FEATURE_BRIGHTNESS, value) != DC1394_SUCCESS)
    	throw cvgException("[DC1394_driver] Cannot set brightness value");
}

// Shutter
void DC1394_driver::setAutoShutter() {
	checkOpenCalled();

    if (dc1394_feature_set_power(cam, DC1394_FEATURE_SHUTTER, DC1394_ON) != DC1394_SUCCESS)
    	throw cvgException("[DC1394_driver] Cannot enable shutter feature");
    if (dc1394_feature_set_mode(cam, DC1394_FEATURE_SHUTTER, DC1394_FEATURE_MODE_AUTO) != DC1394_SUCCESS)
    	throw cvgException("[DC1394_driver] Unable to set automatic shutter");
}

void DC1394_driver::setShutter(cvg_uint value) {
	checkOpenCalled();

    if (dc1394_feature_set_power(cam, DC1394_FEATURE_SHUTTER, DC1394_ON) != DC1394_SUCCESS)
    	throw cvgException("[DC1394_driver] Cannot enable shutter feature");
    if (dc1394_feature_set_mode(cam, DC1394_FEATURE_SHUTTER, DC1394_FEATURE_MODE_MANUAL) != DC1394_SUCCESS)
    	throw cvgException("[DC1394_driver] Unable to set manual shutter");
    if (dc1394_feature_set_value(cam, DC1394_FEATURE_SHUTTER, value) != DC1394_SUCCESS)
    	throw cvgException("[DC1394_driver] Cannot set shutter value");
}

// Gain
void DC1394_driver::setAutoGain() {
	checkOpenCalled();

    if (dc1394_feature_set_power(cam, DC1394_FEATURE_GAIN, DC1394_ON) != DC1394_SUCCESS)
    	throw cvgException("[DC1394_driver] Cannot enable gain feature");
    if (dc1394_feature_set_mode(cam, DC1394_FEATURE_GAIN, DC1394_FEATURE_MODE_AUTO) != DC1394_SUCCESS)
    	throw cvgException("[DC1394_driver] Unable to set automatic gain");
}

void DC1394_driver::setGain(cvg_uint value) {
	checkOpenCalled();

    if (dc1394_feature_set_power(cam, DC1394_FEATURE_GAIN, DC1394_ON) != DC1394_SUCCESS)
    	throw cvgException("[DC1394_driver] Cannot enable shutter gain");
    if (dc1394_feature_set_mode(cam, DC1394_FEATURE_GAIN, DC1394_FEATURE_MODE_MANUAL) != DC1394_SUCCESS)
    	throw cvgException("[DC1394_driver] Unable to set manual gain");
    if (dc1394_feature_set_value(cam, DC1394_FEATURE_GAIN, value) != DC1394_SUCCESS)
    	throw cvgException("[DC1394_driver] Cannot set gain value");
}

void DC1394_driver::convertFrame(dc1394video_frame_t *source, uint8_t *dest) {
	convertFrame(source->image, source->size[0], source->size[1], source->color_coding, source->yuv_byte_order, dest);
}

void DC1394_driver::convertFrame(uint8_t *srcData, uint32_t srcWidth, uint32_t srcHeight, dc1394color_coding_t srcColorCoding, uint32_t srcByteOrder, uint8_t *dest) {
	// Only RAW_RGB24, RAW_BGR24 and RAW_BAYER8 are allowed for the desired output format
	if (srcColorCoding == DC1394_COLOR_CODING_RAW8 || (monoModesAreRaw && srcColorCoding == DC1394_COLOR_CODING_MONO8)) {
		if (desiredFormat != IVideoSource::RAW_BAYER8 &&
			dc1394_bayer_decoding_8bit(srcData, dest, srcWidth, srcHeight, BAYER_COLOR_FILTER_TILE, BAYER_METHOD) != DC1394_SUCCESS)
			throw cvgException("[DC1394_driver] Cannot debayer RAW8 image");
	} else if (srcColorCoding == DC1394_COLOR_CODING_RAW16 || (monoModesAreRaw && srcColorCoding == DC1394_COLOR_CODING_MONO16)) {
		if (desiredFormat != IVideoSource::RAW_BAYER8) {
			debayer16Buffer.resize(srcWidth * srcHeight * 3);
			if (dc1394_bayer_decoding_16bit((uint16_t *)srcData, (uint16_t *)&debayer16Buffer, srcWidth, srcHeight, BAYER_COLOR_FILTER_TILE, BAYER_METHOD, 16) != DC1394_SUCCESS)
				throw cvgException("[DC1394_driver] Cannot debayer RAW16 image");
			for (cvg_int i = 0; i < srcWidth * srcHeight * 3;i ++)
				dest[i] = debayer16Buffer[i] >> 1;
		} else {
			uint16_t *inputBuffer = (uint16_t *)srcData;
			for (cvg_int i = 0; i < srcWidth * srcHeight * 3; i++)
				dest[i] = inputBuffer[i] >> 1;
		}
	} else {
		if (desiredFormat != IVideoSource::RAW_BAYER8) {
			if (dc1394_convert_to_RGB8(srcData, dest, srcWidth, srcHeight, srcByteOrder, srcColorCoding, 16) != DC1394_SUCCESS)
				throw cvgException("[DC1394_driver] Cannot convert captured frame to the desired format");
		} else throw cvgException("[DC1394_driver] Conversion from " + getColorCodingName(srcColorCoding) + " to BAYER8 is not supported");
	}

	if (desiredFormat == IVideoSource::RAW_BGR24 &&
		isColorVideoMode(srcColorCoding)
		)
	{
		// Flip red and blue channels
		cvg_char tmp;
		for (cvg_int i = 0; i < srcWidth * srcHeight * 3; i += 3) {
			tmp = dest[i];
			dest[i] = dest[i + 2];
			dest[i + 2] = tmp;
		}
	}
}

// Wait until a frame is captured by the camera. It returns true, if a frame was captured successfully.
cvg_bool DC1394_driver::waitForFrame(cvg_char *frameData) {
	checkOpenCalled();
	checkSetVideoFormatCalled();

	dc1394video_frame_t *frame = NULL;
	while(frame == NULL || frame->frames_behind > 0) {
		if (dc1394_capture_dequeue(cam, DC1394_CAPTURE_POLICY_WAIT, &frame) != DC1394_SUCCESS)
			return false;
	//		throw cvgException("[DC1394_driver] waitForFrame(): Cannot capture frame");

		if (frame == NULL) return false;

		if (dc1394_capture_is_frame_corrupt(cam, frame) == DC1394_TRUE) {
			if (dc1394_capture_enqueue(cam, frame) != DC1394_SUCCESS)
				throw cvgException("[DC1394_driver] Cannot enqueue the frame back");
			return false;
	//		throw cvgException("[DC1394_driver] waitForFrame(): The captured frame is corrupt");
		}

		if (frame->frames_behind == 0) {
			try {
				if (desiredFormat == IVideoSource::ANY_FORMAT)
					memcpy(frameData, frame->image, frame->total_bytes);
				else
					convertFrame(frame, (uint8_t *)frameData);
			} catch (cvgException &e) {
				if (dc1394_capture_enqueue(cam, frame) != DC1394_SUCCESS)
					throw cvgException("[DC1394_driver] Cannot enqueue the frame back");
				return false;
			}
		}

		if (dc1394_capture_enqueue(cam, frame) != DC1394_SUCCESS)
			throw cvgException("[DC1394_driver] Cannot enqueue the frame back");
	}

	return true;
}

// Check without blocking if there is a new captured frame. It returns true if a new frame was received.
cvg_bool DC1394_driver::pollFrame(char *frameData) {
	checkOpenCalled();
	checkSetVideoFormatCalled();

	dc1394video_frame_t *frame = NULL;
	while(frame == NULL || frame->frames_behind > 0) {
		if (dc1394_capture_dequeue(cam, DC1394_CAPTURE_POLICY_POLL, &frame) != DC1394_SUCCESS)
			return false;
	//		throw cvgException("[DC1394_driver] waitForFrame(): Cannot capture frame");

		if (frame == NULL) return false;

		if (dc1394_capture_is_frame_corrupt(cam, frame) == DC1394_TRUE) {
			if (dc1394_capture_enqueue(cam, frame) != DC1394_SUCCESS)
				throw cvgException("[DC1394_driver] Cannot enqueue the frame back");
			return false;
	//		throw cvgException("[DC1394_driver] waitForFrame(): The captured frame is corrupt");
		}

		if (frame->frames_behind == 0) {
			try {
				if (desiredFormat == IVideoSource::ANY_FORMAT)
					memcpy(frameData, frame->image, frame->total_bytes);
				else
					convertFrame(frame, (uint8_t *)frameData);
			} catch (cvgException &e) {
				if (dc1394_capture_enqueue(cam, frame) != DC1394_SUCCESS)
					throw cvgException("[DC1394_driver] Cannot enqueue the frame back");
				return false;
			}
		}

		if (dc1394_capture_enqueue(cam, frame) != DC1394_SUCCESS)
			throw cvgException("[DC1394_driver] Cannot enqueue the frame back");
	}

	return true;
}

void DC1394_driver::startCapture() {
	checkOpenCalled();
	checkSetVideoFormatCalled();
	if (captureStarted) return;

    if (dc1394_capture_setup (cam, numBuffers, DC1394_CAPTURE_FLAGS_DEFAULT))
    	throw cvgException("[DC1394_driver] Unable to setup capture");
    if (dc1394_video_set_transmission(cam, DC1394_ON) != DC1394_SUCCESS) {
    	dc1394_capture_stop(cam);
    	throw cvgException("[DC1394_driver] Cannot switch on transmission");
    }
    captureStarted = true;

    // Capture some frames to prevent corruption
/*    int i = 10;
    dc1394video_frame_t *frame;
    while (i--) {
        dc1394_capture_dequeue(cam, DC1394_CAPTURE_POLICY_WAIT, &frame);
        dc1394_capture_enqueue(cam, frame);
    }*/
}

void DC1394_driver::stopCapture() {
	checkOpenCalled();
	if (!captureStarted) return;

    if (dc1394_video_set_transmission(cam, DC1394_OFF) != DC1394_SUCCESS)
    	throw cvgException("[DC1394_driver] Cannot switch off transmission");
    if (dc1394_capture_stop(cam) != DC1394_SUCCESS)
    	throw cvgException("[DC1394_driver] Cannot stop capture");

    captureStarted = false;
}

IVideoSource::VideoFormat DC1394_driver::getCurrentVideoFormat() {
	return selectedModeFormat;
}

cvg_bool DC1394_driver::colorCodingToFormat(dc1394color_coding_t coding, IVideoSource::VideoFormat *format) {
	checkOpenCalled();

	cvg_bool isValid = false;
	switch(coding) {
	case DC1394_COLOR_CODING_MONO8:
		if (monoModesAreRaw) {
			isValid = true;
			if (format != NULL) *format = IVideoSource::RAW_BAYER8;
		}
		break;
	case DC1394_COLOR_CODING_RAW8:
		if (format != NULL) *format = IVideoSource::RAW_BAYER8;
		isValid = true;
		break;
	case DC1394_COLOR_CODING_RGB8:
		if (format != NULL) *format = IVideoSource::RAW_RGB24;
		isValid = true;
		break;
/*	case DC1394_COLOR_CODING_MONO16: isValid = false; break;
	case DC1394_COLOR_CODING_MONO16S: isValid = false; break;
	case DC1394_COLOR_CODING_RAW16: isValid = false; break;
	case DC1394_COLOR_CODING_YUV422: isValid = false; break;
	case DC1394_COLOR_CODING_YUV411: isValid = false; break;
	case DC1394_COLOR_CODING_YUV444: isValid = false; break;
	case DC1394_COLOR_CODING_RGB16: isValid = false; break;
	case DC1394_COLOR_CODING_RGB16S: isValid = false; break;
	default:
	*/
	}
	return isValid;
}

}
