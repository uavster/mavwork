/*
 * SyncVideoTranscoder.cpp
 *
 *  Created on: 18/10/2012
 *      Author: Ignacio Mellado-Bataller
 */

#include "cam/SyncVideoTranscoder.h"
#include <dc1394/dc1394.h>

namespace Video {

SyncVideoTranscoder::SyncVideoTranscoder() {
	srcIplImage = dstIplImage = midIplImage = NULL;
}

SyncVideoTranscoder::~SyncVideoTranscoder() {
	if (srcIplImage != NULL) {
		cvReleaseImageHeader(&srcIplImage);
		srcIplImage = NULL;
	}
	if (midIplImage != NULL) {
		cvReleaseImage(&midIplImage);
		midIplImage = NULL;
	}
	if (dstIplImage != NULL) {
		cvReleaseImageHeader(&dstIplImage);
		dstIplImage = NULL;
	}
}

cvgString SyncVideoTranscoder::getFormatName(IVideoSource::VideoFormat format) {
	switch(format) {
	case IVideoSource::RAW_BGR24: return "RAW_BGR24";
	case IVideoSource::RAW_RGB24: return "RAW_RGB24";
	case IVideoSource::JPEG: return "JPEG";
	case IVideoSource::RAW_BAYER8: return "BAYER8";
	case IVideoSource::RAW_D16: return "RAW_D16";
	case IVideoSource::ANY_FORMAT: return "ANY_FORMAT";
	default: return "UNKNOWN";
	}
}

cvg_bool SyncVideoTranscoder::canTranscode(IVideoSource::VideoFormat sourceFormat, IVideoSource::VideoFormat destFormat) {
	if (sourceFormat == IVideoSource::ANY_FORMAT)
		throw cvgException("[SyncVideoTranscoder] Source format must be specified");

	if (sourceFormat == destFormat || destFormat == IVideoSource::ANY_FORMAT) return true;

	switch(destFormat) {
	case IVideoSource::RAW_RGB24:
	case IVideoSource::RAW_BGR24:
		if (sourceFormat == IVideoSource::RAW_BAYER8) return true;
		else return false;
	case IVideoSource::RAW_BAYER8:
	case IVideoSource::RAW_D16:
	case IVideoSource::JPEG:
		return false;
	}
}

void SyncVideoTranscoder::transcodeFrame(IVideoSource::VideoFormat sourceFormat, cvg_int sourceWidth, cvg_int sourceHeight, void *sourceData, IVideoSource::VideoFormat destFormat, cvg_int destWidth, cvg_int destHeight, void *destData) {
	cvg_bool sizeChanges = sourceWidth != destWidth || sourceHeight != destHeight;

	if (sourceFormat == destFormat) {
		// Source and dest formats are the same
		if (!sizeChanges) {
			// The size is the same, so just copy the pixels
			int iplDepth, depth, planes;
			getFormatDepthAndPlanes(sourceFormat, &iplDepth, &depth, &planes);
			memcpy(destData, sourceData, sourceWidth * sourceHeight * planes * depth);
		} else {
			// Resize the image if needed
			setupIplImages(sourceFormat, sourceWidth, sourceHeight, sourceData, destFormat, destWidth, destHeight, destData);
			cvResize(srcIplImage, dstIplImage, CV_INTER_NN);
		}
	} else {
		// Different formats, we need to recode
		if (!canTranscode(sourceFormat, destFormat))
			throw cvgException("[SyncVideoTranscoder] Impossible to transcode from " + getFormatName(sourceFormat) + " to " + getFormatName(destFormat));

		setupIplImages(sourceFormat, sourceWidth, sourceHeight, sourceData, destFormat, destWidth, destHeight, destData);

		// If source and dest are the same size, there's no need to use the intermediate buffer
		// If the sizes are different, first do the conversion on the intermediate buffer and then resize on the destination
		IplImage *output = sizeChanges ? midIplImage : dstIplImage;
		if (sourceFormat == IVideoSource::RAW_BAYER8) {
			switch(destFormat) {
			case IVideoSource::RAW_BGR24:
				cvCvtColor(srcIplImage, output, CV_BayerBG2BGR);
				break;
			case IVideoSource::RAW_RGB24:
				//cvCvtColor(srcIplImage, output, CV_BayerBG2RGB);
				// dc1394 with nearest neighbor is faster than OpenCV on Gumstix
//				cvgTimer debayerTimer;
				dc1394_bayer_decoding_8bit((uint8_t *)srcIplImage->imageData, (uint8_t *)output->imageData, srcIplImage->width, srcIplImage->height, DC1394_COLOR_FILTER_RGGB, DC1394_BAYER_METHOD_NEAREST);
//				cvg_double e1 = debayerTimer.getElapsedSeconds();
//				std::cout << e1 << std::endl;
				break;
			}
		}

		if (sizeChanges) {
			// Resize the image if needed
			cvResize(output, dstIplImage, CV_INTER_NN);
		}
	}
}

void SyncVideoTranscoder::getFormatDepthAndPlanes(IVideoSource::VideoFormat format, int *iplDepth, int *depth, int *planes) {
	switch(format) {
	case IVideoSource::RAW_BGR24:
	case IVideoSource::RAW_RGB24:
		*iplDepth = IPL_DEPTH_8U;
		*depth = 1;
		*planes = 3;
		break;
	case IVideoSource::RAW_BAYER8:
		*iplDepth = IPL_DEPTH_8U;
		*depth = 1;
		*planes = 1;
		break;
	case IVideoSource::RAW_D16:
		*iplDepth = IPL_DEPTH_16U;
		*depth = 2;
		*planes = 1;
		break;
	default:
		throw cvgException("[SyncVideoTranscoder::getFormatDepthAndPlanes()] These features have no sense for the " + getFormatName(format) + " format");
	}
}

void SyncVideoTranscoder::setupIplImages(IVideoSource::VideoFormat sourceFormat, cvg_int sourceWidth, cvg_int sourceHeight, void *sourceData, IVideoSource::VideoFormat destFormat, cvg_int destWidth, cvg_int destHeight, void *destData) {
	int srcIplDepth, srcDepth, srcPlanes;
	getFormatDepthAndPlanes(sourceFormat, &srcIplDepth, &srcDepth, &srcPlanes);
	int dstIplDepth, dstDepth, dstPlanes;
	getFormatDepthAndPlanes(destFormat, &dstIplDepth, &dstDepth, &dstPlanes);

	cvg_bool scaleChanged = sourceWidth != destWidth || sourceHeight != destHeight;

	if (srcIplImage != NULL &&
		(
		srcIplImage->width != sourceWidth || srcIplImage->height != sourceHeight ||
		srcIplImage->depth != srcIplDepth || srcIplImage->nChannels != srcPlanes
		)
	) {
		cvReleaseImageHeader(&srcIplImage);
		srcIplImage = NULL;
	}
	if (midIplImage != NULL &&
		(
		midIplImage->width != destWidth || midIplImage->height != destHeight ||
		midIplImage->depth != dstIplDepth || midIplImage->nChannels != dstPlanes
		)
	) {
		cvReleaseImage(&midIplImage);
		midIplImage = NULL;
	}
	if (dstIplImage != NULL &&
		(
		dstIplImage->width != destWidth || dstIplImage->height != destHeight ||
		dstIplImage->depth != dstIplDepth || dstIplImage->nChannels != dstPlanes
		)
	) {
		cvReleaseImageHeader(&dstIplImage);
		dstIplImage = NULL;
	}

	if (srcIplImage == NULL) {
		srcIplImage = cvCreateImageHeader(cvSize(sourceWidth, sourceHeight), srcIplDepth, srcPlanes);
	}
	if (scaleChanged && midIplImage == NULL) {
		midIplImage = cvCreateImage(cvSize(sourceWidth, sourceHeight), dstIplDepth, dstPlanes);
	}
	if (dstIplImage == NULL) {
		dstIplImage = cvCreateImageHeader(cvSize(destWidth, destHeight), dstIplDepth, dstPlanes);
	}

	cvSetData(srcIplImage, sourceData, sourceWidth * srcPlanes * srcDepth);
	cvSetData(dstIplImage, destData, destWidth * dstPlanes * dstDepth);
}

cvg_int SyncVideoTranscoder::getFrameSizeInBytes(IVideoSource::VideoFormat format, cvg_int width, cvg_int height) {
	cvg_int iplDepth, depth, planes;
	getFormatDepthAndPlanes(format, &iplDepth, &depth, &planes);
	return width * height * depth * planes;
}

}
