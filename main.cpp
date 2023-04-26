#include <chrono>
#include <csignal>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>
//----------------------------------------------------
#include <opencv2/opencv.hpp>
#include "appHandler.h"

//========================================
#include "spinCamera.h"

//=============================================
std::unique_ptr<cAppHandler>  pAppHandler = nullptr ;
//----------------------------------------------
volatile  bool isRunning = true ;
volatile  bool isSave = false ;
//**********************************************
//
//*********************************************
static void sigHandler(int signum)
{
    if (signum == SIGINT)
      isRunning = false ;
    else if (signum == SIGABRT )
      isRunning = false ;
    else if (signum == SIGSEGV )
      isRunning = false ;
    else if (signum == SIGTERM )
      isRunning = false ;
    else if (signum == SIGHUP )
      isRunning = false ;
    else
      isRunning = true;
}
//***************************************************************
//
//***************************************************************
static std::unique_ptr<cAppHandler> createAPP( std::string vonfigFile )
{
  const char* handlerName = std::string(getenv("HOME")).c_str() ;
    std::unique_ptr<cAppHandler> appHandler = std::make_unique<cAppHandler>( handlerName ) ;
//----------------------------------------------------------------------
    int configVideoNumber = appHandler->InitHandler( vonfigFile ) ;
    if ( configVideoNumber <= 0 ) {
        printf("createApp( %d ) failed\n", configVideoNumber) ;
        appHandler = nullptr ;
    }
    return appHandler ;
}

/******************************************************************
 *
******************************************************************/
int main(int argc, char** argv)
{
    signal(SIGINT, sigHandler) ;
//========================================
/*
    cSpinCamera spinCam( 0 ) ;
    spinCam.print_device_info();
//    spinCam.disable_trigger();
//    spinCam.disable_frame_rate_auto();;
//    spinCam.disable_exposure_auto();
//    spinCam.set_frame_rate(20);
//    spinCam.set_exposure_time(5000);
    spinCam.start();
    try {
        while (true) {
            cv::Mat img1 = spinCam.grab_next_image("gray") ;
//            cv::Mat img1 = spinCam.grab_next_image("bgr") ;
            cv::Mat dst1;
            cv::resize(img1, dst1, cv::Size(0, 0), 0.5, 0.5);
            cv::imshow("spinCamImage", img1);
            cv::imshow("spinCamResized", dst1);
            cv::waitKey(1);
            if (! isRunning) break;
        }
    } catch (Spinnaker::Exception &e) {
        std::cout << "Error: " << e.what() << std::endl;
    }
    spinCam.end();
*/
//===============================================
  std::string configFile ;
    if (argc < 2) {
        std::cout << "Usage: ./goFLIR ./config/videoConfig.yaml" << std::endl;
        configFile = "./config/videoConfig.yaml" ;
    }
    else configFile = argv[1] ;
    pAppHandler = createAPP( configFile ) ;
    if (! pAppHandler )   {
        std::cout << "Create AppHandler failed !" << std::endl ;
        return -1 ;
    }
    isRunning = true ;
//===================================================================
	  while( isRunning )  {
        pAppHandler->GoHandler( ) ;
        int key = cv::waitKey(10);
        if (key == 27)  break;
        if (key == 's') cv::waitKey(0);
    }
    pAppHandler->ExitHandler( ) ;
    return 0;
}
