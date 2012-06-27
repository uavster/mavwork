/*
 * ConfigManager.c
 *
 *  Created on: 16/01/2012
 *      Author: Ignacio Mellado
 */

#define MAX_PARAM_NAME_LENGTH		64

#include "ConfigManager.h"
#include "Protocol.h"
#include <ardrone_api.h>
#include <VP_Os/vp_os_print.h>
#include <arpa/inet.h>
#include <Soft/Lib/ardrone_tool/ardrone_tool_configuration.h>
#include "VideoServer.h"
#include "VideoTranscoder.h"

#define VALUETYPE_ARRAY		0
#define VALUETYPE_INT32		1
#define VALUETYPE_FLOAT		2
#define VALUETYPE_BOOL		3

/* Default parameter values */
#define MAX_EULER_ANGLE_DEG			2.0f
#define MAX_VERTICAL_SPEED_M_S		1.0f
#define MAX_YAW_SPEED_DEG_S			100.0f
#define MAX_ALTITUDE_M				3.0f
#define VIDEO_FPS					15	// It doesn't seem to have much effect on the drone
#define VIDEO_FORMAT				JPEG //RAW_BGR24
#define VIDEO_QUALITY				0.7f

typedef struct {
	uint32_t	id;
	char		name[MAX_PARAM_NAME_LENGTH + 1];
	uint8_t		valueType;
	uint8_t		*value;
	uint32_t	valueLength;
} TParam;

TParam configManager_params[CONFIGPARAM_NUM_PARAMS];

void configManager_registerParamArray(uint32_t id, const char *name, uint8_t valueType, uint32_t valueLength) {
	configManager_params[id].id = id;
	if (strlen(name) > MAX_PARAM_NAME_LENGTH) {
		vp_os_memcpy(configManager_params[id].name, name, MAX_PARAM_NAME_LENGTH);
		configManager_params[id].name[MAX_PARAM_NAME_LENGTH] = '\0';
	}
	else strcpy(configManager_params[id].name, name);
	configManager_params[id].valueType = valueType;
	switch(valueType) {
	case VALUETYPE_ARRAY:
		configManager_params[id].valueLength = valueLength;
		break;
	case VALUETYPE_INT32:
		configManager_params[id].valueLength = 4;
		break;
	case VALUETYPE_FLOAT:
		configManager_params[id].valueLength = 4;
		break;
	case VALUETYPE_BOOL:
		configManager_params[id].valueLength = 1;
		break;
	}
	configManager_params[id].value = vp_os_malloc(valueLength);
}

void configManager_registerParam(uint32_t id, const char *name, uint8_t valueType) {
	configManager_registerParamArray(id, name, valueType, 0);
}

void configManager_unregisterParam(uint32_t id) {
	vp_os_free(configManager_params[id].value);
}

C_RESULT configManager_setParameter(uint32_t id, uint8_t *value, uint32_t valueLength, bool_t fromNetwork, bool_t verbose);

void configManager_init() {
	/* Register existing parameters */
	configManager_registerParam(CONFIGPARAM_MAX_EULER_ANGLES, "Max. Euler angles", VALUETYPE_FLOAT);
	configManager_registerParam(CONFIGPARAM_MAX_VERTICAL_SPEED, "Max. vertical speed", VALUETYPE_FLOAT);
	configManager_registerParam(CONFIGPARAM_MAX_YAW_SPEED, "Max. yaw speed", VALUETYPE_FLOAT);
	configManager_registerParam(CONFIGPARAM_MAX_ALTITUDE, "Max. altitude", VALUETYPE_FLOAT);
	configManager_registerParam(CONFIGPARAM_CAMERA_FRAMERATE, "Camera framerate", VALUETYPE_FLOAT);
	configManager_registerParam(CONFIGPARAM_VIDEO_ENCODING_TYPE, "Video encoding type", VALUETYPE_INT32);
	configManager_registerParam(CONFIGPARAM_VIDEO_ENCODING_QUALITY, "Video encoding quality", VALUETYPE_FLOAT);
	configManager_registerParamArray(CONFIGPARAM_LEDS_ANIMATION, "LEDs animation", VALUETYPE_ARRAY, sizeof(LedsAnimation));

	/* Set default values */
	{
		int32_t ivalue;
		float value =  MAX_EULER_ANGLE_DEG;
		configManager_setParameter(CONFIGPARAM_MAX_EULER_ANGLES, (uint8_t *)&value, sizeof(float), FALSE, FALSE);
		value = MAX_VERTICAL_SPEED_M_S;
		configManager_setParameter(CONFIGPARAM_MAX_VERTICAL_SPEED, (uint8_t *)&value, sizeof(float), FALSE, FALSE);
		value = MAX_YAW_SPEED_DEG_S;
		configManager_setParameter(CONFIGPARAM_MAX_YAW_SPEED, (uint8_t *)&value, sizeof(float), FALSE, FALSE);
		value = MAX_ALTITUDE_M;
		configManager_setParameter(CONFIGPARAM_MAX_ALTITUDE, (uint8_t *)&value, sizeof(float), FALSE, FALSE);
		value = VIDEO_FPS;
		configManager_setParameter(CONFIGPARAM_CAMERA_FRAMERATE, (uint8_t *)&value, sizeof(float), FALSE, FALSE);
		ivalue = VIDEO_FORMAT;
		configManager_setParameter(CONFIGPARAM_VIDEO_ENCODING_TYPE, (uint8_t *)&ivalue, sizeof(int32_t), FALSE, FALSE);
		value = VIDEO_QUALITY;
		configManager_setParameter(CONFIGPARAM_VIDEO_ENCODING_QUALITY, (uint8_t *)&value, sizeof(float), FALSE, FALSE);
	}
}

