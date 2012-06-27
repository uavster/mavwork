/**
 * @file main.c
 * @author sylvain.gaeremynck@parrot.com
 * @date 2009/07/01
 *
 * Customization: Ignacio Mellado
 */

//#define VIDEO_SIMULATION

#include <ardrone_testing_tool.h>

//ARDroneLib
#include <ardrone_tool/ardrone_time.h>
#include <ardrone_tool/Navdata/ardrone_navdata_client.h>
#include <ardrone_tool/Control/ardrone_control.h>
#include <ardrone_tool/UI/ardrone_input.h>

//Common
#include <config.h>
#include <ardrone_api.h>

//VP_SDK
#include <ATcodec/ATcodec_api.h>
#include <VP_Os/vp_os_print.h>
#include <VP_Api/vp_api_thread_helper.h>
#include <VP_Os/vp_os_signal.h>

//Local project
#include <UI/gamepad.h>
#include <UI/keyboard.h>
#include <Video/video_stage.h>
#include <Server/commandServer.h>
#include <Navdata/navdata.h>
#include <VP_Com/vp_com.h>
#include <VP_Com/vp_com_error.h>
#include <Server/VideoServer.h>
#include <Server/ConfigServer.h>
#include <signal.h>	/* Only for Linux */
#include "Server/ConfigManager.h"

static int32_t exit_ihm_program = 1;

/* Implementing Custom methods for the main function of an ARDrone application */

vp_com_t globalCom;

void endSignalCatcher(int s) {
	signal_exit();
}

PROTO_THREAD_ROUTINE(cameraTest, data);

bool_t appStarted = FALSE;

/* The delegate object calls this method during initialization of an ARDrone application */
C_RESULT ardrone_tool_init_custom(int argc, char **argv)
{
	struct sigaction sa;
	vp_os_memset(&sa, 0, sizeof(sa));
	sa.sa_handler = &endSignalCatcher;
	sigaction(SIGINT, &sa, NULL);
	sigaction(SIGTERM, &sa, NULL);

	/* Open communications */
	vp_os_memset((char *)&globalCom, 0, sizeof(vp_com_t));
	globalCom.type = VP_COM_WIFI;
	C_RESULT res = vp_com_init(&globalCom);
	if (res != VP_COM_OK) {
		PRINT("Error opening communications: 0x%x\n", res);
		return C_FAIL;
	}

  /* Registering for a new device of game controller */
  /* Do not use any input method, since all commands will come from the server */
  /* ardrone_tool_input_add( &keyboard ); */

  /* Init configuration manager */
  configManager_init();

  /* Init video server */
  if (FAILED(videoServer_init())) {
	  vp_com_shutdown(&globalCom);
	  PRINT("Error initializing video server\n");
	  return C_FAIL;
  }

  /* Init config server */
  if (FAILED(configServer_init())) {
	  vp_com_shutdown(&globalCom);
	  videoServer_destroy();
	  PRINT("Error initializing configuration server\n");
	  return C_FAIL;
  }

  /* Start all threads of your application */
  START_THREAD(server, NULL);
  START_THREAD(feedbackClient, NULL);
  START_THREAD(videoServer_broadcaster, NULL);
  START_THREAD(videoServer, NULL);
  START_THREAD( video_stage, NULL );
  START_THREAD(configServer_roomService, NULL);
  START_THREAD(configServer_receptionist, NULL);
#ifdef VIDEO_SIMULATION
  START_THREAD(cameraTest, NULL);
#endif

  appStarted = TRUE;
  
  return C_OK;
}

/* The delegate object calls this method when the event loop exit */
C_RESULT ardrone_tool_shutdown_custom()
{
	if (!appStarted) return C_OK;

  PRINT("Joining threads...\n");

  /* Relinquish all threads of your application */
#ifdef VIDEO_SIMULATION
  JOIN_THREAD(cameraTest);
#endif
  JOIN_THREAD(configServer_receptionist);
  JOIN_THREAD(configServer_roomService);
  JOIN_THREAD( video_stage );
  JOIN_THREAD(videoServer);
  videoServer_endBroadcaster();
  JOIN_THREAD(videoServer_broadcaster);
  JOIN_THREAD(feedbackClient);
  JOIN_THREAD(server);

  PRINT("All threads joined\n");

  configServer_destroy();
  videoServer_destroy();
  configManager_destroy();

  /* Unregistering for the current device */
  /* ardrone_tool_input_remove( &gamepad ); */
  vp_com_shutdown(&globalCom);

  PRINT("Control is given back to AR.Drone SDK\n");

  return C_OK;
}

/* The event loop calls this method for the exit condition */
bool_t ardrone_tool_exit()
{
  return exit_ihm_program == 0;
}

C_RESULT signal_exit()
{
  exit_ihm_program = 0;

  return C_OK;
}

#ifdef VIDEO_SIMULATION

#include <unistd.h>

DEFINE_THREAD_ROUTINE(cameraTest, data) {
	uint8_t *buffer = vp_os_malloc(320*240*3);
	uint32_t count = 0;
	while(!ardrone_tool_exit()) {
		uint32_t y, x;
		for (y = 0; y < 240; y++) {
			for (x = 0; x < 320 * 3; x++) {
				buffer[y * (320 * 3) + x] = y + count;
			}
		}
		count ++;
		if (videoServer_isStarted()) {
			videoServer_lockFrameBuffer();
			videoServer_feedFrame_BGR24((int8_t *)buffer);
			videoServer_unlockFrameBuffer();
		}
		usleep(16667);
	}
	vp_os_free((int8_t *)buffer);
	THREAD_RETURN(0);
}

#endif

/* Implementing thread table in which you add routines of your application and those provided by the SDK */
BEGIN_THREAD_TABLE
  THREAD_TABLE_ENTRY( ardrone_control, 20 )
  THREAD_TABLE_ENTRY( navdata_update, 20 )
  THREAD_TABLE_ENTRY( video_stage, 20 )
  THREAD_TABLE_ENTRY(server, 20)
  THREAD_TABLE_ENTRY(feedbackClient, 20)
  THREAD_TABLE_ENTRY(videoServer, 20)
  THREAD_TABLE_ENTRY(videoServer_broadcaster, 20)
  THREAD_TABLE_ENTRY(configServer_receptionist, 20)
  THREAD_TABLE_ENTRY(configServer_roomService, 20)
#ifdef VIDEO_SIMULATION
  THREAD_TABLE_ENTRY(cameraTest, 20)
#endif
END_THREAD_TABLE

