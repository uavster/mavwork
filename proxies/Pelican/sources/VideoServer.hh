/*
 * VideoServer.hh
 *
 *  Created on: 22/05/2012
 *      Author: Ignacio Mellado-Bataller
 */

#define VIDEOSERVER_READ_TIMEOUT	0.2
#define VIDEOSERVER_WRITE_TIMEOUT	0.05
#define VIDEOSERVER_FPS				10

#define template_def	template<class _ServerT, class _FrameT>
#define VideoServer_	VideoServer<_ServerT, _FrameT>

#include "comm/application/video/codecs/jpeg.h"

namespace Comm {

template_def void VideoServer_::open(const IAddress &addr, cvg_int maxQueueLength) {
	ManagedServer<_ServerT>::open(addr, maxQueueLength);
	addEventListener(&serverEventHandler);
}

template_def void VideoServer_::close() {
	removeEventListener(&serverEventHandler);
	ManagedServer<_ServerT>::close();

	while(clients.size() > 0) {
		typename std::list<Client *>::iterator it = clients.begin();
		Client *ct = *it;
		clients.erase(it);
		delete ct;
	}
}

template_def void VideoServer_::ManServerListener::gotEvent(EventGenerator &source, const Event &e) {
	parent->processEvent(source, e);
}

template_def void VideoServer_::processEvent(EventGenerator &source, const Event &e) {
	const typename ManagedServer<_ServerT>::NewClientEvent *nce = dynamic_cast<const typename ManagedServer<_ServerT>::NewClientEvent *>(&e);
	if (nce != NULL) {
		// This overrides the const qualifier. Therefore, callers must not rely on e.socket->userData after calling gotEvent()
		((typename ManagedServer<_ServerT>::NewClientEvent *)nce)->socket->userData = new Client;

		if (!clientsMutex.lock()) throw cvgException("[VideoServer] Unable to lock clients mutex");
		clients.push_back((Client *)nce->socket->userData);
		clientsMutex.unlock();
		nce->socket->setTimeouts(VIDEOSERVER_READ_TIMEOUT, VIDEOSERVER_WRITE_TIMEOUT);
	} else {
		const typename ManagedServer<_ServerT>::ClientStoppedEvent *cee = dynamic_cast<const typename ManagedServer<_ServerT>::ClientStoppedEvent *>(&e);
		if (cee != NULL) {
			if (!clientsMutex.lock()) throw cvgException("[VideoServer] Unable to lock clients mutex");
			typename std::list<Client *>::iterator it;
			for (it = clients.begin(); it != clients.end(); it++) {
				if (*it == (Client *)cee->socket->userData) {
					clients.erase(it);
					break;
				}
			}
			clientsMutex.unlock();
			// This overrides the const qualifier. Therefore, callers must not rely on e.socket->userData after calling gotEvent()
			delete (Client *)((typename ManagedServer<_ServerT>::ClientStoppedEvent *)cee)->socket->userData;
		}
	}
}

template_def void VideoServer_::manageClient(IConnectedSocket *socket) {
	Client *client = (Client *)socket->userData;
	cvg_bool timeout = true;
	if (client->mutex.lock()) {
		timeout = !client->condition.wait(&client->mutex, 0.5);
		client->mutex.unlock();
	}
	if (!timeout) {
		if (bufferMutex.lock()) {
			cvg_ulong tmp_frameLength = frameLength + headerBuffer.sizeBytes();
			// Copy buffer to our private send buffer
			client->sendFrameBuffer.resize(maxFrameLength + headerBuffer.sizeBytes());
			client->sendFrameBuffer.copy(&headerBuffer, headerBuffer.sizeBytes());
			client->sendFrameBuffer.copy(&frameBuffer, frameLength, headerBuffer.sizeBytes());
//			client->sendHeaderBuffer = headerBuffer;
			bufferMutex.unlock();

//			socket->write(client->sendHeaderBuffer);
//			std::cout << "frame length: " << tmp_frameLength << std::endl;
			socket->write(&client->sendFrameBuffer, tmp_frameLength);
		} else usleep(10000);
	}
}

template_def void VideoServer_::feedFrame(const _FrameT *header, VideoFormat inputFormat, const void *frameData, cvg_int frameDataLength, cvg_int width, cvg_int height, cvg_uint *actualSize) {
	if (fpsTimer.getElapsedSeconds() <= 1.0 / VIDEOSERVER_FPS) return;
	fpsTimer.restart();

	// If the input and output formats do not match, recode the image data
	if (inputFormat == outputFormat) {
		if (bufferMutex.lock()) {
			frameLength = frameDataLength;
			maxFrameLength = frameDataLength;
			*actualSize = frameLength;
			headerBuffer.copy(header, sizeof(_FrameT));
			headerBuffer[0].hton();
			frameBuffer.copy(frameData, frameDataLength);
			bufferMutex.unlock();
		}
	} else {
		// Ensure the frame buffer is resized to the maximum on the first frame to avoid continuous resizing
		// We assume that the output encoding will produce smaller frame data than the input format
		char *tmpFrameData = (char *)frameData;
		if (inputFormat == RAW_RGB24 && (outputFormat == RAW_BGR24 || outputFormat == JPEG)) {
			recodingBuffer.resize(frameDataLength);
			tmpFrameData = (char *)&recodingBuffer;
			for (cvg_int i = 0; i < width * height * 3; i += 3) {
				tmpFrameData[i] = ((char *)frameData)[i + 2];
				tmpFrameData[i + 1] = ((char *)frameData)[i + 1];
				tmpFrameData[i + 2] = ((char *)frameData)[i];
			}
		}

		cvg_int tmp_frameLength = frameDataLength;
		if (outputFormat == JPEG) {
			recodingBuffer2.resize(frameDataLength);
			tmp_frameLength = jpeg_compress((char *)&recodingBuffer2, tmpFrameData, width, height, frameBuffer.sizeBytes(), (int)(outputQuality * 100));
			tmpFrameData = (char *)&recodingBuffer2;
		}

		if (bufferMutex.lock()) {
			frameLength = tmp_frameLength;
			maxFrameLength = frameDataLength;
			*actualSize = frameLength;
			headerBuffer.copy(header, sizeof(_FrameT));
//std::cout << "w:" << header->videoData.width << ",h:" << header->videoData.height << ",f:" << (int)header->videoData.encoding << ",l:" << header->videoData.dataLength << std::endl;
			headerBuffer[0].hton();
			frameBuffer.resize(frameDataLength);
			frameBuffer.copy(tmpFrameData, frameLength);
			bufferMutex.unlock();
		}
	}

	// Notify clients
	if (clientsMutex.lock()) {
		typename std::list<Client *>::iterator it;
		for (it = clients.begin(); it != clients.end(); it++) {
			Client *c = *it;
			if (c->mutex.lock()) {
				c->condition.signal();
				c->mutex.unlock();
			}
		}
		clientsMutex.unlock();
	}
}

}

#undef VideoServer_
#undef template_def

#undef VIDEOSERVER_READ_TIMEOUT
#undef VIDEOSERVER_WRITE_TIMEOUT