void configManager_destroy() {
	uint32_t i;
	for (i = 0; i < CONFIGPARAM_NUM_PARAMS; i++)
		configManager_unregisterParam(i);
}

void configManager_applyValue(uint32_t id) {
	switch(id) {
	case CONFIGPARAM_MAX_EULER_ANGLES:
		ardrone_control_config.euler_angle_max = (configManager_getParamFloat(id, NULL) * M_PI) / 180.0;
		ARDRONE_TOOL_CONFIGURATION_ADDEVENT(euler_angle_max, &ardrone_control_config.euler_angle_max, NULL);
		break;
	case CONFIGPARAM_MAX_VERTICAL_SPEED:
		ardrone_control_config.control_vz_max = configManager_getParamFloat(id, NULL) * 1000.0;
		ARDRONE_TOOL_CONFIGURATION_ADDEVENT(control_vz_max, &ardrone_control_config.control_vz_max, NULL);
		break;
	case CONFIGPARAM_MAX_YAW_SPEED:
		ardrone_control_config.control_yaw = (configManager_getParamFloat(id, NULL) * M_PI) / 180.0;
		ARDRONE_TOOL_CONFIGURATION_ADDEVENT(control_yaw, &ardrone_control_config.control_yaw, NULL);
		break;
	case CONFIGPARAM_MAX_ALTITUDE:
		ardrone_control_config.altitude_max = configManager_getParamFloat(id, NULL) * 1000.0;
		ARDRONE_TOOL_CONFIGURATION_ADDEVENT(altitude_max, &ardrone_control_config.altitude_max, NULL);
		break;
	case CONFIGPARAM_CAMERA_FRAMERATE:
		ardrone_control_config.camif_fps = configManager_getParamFloat(id, NULL);
		ARDRONE_TOOL_CONFIGURATION_ADDEVENT(camif_fps, &ardrone_control_config.camif_fps, NULL);
		break;
	case CONFIGPARAM_LEDS_ANIMATION:
		{
			LedsAnimation anim;
			uint32_t actualLength;
			configManager_getParam(CONFIGPARAM_LEDS_ANIMATION, (uint8_t *)&anim, sizeof(LedsAnimation), &actualLength);
			ardrone_at_set_led_animation(anim.typeId, anim.frequencyHz, anim.durationSec);
		}
		break;
	case CONFIGPARAM_VIDEO_ENCODING_TYPE:
		videoServer_setOutputEncoding(configManager_getParamInt32(id, NULL));
		break;
	case CONFIGPARAM_VIDEO_ENCODING_QUALITY:
		videoTranscoder_setQuality(configManager_getParamFloat(id, NULL));
		break;
	}
}

C_RESULT configManager_setParam(uint32_t id, uint8_t *value, uint32_t valueLength) {
	return configManager_setParameter(id, value, valueLength, FALSE, TRUE);
}

C_RESULT configManager_setParamFromNetwork(uint32_t id, uint8_t *value, uint32_t valueLength) {
	return configManager_setParameter(id, value, valueLength, TRUE, TRUE);
}

