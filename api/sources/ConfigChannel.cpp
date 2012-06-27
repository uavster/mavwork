/*
 * ConfigChannel.cpp
 *
 *  Created on: 15/01/2012
 *      Author: Ignacio Mellado
 */

#include "ConfigChannel.h"

namespace DroneProxy {
namespace Comm {

ConfigChannel::ConfigChannel() {
	openLog("ConfigChannel");
	setChannelName("Configuration");
}

ConfigChannel::~ConfigChannel() {
	close();
	closeLog();
}

void ConfigChannel::open(cvgString host, cvg_int port) {
	close();
	setCurrentState(CONNECTING);
	try {
		Channel::open(host, port);
		configSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (configSocket == -1)
			throw cvgException("[ConfigChannel] Error opening socket");
		// Set linger time
		linger lingerData;
		lingerData.l_onoff = 1;
		lingerData.l_linger = CVG_LITERAL_INT(CONFIG_CHANNEL_DEFAULT_LINGER_SECONDS);
		setsockopt(configSocket, SOL_SOCKET, SO_LINGER, &lingerData, sizeof(linger));
		// Set socket timeouts so recv does not block permanently and the worker thread may be closed
		timeval sockTimeout;
		sockTimeout.tv_sec = floor(CONFIG_CHANNEL_MAX_OPERATION_SECONDS);
		sockTimeout.tv_usec = (long)(CONFIG_CHANNEL_MAX_OPERATION_SECONDS * 1e6 - sockTimeout.tv_sec * 1e6);
		setsockopt(configSocket, SOL_SOCKET, SO_RCVTIMEO, &sockTimeout, sizeof(timeval));
		setsockopt(configSocket, SOL_SOCKET, SO_SNDTIMEO, &sockTimeout, sizeof(timeval));
		// Connect to server
		sockaddr_in socketAddress;
		bzero((char *)&socketAddress, sizeof(socketAddress));
		socketAddress.sin_family = AF_INET;
		socketAddress.sin_port = htons(port);
		socketAddress.sin_addr.s_addr = inet_addr(host.c_str());
		if (connect(configSocket, (sockaddr *)&socketAddress, sizeof(sockaddr_in)) != 0) {
			throw cvgException("[ConfigChannel] Cannot connect to server");
		}

		// TODO: Send all the cached configuration changes to maintain status

		setCurrentState(CONNECTED);
	} catch(cvgException e) {
		close();
		setCurrentState(FAULT);
		throw e;
	}
}

void ConfigChannel::close() {
	setCurrentState(DISCONNECTING);
	if (configSocket != -1) {
		shutdown(configSocket, SHUT_RDWR);
		::close(configSocket);
		configSocket = -1;
	}
	setCurrentState(DISCONNECTED);
	Channel::close();
}

void ConfigChannel::sendBuffer(const void *buffer, cvg_int length, cvg_bool partial) {
	cvg_int length0 = length;
	while(length > 0) {
		ssize_t result = send(configSocket, &((cvg_char *)buffer)[length - length0], length, MSG_NOSIGNAL | (partial ? MSG_MORE : 0));
		if (result < 0) throw cvgException("send() failed");
		length -= result;
	}
}

void ConfigChannel::recvBuffer(void *buffer, cvg_int length, cvg_bool justPurge) {
	cvg_int length0 = length;
	while(length > 0) {
		ssize_t result = recv(configSocket, justPurge ? ((cvg_char *)buffer) : &((cvg_char *)buffer)[length - length0], length, MSG_NOSIGNAL);
		if (result < 0) throw cvgException("recv() failed");
		length -= result;
	}
}

void ConfigChannel::writeParam(cvg_int id, const cvg_char *buffer, cvg_int length) {
	cvg_bool closeChannelOnError = true;
	try {
		ConfigHeader header;
		header.signature = PROTOCOL_CONFIG_SIGNATURE;
		header.info.mode = CONFIG_WRITE;
		header.info.paramId = id;
		header.info.dataLength = length;
		ConfigHeader_hton(&header);
		sendBuffer(&header, sizeof(ConfigHeader), true);
		sendBuffer(buffer, length, false);
		recvBuffer(&header, sizeof(ConfigHeader));
		ConfigHeader_ntoh(&header);
		if (header.info.paramId != id || (header.info.mode != CONFIG_ACK && header.info.mode != CONFIG_NACK))
			throw cvgException("bad response");
		if (header.info.mode == CONFIG_NACK) {
			closeChannelOnError = false;
			throw cvgException("bad parameter specification");
		}
	} catch(cvgException e) {
		if (closeChannelOnError) close();
		throw cvgException(cvgString("[ConfigChannel] Error writing parameter ID ") + id + ". Reason: " + e.getMessage());
	}
}

void ConfigChannel::readParamArray(cvg_int id, cvg_char *buffer, cvg_int length) {
	cvg_bool closeChannelOnError = true;
	try {
		ConfigHeader header;
		header.signature = PROTOCOL_CONFIG_SIGNATURE;
		header.info.mode = CONFIG_READ;
		header.info.paramId = id;
		header.info.dataLength = 0;
		ConfigHeader_hton(&header);
		sendBuffer(&header, sizeof(ConfigHeader), true);
		recvBuffer(&header, sizeof(ConfigHeader));
		ConfigHeader_ntoh(&header);
		if ((header.info.mode != CONFIG_WRITE && header.info.mode != CONFIG_NACK) || header.info.paramId != id) {
			throw cvgException("bad response");
		}
		if (header.info.dataLength > length) {
			throw cvgException("data too long");
		}
		if (header.info.mode == CONFIG_NACK) {
			closeChannelOnError = false;
			throw cvgException("bad parameter specification");
		}
		recvBuffer(buffer, length);
	} catch(cvgException e) {
		if (closeChannelOnError) close();
		throw cvgException(cvgString("[ConfigChannel] Error writing parameter ID ") + id + ". Reason: " + e.getMessage());
	}
}

void ConfigChannel::writeParam(cvg_int id, cvg_float value) {
	float_hton(&value);
	writeParam(id, (cvg_char *)&value, sizeof(cvg_float));
}

void ConfigChannel::writeParam(cvg_int id, cvg_int value) {
	value = htonl(value);
	writeParam(id, (cvg_char *)&value, sizeof(cvg_int));
}

void ConfigChannel::writeParam(cvg_int id, const cvgString &value) {
	writeParam(id, value.c_str(), value.length());
}

cvg_float ConfigChannel::readParamFloat(cvg_int id) {
	cvg_float buffer;
	readParamArray(id, (cvg_char *)&buffer, sizeof(cvg_float));
	float_ntoh(&buffer);
	return buffer;
}

cvg_int ConfigChannel::readParamInt(cvg_int id) {
	cvg_int buffer;
	readParamArray(id, (cvg_char *)&buffer, sizeof(cvg_int));
	buffer = ntohl(buffer);
	return buffer;
}

cvgString ConfigChannel::readParamString(cvg_int id, cvg_int length) {
	cvg_char buffer[length + 1];
	readParamArray(id, buffer, length);
	buffer[length] = '\0';
	return cvgString(buffer);
}

}
}
