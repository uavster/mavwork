/*
 * OpticalFlow.h
 *
 *  Created on: 25/05/2012
 *      Author: Ignacio Mellado-Bataller
 */

#ifndef OPTICALFLOW_H_
#define OPTICALFLOW_H_

#include <cv.h>
#include <atlante.h>

namespace Mav {
namespace Linkquad {

class OpticalFlow {
private:
	IplImage *currentFrame;
	IplImage *lastFrame;
	IplImage *eigImage, *tempImage;
	IplImage *pyramid1, *pyramid2;

	int lastFrameFeaturesCount;
	CvPoint2D32f *lastFrameFeatures, *newFeatures, *lastFrameFeaturesCopy;
	char *foundFeatures;
	float *errorFeatures;

	cvg_int maxNumFeatures;

	cvg_bool firstRun;
	cvg_bool showDebug;
	IplImage *debugImage;

	Vector3 globalDisplacement;

	void outputDebugInfo(IplImage *frame, cvg_int numFeatures, char *foundFeatures, CvPoint2D32f *lastFeatures, CvPoint2D32f *currentFeatures);

public:
	OpticalFlow();
	virtual ~OpticalFlow();

	void create(cvg_int imageWidth, cvg_int imageHeight, cvg_int numFeatures);
	void destroy();

	cvg_bool process(IplImage *image, cvg_int *prevFrameNumFeatures, CvPoint2D32f **prevFrameFeatures, char **foundFeaturesInCurrentFrame, CvPoint2D32f **currentFrameFeatures);

	inline void enableDebug(cvg_bool e, IplImage *debugImage) { showDebug = e; this->debugImage = debugImage; }
//	inline const Vector3 &getGlobalDisplacement() { return globalDisplacement; }

};

}
}

#endif /* OPTICALFLOW_H_ */
