/*
 * RdcpProtocol.cpp
 *
 *  Created on: 05/05/2012
 *      Author: Ignacio Mellado-Bataller
 */

#include "comm/transport/cvg/RdcpProtocol.h"
#include <arpa/inet.h>

namespace Comm { namespace Proto { namespace Rdcp {

RdcpHeader *RdcpHeader_hton(RdcpHeader *h) {
	h->signature = htons(h->signature);
	h->responsePort = htons(h->responsePort);
	h->indexH = htonl(h->indexH);
	h->indexL = htonl(h->indexL);
	return h;
}

RdcpHeader *RdcpHeader_ntoh(RdcpHeader *h) {
	h->signature = ntohs(h->signature);
	h->responsePort = ntohs(h->responsePort);
	h->indexH = ntohl(h->indexH);
	h->indexL = ntohl(h->indexL);
	return h;
}

}}}
