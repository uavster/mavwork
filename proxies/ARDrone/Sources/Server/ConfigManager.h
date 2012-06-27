/*
 * ConfigManager.h
 *
 *  Created on: 16/01/2012
 *      Author: Ignacio Mellado
 */

#ifndef CONFIGMANAGER_H_
#define CONFIGMANAGER_H_

#include <ardrone_api.h>

void configManager_init();
void configManager_destroy();
C_RESULT configManager_setParam(uint32_t id, uint8_t *value, uint32_t valueLength);
C_RESULT configManager_setParamFromNetwork(uint32_t id, uint8_t *value, uint32_t valueLength);
C_RESULT configManager_getParam(uint32_t id, uint8_t *value, uint32_t maxLength, uint32_t *actualLength);
C_RESULT configManager_getParamForNetwork(uint32_t id, uint8_t *value, uint32_t maxLength, uint32_t *actualLength);
float configManager_getParamFloat(uint32_t id, C_RESULT *result);
int32_t configManager_getParamInt32(uint32_t id, C_RESULT *result);

#endif /* CONFIGMANAGER_H_ */
