/*
 * Vid1Protocol.h
 *
 *  Created on: 22/05/2012
 *      Author: Ignacio Mellado-Bataller
 */

#ifndef VID1PROTOCOL_H_
#define VID1PROTOCOL_H_

#ifndef __PACKED
#define __PACKED	__attribute__((__packed__))
#endif

#include "../../presentation/Presentation.h"

namespace Comm { namespace Proto { namespace Vid1 {

#define PROTOCOL_VIDEOFRAME_SIGNATURE	0x91C7

typedef enum { RAW_BGR24 = 0, RAW_RGB24 = 1, JPEG = 2, RAW_D13 = 3 } VideoFormat;

struct Vid1FrameHeader__ {
	unsigned short	signature;
	struct videoData__ {
		unsigned int	timeCodeH;
		unsigned int	timeCodeL;
		char			encoding;
		unsigned int	width;
		unsigned int	height;
		unsigned int	dataLength;
	} __PACKED videoData;

	void hton() {
		Presentation::hton(&signature);
		Presentation::hton(&videoData.timeCodeH); Presentation::hton(&videoData.timeCodeL);
		Presentation::hton(&videoData.encoding); Presentation::hton(&videoData.width);
		Presentation::hton(&videoData.height); Presentation::hton(&videoData.dataLength);
	}

	void ntoh() {
		Presentation::ntoh(&signature);
		Presentation::ntoh(&videoData.timeCodeH); Presentation::ntoh(&videoData.timeCodeL);
		Presentation::ntoh(&videoData.encoding); Presentation::ntoh(&videoData.width);
		Presentation::ntoh(&videoData.height); Presentation::ntoh(&videoData.dataLength);
	}

} __PACKED;

typedef struct Vid1FrameHeader__ Vid1FrameHeader;

}}}

#endif /* VID1PROTOCOL_H_ */
