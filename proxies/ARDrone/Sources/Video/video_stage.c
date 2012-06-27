/*
 * @video_stage.c
 * @author marc-olivier.dzeukou@parrot.com
 * @date 2007/07/27
 *
 * ihm vision thread implementation
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <termios.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>

#include <sys/time.h>
#include <time.h>

#include <VP_Api/vp_api.h>
#include <VP_Api/vp_api_error.h>
#include <VP_Api/vp_api_stage.h>
#include <VP_Api/vp_api_picture.h>
#include <VP_Stages/vp_stages_io_file.h>
#include <VP_Stages/vp_stages_i_camif.h>

#include <config.h>
#include <VP_Os/vp_os_print.h>
#include <VP_Os/vp_os_malloc.h>
#include <VP_Os/vp_os_delay.h>
#include <VP_Stages/vp_stages_yuv2rgb.h>
#include <VP_Stages/vp_stages_buffer_to_picture.h>
#include <VLIB/Stages/vlib_stage_decode.h>

#include <ardrone_tool/ardrone_tool.h>
#include <ardrone_tool/Com/config_com.h>

#include <Server/VideoServer.h>

/* #define IMAGE_DEBUG */

/*#ifndef RECORD_VIDEO
#define RECORD_VIDEO
#endif*/
#ifdef RECORD_VIDEO
#undef RECORD_VIDEO
#endif

#ifdef RECORD_VIDEO
#    include <ardrone_tool/Video/video_stage_recorder.h>
#endif

#include <ardrone_tool/Video/video_com_stage.h>

#include "Video/video_stage.h"

#include <cv.h>
#include <highgui.h>
IplImage *debugImage = NULL;

#define NB_STAGES 10

PIPELINE_HANDLE pipeline_handle;

static uint8_t*  pixbuf_data       = NULL;
int8_t *outputImageBuffer = NULL;

C_RESULT output_gtk_stage_open( void *cfg, vp_api_io_data_t *in, vp_api_io_data_t *out)
{
#ifdef IMAGE_DEBUG
	PRINT("Starting local video debug...\n");
	cvStartWindowThread();
	cvNamedWindow("DroneCamera", CV_WINDOW_AUTOSIZE);
#endif
	debugImage = cvCreateImageHeader(cvSize(QVGA_WIDTH, QVGA_HEIGHT), IPL_DEPTH_8U, 3);
	return (SUCCESS);
}

/* struct timeval lastFrameTime; */

C_RESULT output_gtk_stage_transform( void *cfg, vp_api_io_data_t *in, vp_api_io_data_t *out)
{
  uint64_t timeCode;
  struct timeval lastFrameReceptionTime;

  gettimeofday(&lastFrameReceptionTime, NULL);

  if (!videoServer_isStarted()) return SUCCESS;

  videoServer_lockFrameBuffer();
 
  if(out->status == VP_API_STATUS_INIT)
  {
    out->numBuffers = 1;
    out->size = QVGA_WIDTH * QVGA_HEIGHT * 3;
    out->buffers = (int8_t **) vp_os_malloc (sizeof(int8_t *)+out->size*sizeof(int8_t));
    out->buffers[0] = (int8_t *)(out->buffers+1);
    outputImageBuffer = out->buffers[0];
    cvSetData(debugImage, outputImageBuffer, QVGA_WIDTH * 3);

    out->indexBuffer = 0;
    // out->lineSize not used
    out->status = VP_API_STATUS_PROCESSING;
  }

  if(out->status == VP_API_STATUS_PROCESSING && outputImageBuffer != NULL) {
	  /* Get a reference to the last decoded picture */
	  pixbuf_data      = (uint8_t*)in->buffers[0];

	  {
		  int i;
		  int8_t *outBuffer = outputImageBuffer;
		  for (i = 0; i < QVGA_WIDTH * QVGA_HEIGHT * 3; i += 3) {
			  outBuffer[i] = pixbuf_data[i + 2];
			  outBuffer[i + 1] = pixbuf_data[i + 1];
			  outBuffer[i + 2] = pixbuf_data[i];
		  }
	  }

	  timeCode = (uint64_t)lastFrameReceptionTime.tv_sec * 1e6 + (uint64_t)lastFrameReceptionTime.tv_usec;
	  videoServer_feedFrame_BGR24(timeCode, outputImageBuffer);

/*	  struct timeval currentTime;
	  gettimeofday(&currentTime, NULL);
	  double elapsed = (currentTime.tv_sec - lastFrameTime.tv_sec) + (currentTime.tv_usec - lastFrameTime.tv_usec) / 1e6;
	  lastFrameTime = currentTime;
	  PRINT("fps: %f\n", 1.0 / elapsed);
*/
#ifdef IMAGE_DEBUG
	  cvShowImage("DroneCamera", debugImage);
#endif

  }

  videoServer_unlockFrameBuffer();

  return (SUCCESS);
}

C_RESULT output_gtk_stage_close( void *cfg, vp_api_io_data_t *in, vp_api_io_data_t *out)
{
#ifdef IMAGE_DEBUG
	PRINT("Closing local video debug...\n");
	cvDestroyWindow("DroneCamera");
#endif
	if (debugImage != NULL) {
		cvReleaseImageHeader(&debugImage);
		debugImage = NULL;
	}
	if (outputImageBuffer != NULL) {
		vp_os_free(outputImageBuffer);
		outputImageBuffer = NULL;
	}
	/* In other filters, memory is not freed. I guess it is done by the VP stage manager */
  return (SUCCESS);
}

