#pragma once

#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <sstream>
#include <string>
#include <algorithm>
#include <cmath>
#include <numeric>
#include <functional>
//---------------------------------------------------
#include <opencv2/opencv.hpp>
//*********************************************
#include "Spinnaker.h"
#include "SpinGenApi/SpinnakerGenApi.h"
using namespace Spinnaker;
using namespace Spinnaker::GenApi;
using namespace Spinnaker::GenICam;
//-------------------------------------------
#include "callbackHandler.h"
#include "captureDef.h"
//#include "queueFPS2.h"
#include "sharedQueue.h"

/**********************************************************
*
***********************************************************/
// typedef for single stream ;
typedef std::unordered_map<std::string, pthread_t> flirHandlerThreadMap ;
typedef std::unique_ptr<cSharedQueue<ImagePtr>> 	 sImagePtrQueue ;
/*
*    Implements synchronization of multiple streams
*
*/
/**********************************************************
*
***********************************************************/
class cFLIRHandler
{
  public :
    enum eCaptureCMDType
    {
        CAPHANDLER_START = 100,
        CAPHANDLER_READY,
        CAPHANDLER_RUNNING,
        CAPHANDLER_STOP,
        CAPHANDLER_EXIT,
    } ;
    typedef enum {
        SPPixelConvertNone,
        SPPixelConvertMono8,
        SPPixelConvertMono16,
        SPPixelConvertRaw16,
        SPPixelConvertRGB8,
        SPPixelConvertRGB16
    } SPPixelConvert_t;
//----------------------------------------------------
    cFLIRHandler( std::string& devName, std::string& devAddress) ;
    ~cFLIRHandler( ) ;
//--------------------------------------------------------------------------
	  void            Connect( CameraPtr cameraPtr );
		cv::Mat&  			initFLIRHandler( CameraPtr cameraPtr, cv::Mat& thumbnail ) ;
    int         		printDeviceInfo(INodeMap& nodeMap) ;
    int         		configImageSettings(INodeMap& nodeMap) ;
		void 						capProperty( ) ;
		void 				  	stopHandler( ) ;
		void 				  	exitHandler( ) ;
//===============================================================
    void        		startCapture( ) ;
    void            captureFrame( std::string name ) ;
    void            generateFrame( std::string name ) ;
		void 				  	stopCapture( ) ;
//=======================================================
		std::string& 		getDevAddress( )	{ return mDevAddress ; } ;
    cv::Mat&        getThumbnail( )   { return mThumbnail ; } ;
    cv::Mat&        getFirstFrame( )  { return mFirstFrame ; } ;
		ImagePtr 				getSrcImage( cv::Mat&  srcFrame ) ;
		ImagePtr				ConvertImage( ImagePtr pImage ) ;
//======================================================================
		ImagePtr 				acquireImage() ;
//======================================================================
  	void          	cbCaptureImage( std::string name ) ;
    void     				requestCMD(sCBHandlerData* requestCmd) ;
    void     				handlerReply(sCBHandlerData* replyCmd ) ;
		template<typename T_object>
		cCallbackHandler::intCallbackID regCapNotify(T_object* object, void(T_object::*function)(void*,void*), void* user_data)
		{
			   return mCBCapture.registerCallback(object, function, user_data) ;
		}
		void 						unRegCapNotify(cCallbackHandler::intCallbackID id) ;
  private :
//=========================================================
    std::string              	  mDevName ;
    std::string              	  mDevAddress ;
		bool 			                  mCBCapStarted = false ;
//-------------------------------------------------------------
    CameraPtr 									pCameraPtr ;
    ImageProcessor 							mImageProcessor ;
    ImagePtr  									pSrcImage ;
    ImagePtr  									pAcquiredImage ;
    ImagePtr  									pConvertedImage ;
//----------------------------------------------------------------
		sImagePtrQueue							pImagePtrQueue ;
//    cQueueFPS<ImagePtr>  				qmImagePtr ;
		std::string 								mFLIRSerial ;       // serial number
    std::string 								mFLIRModel ;        // camera model number
//----------------------------------------------------
		cv::Mat                     mSrcFrame ;
		cv::Mat                     mFirstFrame ;
		cv::Mat                     mThumbnail ;
//----------------------------------------------------------
		int 												mOffsetX ;
		int 												mOffsetY ;
    int 												mImageWidth ;
		int 												mImageHeight ;
		int 												mImageStride ;
		std::string 								mPixelFormatS ;
    Spinnaker::PixelFormatEnums mPixelFormatEnums ;
//--------------------------------------------------------------
		sCaptureProperty					    smProperty ;
//		std::vector<std::string> 	    vmPropertyMsg ;    
    std::map<std::string, double> mmCamProperty ;
//-----------------------------------------------------------------
    std::mutex                  mBufferMutex ;
    std::condition_variable     mBufferCondition ;
//----------------------------------------------------------
    std::thread*           			pCapThread ;
		std::thread*								pPacketThread ;
    std::thread* 	              pCBCapThread ;
    flirHandlerThreadMap        mThreadMap ;
//===================================================
		cCallbackHandler 					 	mCBCapture ;
    std::queue<sCBHandlerData>  qmCmdQueue ;
    std::mutex                  mMutex ;
    std::condition_variable     mReplyCondition ;
		sCBHandlerData 							mReplyCmd ;
//===================================================
    int                 m_R;
    double              m_B;
    double              m_F;
    double              m_O;
    double              m_WT;
    double              m_W4WT;
//----------------------------------------
    int                 m_CamType;
    int					        m_Width;
    int					        m_Height;
    bool				        m_bTLUTCapable;
    bool				        m_bMeasCapable;
    double				      m_LowRangeLimit;
    double				      m_TOffset;
    bool				        m_bAlignAvailable;
    bool				        m_bFrameRateControlEnabled;
    bool				        m_bAdjustOnce;
    bool				        m_bAutoAdjust;
    bool				        m_bSingleRange;
    bool				        m_bFast;
    bool				        m_bFlyingSpotActive;
    bool				        m_bAreaActive;
    bool				        m_bAcquiringImages;
    int					        m_CalcCheckNeeded;
    int					        m_iPresentation;
    int					        m_WindowingMode;
    unsigned long       m_J0; // Global offset
    double              m_J1; // Global gain
    double              m_K1;
    double              m_K2;

// Object parameters
    double              m_Emissivity;
    double              m_ObjectDistance;
    double              m_AtmTemp;
    double              m_AmbTemp;
    double              m_AtmTao;
    double              m_ExtOptTransm;
    double              m_ExtOptTemp;
    double              m_RelHum;
// Spectral response parameters
    double              m_X;
    double              m_alpha1;
    double              m_alpha2;
    double              m_beta1;
    double              m_beta2;

// Alarm parameters
    bool				        m_bAlarmActive;
    int					        m_iAlarmFunc;
    int					        m_iAlarmResult;
    int					        m_iAlarmCondition;
};
