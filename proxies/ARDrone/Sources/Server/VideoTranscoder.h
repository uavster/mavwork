/*
 * VideoTranscoder.h
 *
 *  Created on: 18/01/2012
 *      Author: Ignacio Mellado
 */

#ifndef VIDEOTRANSCODER_H_
#define VIDEOTRANSCODER_H_

#include <ardrone_api.h>
#include "Protocol.h"

void videoTranscoder_init();
void videoTranscoder_destroy();

bool_t videoTranscoder_transcode(void *inputFrame, VideoFormat outputFormat, uint8_t **outputFrame);
void videoTranscoder_setQuality(double quality);

#endif /* VIDEOTRANSCODER_H_ */
