#pragma once
#include <sys/time.h>  // int gettimeofday(struct timeval *restrict tv, struct timezone *restrict tz);
#include <opencv2/opencv.hpp>

//*******************************************
//
//*********************************************
struct sMatTFrame
{
		cv::Mat 			 		mMatFrame ;
		struct timeval 		mCaptureTime ;
} ;

/**********************************************************
*
***********************************************************/
#define MVS_DTYPE int32_t

struct sAVFrame {
	struct timeval  mCaptureTime ;
  double          timestamp ;
  int             height ;
  int             width ;
  char            frame_type[2] ;
  int             frame_status ;
  uint8_t         *frame ;
  MVS_DTYPE       num_mvs ;
  MVS_DTYPE       *motion_vectors ;
} ;

/**********************************************************
*
***********************************************************/
struct sMatFrame {
	struct timeval  mCaptureTime ;
  double          timestamp ;
  cv::Mat         mMatFrame ;
  char            frame_type[2] ;
  int             frame_status ;
  MVS_DTYPE       num_mvs ;
  MVS_DTYPE       *motion_vectors ;
} ;

typedef std::shared_ptr<sAVFrame>  spAVFrame ;
typedef std::shared_ptr<sMatFrame> spMatFrame ;
//*******************************************
//
//*********************************************
struct sCaptureProperty
{
		unsigned int        mFourCC ;
		int 								mFrameCount ;
		int 								mFramePosi ;
		int 								mFormat ;
		int              		mWidth ;		// frame width ;
		int              		mHeight ;  // frame height ;
		int              		mChannels ;  		// frame channels ;
		double 							mDuration	;    // video file duration ( Sec.)
		double 						 	mFPS ;
} ;
//**********************************************
//
//*********************************************
struct sCapFrameROI {
    std::string             regionName ;
    int                     regionType ;
    std::string             signalName ;
    int                     actionType ;
    std::vector<cv::Point>  roiPts ;
		cv::Point								p1 ;			// bounding box p1 ;
		cv::Point								p2 ;			// bounding box p2 ; ;
} ;

struct sCapROILists {
    std::string     name;
    std::string     url ;
    std::vector<sCapFrameROI> regionLists ;
};

//*******************************************
//
//*********************************************
struct sThermalProperty
{
		Spinnaker::PixelFormatEnums mPixelFormat ;
		int 								mOffsetX ;
		int 								mOffsetY ;
		int 								mWidth ;
		int 								mHeight ;
		int 								mStride ;
		int 								mXPadding ;
		int 								mYPadding ;
		int                 mR;
		double              mB;
		double              mF;
		double              mO;
		double              mWT;
		double              mW4WT;
//----------------------------------------
	// Object parameters
    double              mEmissivity;
    double              mObjectDistance;
    double              mAtmTemp;
    double              mAmbTemp;
    double              mAtmTao;
    double              mExtOptTransm;
    double              mExtOptTemp;
    double              mRelHum;
// Spectral response parameters
    double              mX;
    double              mAlpha1;
    double              mAlpha2;
    double              mBeta1;
    double              mBeta2;
} ;
