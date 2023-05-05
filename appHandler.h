#pragma once
#include <string>
#include <vector>
#include <sys/time.h>  // int gettimeofday(struct timeval *restrict tv, struct timezone *restrict tz);
//**********************************************
#include <opencv2/opencv.hpp>
#include <yaml-cpp/yaml.h>
//*********************************************
#include "Spinnaker.h"
#include "SpinGenApi/SpinnakerGenApi.h"
using namespace Spinnaker;
using namespace Spinnaker::GenApi;
//-----------------------------------------------------------
#include "callbackHandler.h"
#include "flirHandler.h"
#include "flirUtils.h"

//**********************************************
//
//*********************************************
class cAppHandler
{
  public :
	//----------------------------------------------------
    cAppHandler( const char* appPath ) ;
	  ~cAppHandler( )	;
//=====  functions of server side ===================================
    int 				InitHandler( std::string configFile ) ;
    void        GoHandler( ) ;
    void 				ExitHandler( ) ;
  private :
//===================================================================
    int         getSpinSystemConfig( ) ;
    int         readCaptureConfig( const YAML::Node &capConfig ) ;
    void 				checkROILists(sCapDevice& capDevice, int width, int height);
    int         configFLIRHandler( ) ;
		void 				goEngine( uint16_t* pData, cv::Mat& frame, sCapDevice& capDevices ) ;
//-----------------------------------------------------------
    void        showImageMat(void* mat_ptr, const char *name) ;
//================================================================
	private :
    std::string                 mAppPath ;
//------------------------------------------------------
    SystemPtr                         mSpinSystem ;
    CameraList                        mCameraList ;
    std::map<std::string, CameraPtr>  mmCameraPtr ;   // key: url , value: capHandler
//-----------------------------------------------------
    sPalette                          mPalette ;
    std::vector<std::string>          vmPaletteLists ;
//=========================================================
    std::vector<sCapDevice>             vmCapDevices ;
    std::map<std::string, sCapDevice>   mmCapDevices ; // key: url , value: sCapROILists
    std::map<std::string, cFLIRHandler*> mmFLIRHandler ;  // key: url , value: capHandler
//-----------------------------------------------------
    void    captureNotify(void* trigger_data, void* user_data) ;
		cCallbackHandler::intCallbackID   mCaptureCBId = 0 ;
} ;
