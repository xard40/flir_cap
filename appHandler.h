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
    int         configFLIRHandler( ) ;
//-----------------------------------------------------------
    void        showImageMat(void* mat_ptr, const char *name) ;
//================================================================
	private :
    std::string                 mAppPath ;
//------------------------------------------------------
    SystemPtr                   mSpinSystem ;
    CameraList                  mCameraList ;
//-----------------------------------------------------
//    CameraPtr                   pActiveCameraPtr = nullptr ;
//    ImagePtr                    pRawImage ;
//    ImagePtr                    pConvertedImage ;
    sPalette                    mPalette ;
    std::vector<std::string>    vmPaletteLists ;
//--------------------------------------------------------
    std::vector<CameraPtr>      vpCameraPtr ;
    std::vector<sCapROILists>   vmCapROILists ;
//=========================================================
    std::map<std::string, CameraPtr>      mmCameraPtr ;   // key: url , value: capHandler
    std::map<std::string, cFLIRHandler*>  mmFLIRHandler ;  // key: url , value: capHandler
//------------------------------------------------------------
    std::map<std::string, sCapROILists>   mmCapROILists ; // key: url , value: sCapROILists
//-----------------------------------------------------
    void    captureNotify(void* trigger_data, void* user_data) ;
		cCallbackHandler::intCallbackID   mCaptureCBId = 0 ;
} ;