const vp_api_stage_funcs_t vp_stages_output_gtk_funcs =
{
  NULL,
  (vp_api_stage_open_t)output_gtk_stage_open,
  (vp_api_stage_transform_t)output_gtk_stage_transform,
  (vp_api_stage_close_t)output_gtk_stage_close
};

DEFINE_THREAD_ROUTINE(video_stage, data)
{
  C_RESULT res;

  vp_api_io_pipeline_t    pipeline;
  vp_api_io_data_t        out;
  vp_api_io_stage_t       stages[NB_STAGES];

  vp_api_picture_t picture;

  video_com_config_t              icc;
  vlib_stage_decoding_config_t    vec;
  vp_stages_yuv2rgb_config_t      yuv2rgbconf;
#ifdef RECORD_VIDEO
  video_stage_recorder_config_t   vrc;
#endif
  /// Picture configuration
  picture.format        = PIX_FMT_YUV420P;

  picture.width         = QVGA_WIDTH;
  picture.height        = QVGA_HEIGHT;
  picture.framerate     = 30;

  picture.y_buf   = vp_os_malloc( QVGA_WIDTH * QVGA_HEIGHT     );
  picture.cr_buf  = vp_os_malloc( QVGA_WIDTH * QVGA_HEIGHT / 4 );
  picture.cb_buf  = vp_os_malloc( QVGA_WIDTH * QVGA_HEIGHT / 4 );

  picture.y_line_size   = QVGA_WIDTH;
  picture.cb_line_size  = QVGA_WIDTH / 2;
  picture.cr_line_size  = QVGA_WIDTH / 2;

  vp_os_memset(&icc,          0, sizeof( icc ));
  vp_os_memset(&vec,          0, sizeof( vec ));
  vp_os_memset(&yuv2rgbconf,  0, sizeof( yuv2rgbconf ));

  icc.com                 = COM_VIDEO();
  icc.buffer_size         = 100000;
  icc.protocol            = VP_COM_UDP;
  COM_CONFIG_SOCKET_VIDEO(&icc.socket, VP_COM_CLIENT, VIDEO_PORT, wifi_ardrone_ip);

  vec.width               = QVGA_WIDTH;
  vec.height              = QVGA_HEIGHT;
  vec.picture             = &picture;
  vec.block_mode_enable   = TRUE;
  vec.luma_only           = FALSE;

  yuv2rgbconf.rgb_format = VP_STAGES_RGB_FORMAT_RGB24;
#ifdef RECORD_VIDEO
  vrc.fp = NULL;
#endif

  pipeline.nb_stages = 0;

  stages[pipeline.nb_stages].type    = VP_API_INPUT_SOCKET;
  stages[pipeline.nb_stages].cfg     = (void *)&icc;
  stages[pipeline.nb_stages].funcs   = video_com_funcs;

  pipeline.nb_stages++;

#ifdef RECORD_VIDEO
  stages[pipeline.nb_stages].type    = VP_API_FILTER_DECODER;
  stages[pipeline.nb_stages].cfg     = (void*)&vrc;
  stages[pipeline.nb_stages].funcs   = video_recorder_funcs;

  pipeline.nb_stages++;
#endif // RECORD_VIDEO
  stages[pipeline.nb_stages].type    = VP_API_FILTER_DECODER;
  stages[pipeline.nb_stages].cfg     = (void*)&vec;
  stages[pipeline.nb_stages].funcs   = vlib_decoding_funcs;

  pipeline.nb_stages++;

  stages[pipeline.nb_stages].type    = VP_API_FILTER_YUV2RGB;
  stages[pipeline.nb_stages].cfg     = (void*)&yuv2rgbconf;
  stages[pipeline.nb_stages].funcs   = vp_stages_yuv2rgb_funcs;

  pipeline.nb_stages++;

  stages[pipeline.nb_stages].type    = VP_API_OUTPUT_SDL;
  stages[pipeline.nb_stages].cfg     = NULL;
  stages[pipeline.nb_stages].funcs   = vp_stages_output_gtk_funcs;

  pipeline.nb_stages++;

  pipeline.stages = &stages[0];
 
  /* Processing of a pipeline */
  if( !ardrone_tool_exit() )
  {
    PRINT("\nVideo stage thread initialisation\n");

    res = vp_api_open(&pipeline, &pipeline_handle);

    if( SUCCEED(res) )
    {
      int loop = SUCCESS;
      out.status = VP_API_STATUS_PROCESSING;

      while( !ardrone_tool_exit() && (loop == SUCCESS) )
      {
          if( SUCCEED(vp_api_run(&pipeline, &out)) ) {
            if( (out.status == VP_API_STATUS_PROCESSING || out.status == VP_API_STATUS_STILL_RUNNING) ) {
              loop = SUCCESS;
            }
          }
          else loop = -1; // Finish this thread
      }
// This call is crashing everything. The crash is inside the closure of the VP packetizer. It looks like a Parrot bug.
//      vp_api_close(&pipeline, &pipeline_handle);
    }
  }

  PRINT("Video stage thread ended\n");

  return (THREAD_RET)0;
}