C_RESULT configManager_setParameter(uint32_t id, uint8_t *value, uint32_t valueLength, bool_t fromNetwork, bool_t verbose) {
	if (id < 0 || id >= CONFIGPARAM_NUM_PARAMS) {
		PRINT("[ConfigManager] Bad parameter ID when calling setParam()\n");
		return C_FAIL;
	}
	if (configManager_params[id].valueLength != valueLength) {
		PRINT("[ConfigManager] Error setting parameter \"%s\". The value length is not correct\n", configManager_params[id].name);
		return C_FAIL;
	}
	vp_os_memcpy(configManager_params[id].value, value, valueLength);
	/* Reorder some types into host order */
	if (verbose) PRINT("[ConfigManager] New value set for parameter \"%s\" : ", configManager_params[id].name);
	switch(configManager_params[id].valueType) {
	case VALUETYPE_INT32:
		if (fromNetwork)
			*((uint32_t *)configManager_params[id].value) = ntohl(*((uint32_t *)configManager_params[id].value));
		if (verbose) PRINT("%d\n", *((uint32_t *)configManager_params[id].value));
		break;
	case VALUETYPE_FLOAT:
		if (fromNetwork)
			float_ntoh((float *)configManager_params[id].value);
		if (verbose) PRINT("%f\n", *((float *)configManager_params[id].value));
		break;
	case VALUETYPE_BOOL:
		if (verbose) PRINT("%s\n", configManager_params[id].value[0] != 0 ? "TRUE" : "FALSE");
		break;
	default:
		if (verbose) PRINT("Array\n");
	}
	/* Send value to the drone */
	configManager_applyValue(id);
	return C_OK;
}

C_RESULT configManager_getParam(uint32_t id, uint8_t *value, uint32_t maxLength, uint32_t *actualLength) {
	(*actualLength) = 0;
	if (id < 0 || id >= CONFIGPARAM_NUM_PARAMS) {
		PRINT("[ConfigManager] Bad parameter ID when calling getParam()\n");
		return C_FAIL;
	}
	uint32_t length = configManager_params[id].valueLength;
	(*actualLength) = length;
	if (length > maxLength) length = maxLength;
	vp_os_memcpy(value, configManager_params[id].value, length);
	return C_OK;
}

C_RESULT configManager_getParamForNetwork(uint32_t id, uint8_t *value, uint32_t maxLength, uint32_t *actualLength) {
	(*actualLength) = 0;
	if (id < 0 || id >= CONFIGPARAM_NUM_PARAMS) {
		PRINT("[ConfigManager] Bad parameter ID when calling getParamForNetwork()\n");
		return C_FAIL;
	}
	if (configManager_getParam(id, value, maxLength, actualLength) == C_FAIL) return C_FAIL;
	switch(configManager_params[id].valueType) {
	case VALUETYPE_INT32: *((uint32_t *)value) = htonl(*((uint32_t *)value)); break;
	case VALUETYPE_FLOAT: float_hton((float *)value); break;
	}
	return C_OK;
}

float configManager_getParamFloat(uint32_t id, C_RESULT *result) {
	if (id < 0 || id >= CONFIGPARAM_NUM_PARAMS) {
		if (result != NULL) (*result) = C_FAIL;
		PRINT("[ConfigManager] Bad parameter ID when calling getParamFloat()\n");
		return 0.0f;
	}
	if (result != NULL) (*result) = C_OK;
	switch(configManager_params[id].valueType) {
	case VALUETYPE_FLOAT: return *((float *)configManager_params[id].value);
	case VALUETYPE_INT32: return (float)(*((int32_t *)configManager_params[id].value));
	case VALUETYPE_BOOL: return configManager_params[id].value[0] != 0 ? 1.0f : 0.0f;
	default:
		if (result != NULL) (*result) = C_FAIL;
		PRINT("[ConfigManager] Unable to get parameter \"%s\" as FLOAT\n", configManager_params[id].name);
		return 0.0f;
		break;
	}
}

int32_t configManager_getParamInt32(uint32_t id, C_RESULT *result) {
	if (id < 0 || id >= CONFIGPARAM_NUM_PARAMS) {
		if (result != NULL) (*result) = C_FAIL;
		PRINT("[ConfigManager] Bad parameter ID when calling getParamInt32()\n");
		return 0;
	}
	if (result != NULL) (*result) = C_OK;
	switch(configManager_params[id].valueType) {
	case VALUETYPE_INT32: return *((int32_t *)configManager_params[id].value);
	case VALUETYPE_FLOAT: return (int32_t)(*((float *)configManager_params[id].value));
	case VALUETYPE_BOOL: return configManager_params[id].value[0] != 0 ? 1 : 0;
	default:
		if (result != NULL) (*result) = C_FAIL;
		PRINT("[ConfigManager] Unable to get parameter \"%s\" as INT32\n", configManager_params[id].name);
		return 0;
		break;
	}
}
