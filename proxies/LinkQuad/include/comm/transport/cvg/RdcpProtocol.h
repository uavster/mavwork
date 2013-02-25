/*
 * RdcpProtocol.h
 *
 *  Created on: 05/05/2012
 *      Author: Ignacio Mellado-Bataller
 */

#ifndef RDCPPROTOCOL_H_
#define RDCPPROTOCOL_H_

namespace Comm { namespace Proto { namespace Rdcp {

#ifndef __PACKED
#define __PACKED	__attribute__((__packed__))
#endif

struct RdcpHeader__ {
	/* From client to server: 0xC0DE
	 * From server to client: 0xFEED */
	unsigned short		signature;
	/* Number of port in the other end that will receive further packets.
	 * This is used during the connection process */
	unsigned short		responsePort;
	/* If this indexH:indexL is lower than in the last received packet,
	 * the current packet is discarded */
	unsigned int		indexH;
	unsigned int		indexL;
} __PACKED;

typedef struct RdcpHeader__	RdcpHeader;

#define RDCPHEADER_CLIENT_TO_SERVER_SIGNATURE	0xC0DE
// Highest bit: 1 = client keeps fixed destination command port, 0 = client changes destination command port to received packet's
#define RDCPHEADER_SERVER_TO_CLIENT_SIGNATURE	0x7EED

RdcpHeader *RdcpHeader_hton(RdcpHeader *h);
RdcpHeader *RdcpHeader_ntoh(RdcpHeader *h);

}}}

#endif /* RDCPPROTOCOL_H_ */
