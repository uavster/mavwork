/*
 * Protocol.c
 *
 *  Created on: 12/08/2011
 *      Author: Ignacio Mellado
 */

#include <arpa/inet.h>
#include <Protocol.h>
#include <string.h>

void float_ntoh(float *f) {
	*(unsigned int *)f = ntohl(*(unsigned int *)f);
}

void float_hton(float *f) {
	*(unsigned int *)f = htonl(*(unsigned int *)f);
}

void CommandPacket_ntoh(CommandPacket *p) {
	p->signature = ntohs(p->signature);
	p->indexL = ntohl(p->indexL);
	p->indexH = ntohl(p->indexH);
	p->feedbackPort = ntohs(p->feedbackPort);
	p->controlData.timeCodeH = ntohl(p->controlData.timeCodeH);
	p->controlData.timeCodeL = ntohl(p->controlData.timeCodeL);
	float_ntoh(&p->controlData.phi);
	float_ntoh(&p->controlData.theta);
	float_ntoh(&p->controlData.gaz);
	float_ntoh(&p->controlData.yaw);
}

void CommandPacket_hton(CommandPacket *p) {
	p->signature = htons(PROTOCOL_COMMAND_SIGNATURE);
	p->indexL = htonl(p->indexL);
	p->indexH = htonl(p->indexH);
	p->feedbackPort = htons(p->feedbackPort);
	p->controlData.timeCodeH = htonl(p->controlData.timeCodeH);
	p->controlData.timeCodeL = htonl(p->controlData.timeCodeL);
	float_hton(&p->controlData.phi);
	float_hton(&p->controlData.theta);
	float_hton(&p->controlData.gaz);
	float_hton(&p->controlData.yaw);
}

void FeedbackPacket_ntoh(FeedbackPacket *p) {
	p->signature = ntohs(p->signature);
	p->commandPort = ntohs(p->commandPort);
	p->indexH = ntohl(p->indexH);
	p->indexL = ntohl(p->indexL);
	p->feedbackData.timeCodeH = ntohl(p->feedbackData.timeCodeH);
	p->feedbackData.timeCodeL = ntohl(p->feedbackData.timeCodeL);
	p->feedbackData.nativeDroneState = ntohl(p->feedbackData.nativeDroneState);
	float_ntoh(&p->feedbackData.batteryLevel);
	float_ntoh(&p->feedbackData.roll);
	float_ntoh(&p->feedbackData.pitch);
	float_ntoh(&p->feedbackData.yaw);
	float_ntoh(&p->feedbackData.altitude);
	float_ntoh(&p->feedbackData.speedX);
	float_ntoh(&p->feedbackData.speedY);
	float_ntoh(&p->feedbackData.speedYaw);
}

void FeedbackPacket_hton(FeedbackPacket *p) {
	p->signature = htons(PROTOCOL_FEEDBACK_SIGNATURE);
	p->commandPort = htons(p->commandPort);
	p->indexH = htonl(p->indexH);
	p->indexL = htonl(p->indexL);
	p->feedbackData.timeCodeH = htonl(p->feedbackData.timeCodeH);
	p->feedbackData.timeCodeL = htonl(p->feedbackData.timeCodeL);
	p->feedbackData.nativeDroneState = htonl(p->feedbackData.nativeDroneState);
	float_hton(&p->feedbackData.batteryLevel);
	float_hton(&p->feedbackData.roll);
	float_hton(&p->feedbackData.pitch);
	float_hton(&p->feedbackData.yaw);
	float_hton(&p->feedbackData.altitude);
	float_hton(&p->feedbackData.speedX);
	float_hton(&p->feedbackData.speedY);
	float_hton(&p->feedbackData.speedYaw);
}

void flyingModeToString(char *s, char fm) {
	switch(fm) {
		case IDLE: strcpy(s, "IDLE"); break;
		case TAKEOFF: strcpy(s, "TAKE-OFF"); break;
		case HOVER: strcpy(s, "HOVER"); break;
		case MOVE: strcpy(s, "MOVE"); break;
		case LAND: strcpy(s, "LAND"); break;
		case INIT: strcpy(s, "INIT"); break;
		case EMERGENCYSTOP: strcpy(s, "EMERGENCY"); break;
		default: strcpy(s, "UNKNOWN"); break;
	}
}

void droneModeToString(char *s, char dm) {
	switch(dm) {
		case LANDED: strcpy(s, "LANDED"); break;
		case FLYING: strcpy(s, "FLYING"); break;
		case HOVERING: strcpy(s, "HOVERING"); break;
		case TAKINGOFF: strcpy(s, "TAKINGOFF"); break;
		case LANDING: strcpy(s, "LANDING"); break;
		case EMERGENCY: strcpy(s, "EMERGENCY"); break;
		default:
		case UNKNOWN: strcpy(s, "UNKNOWN"); break;
	}
}

void VideoFrameHeader_hton(VideoFrameHeader *p) {
	p->signature = htons(PROTOCOL_VIDEOFRAME_SIGNATURE);
	p->videoData.timeCodeH = htonl(p->videoData.timeCodeH);
	p->videoData.timeCodeL = htonl(p->videoData.timeCodeL);
	p->videoData.width = htonl(p->videoData.width);
	p->videoData.height = htonl(p->videoData.height);
	p->videoData.dataLength = htonl(p->videoData.dataLength);
}

void VideoFrameHeader_ntoh(VideoFrameHeader *p) {
	p->signature = ntohs(p->signature);
	p->videoData.timeCodeH = ntohl(p->videoData.timeCodeH);
	p->videoData.timeCodeL = ntohl(p->videoData.timeCodeL);
	p->videoData.width = ntohl(p->videoData.width);
	p->videoData.height = ntohl(p->videoData.height);
	p->videoData.dataLength = ntohl(p->videoData.dataLength);
}

void ConfigHeader_hton(ConfigHeader *p) {
	p->signature = htons(p->signature);
	p->info.paramId = htonl(p->info.paramId);
	p->info.dataLength = htonl(p->info.dataLength);
}

void ConfigHeader_ntoh(ConfigHeader *p) {
	p->signature = ntohs(p->signature);
	p->info.paramId = ntohl(p->info.paramId);
	p->info.dataLength = ntohl(p->info.dataLength);
}
