#include <chrono>
#include <csignal>
//#include <iostream>
//#include <sstream>
//#include <string>
//#include <thread>
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
static std::unique_ptr<cAppHandler> initAPP( std::string configFile )
{
//  const char* home_wd = std::string(getenv("HOME")).c_str() ;
  char current_wd[1024] ;
    if (getcwd(current_wd, sizeof(current_wd)) == NULL)
        printf("unKnown Current working dir: %s\n", current_wd) ;
    std::unique_ptr<cAppHandler> appHandler = std::make_unique<cAppHandler>( current_wd ) ;
//----------------------------------------------------------------------
    int configVideoNumber = appHandler->InitHandler( configFile ) ;
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

//===============================================
  std::string configFile ;
    if (argc < 2) {
        std::cout << "Usage: ./goFLIR ./config/flirConfig.yaml" << std::endl;
        configFile = "./config/flirConfig.yaml" ;
    }
    else configFile = argv[1] ;
    pAppHandler = initAPP( configFile ) ;
    if (! pAppHandler )   {
        std::cout << "Create AppHandler failed !" << std::endl ;
        return -1 ;
    }
    isRunning = true ;
//===================================================================
	  while( isRunning )  {
        pAppHandler->GoHandler( ) ;

        char key = cv::waitKey(1);
        if (key == 27 || key == 'q')
            break;
        if (key == 's')
            cv::waitKey(0);
    }
    pAppHandler->ExitHandler( ) ;
    return 0;
}
