/*
 * VideoTranscoder.c
 *
 *  Created on: 18/01/2012
 *      Author: Ignacio Mellado
 */

#include "VideoTranscoder.h"
#include "../Codecs/jpeg.h"
#include "Protocol.h"

#define MAX_POSSIBLE_HEADER_LENGTH			1024
#define MAX_POSSIBLE_NUM_PLANES				3
#define MAX_POSSIBLE_BYTES_PER_PLANE		2

#define GET_MAX_ENCODING_LENGTH(width, height)	(width * height * MAX_POSSIBLE_NUM_PLANES * MAX_POSSIBLE_BYTES_PER_PLANE + MAX_POSSIBLE_HEADER_LENGTH + sizeof(VideoFrameHeader))
#define DEFAULT_ENCODING_BUFFER_LENGTH			GET_MAX_ENCODING_LENGTH(640, 480)

uint8_t *encodingBuffer = NULL;
uint32_t encodingBuffer_length = DEFAULT_ENCODING_BUFFER_LENGTH;
volatile double encodingQuality = 1.0;

void videoTranscoder_init() {
	encodingBuffer = vp_os_malloc(encodingBuffer_length);
}

void videoTranscoder_destroy() {
	if (encodingBuffer != NULL) vp_os_free(encodingBuffer);
}

void setupEncodingBuffer(uint32_t width, uint32_t height) {
	uint32_t theoreticalMaxSize = GET_MAX_ENCODING_LENGTH(width, height);
	/* Realloc encoding buffer if it is too small */
	if (theoreticalMaxSize > encodingBuffer_length) {
		vp_os_free(encodingBuffer);
		encodingBuffer_length = theoreticalMaxSize;
		encodingBuffer = vp_os_malloc(encodingBuffer_length);
	}
}

void videoTranscoder_setQuality(double quality) {
	encodingQuality = quality;
}

bool_t videoTranscoder_transcode(void *inputFrame, VideoFormat outputFormat, uint8_t **outputFrame) {
	/* The input buffer is: VideoFrameHeader + image data */
	/* If the input and output encodings are the same, do nothing */
	VideoFrameHeader *inputHeader = (VideoFrameHeader *)inputFrame;
	if (outputFormat == inputHeader->videoData.encoding) {
		(*outputFrame) = (uint8_t *)inputFrame;
		return TRUE;
	} else {
		bool_t supported = FALSE;
		memcpy(encodingBuffer, inputFrame, sizeof(VideoFrameHeader));
		/* Currently, only RAW_BGR24 to JPEG conversion is supported */
		switch(outputFormat) {
		case JPEG:
			if (inputHeader->videoData.encoding == RAW_BGR24) {
				uint32_t width = inputHeader->videoData.width;
				uint32_t height = inputHeader->videoData.height;
				setupEncodingBuffer(width, height);
				((VideoFrameHeader *)encodingBuffer)->videoData.dataLength = jpeg_compress((char *)(&encodingBuffer[sizeof(VideoFrameHeader)]), &((char *)inputFrame)[sizeof(VideoFrameHeader)], width, height, encodingBuffer_length, (int)(encodingQuality * 100));
				((VideoFrameHeader *)encodingBuffer)->videoData.encoding = outputFormat;
				(*outputFrame) = encodingBuffer;
				supported = TRUE;
			}
			break;
		default:
			supported = FALSE;
			break;
		}
		return supported;
	}
}



