#ifndef CAMUEYE_DRIVER_H
#define CAMUEYE_DRIVER_H

#define NUM_BUFFERS 2

#include <ueye.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ******** ******** ******/
/* uEye defaults CAMERA PARAMETERS */
/* ******** ******** ******/

#include <atlante.h>

namespace Video {

/* ******* ******* ******* *****/
/* uEye CAMERA FUNCTIONS CLASS */
/* ******* ******* ******* *****/
 #ifdef __cplusplus
 extern "C" {
 #endif

class CamUeye_driver
{
    //Q_OBJECT

    // basic params
    HIDS hCam;
    int x_off, y_off, width, height, bpp;
    bool debug;
    // sensor info
    double setFps, setExposureMode;
    double fps, exposure;

    // image buffers
    char*   imgBuffer[NUM_BUFFERS];
    INT     imgBufferId[NUM_BUFFERS];
  

    // auxiliary functions
      int CalcBitsPerPixel(INT ueye_colormode);

      cvgTimer limitTimer;

public:
    explicit CamUeye_driver(int w, int h,
                     int x, int y,
                     double framesPerSec,
                     double SetExposureMode
    				);
    CamUeye_driver();

    virtual ~CamUeye_driver();
    
    // MainFunctions
    bool startCam();
    bool stopCam();
    bool snapshot();
    bool startLive();
    void stopLive();
    void EnableDebug(bool bDebug);
    bool GetImage(char *image, cvg_ulong *timestamp = NULL);
    bool SetExposure(double expval);
    double getExposure();
    double getFps();
    bool setMasterGain(double gain);
    bool setRGBGains(double rGain, double gGain, double bGain);

    cvg_bool getImageSize(int *width, int *height);
};

 
 #ifdef __cplusplus
 }
 #endif

}

#endif // CAMUEYE_DRIVER_H
