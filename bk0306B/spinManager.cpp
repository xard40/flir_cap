#include <string>
//#include <exception>
//#include <iostream>
//#include <iomanip>
//#include <sstream>
//#include <chrono>
//#include <sys/stat.h>
#include <opencv2/opencv.hpp>
//------------------------------------------
#include "Spinnaker.h"
#include "SpinGenApi/SpinnakerGenApi.h"

#include "spinManager.h"
using namespace Spinnaker;
using namespace Spinnaker::GenApi;
using namespace Spinnaker::GenICam;

/********************************************************************
*
*********************************************************************/
cSpinManager::cSpinManager()
{
    pSpinSystem = Spinnaker::System::GetInstance();
    const LibraryVersion spinnakerLibraryVersion = pSpinSystem->GetLibraryVersion();
    std::cout << "Spinnaker library version: " << spinnakerLibraryVersion.major
              << "." << spinnakerLibraryVersion.minor << "."
              << spinnakerLibraryVersion.type << "."
              << spinnakerLibraryVersion.build << std::endl
              << std::endl;
    mCameraList = pSpinSystem->GetCameras();
    int numCameras = mCameraList.GetSize();
    printf("Number of FLIR cameras detected: %d\n\n", numCameras) ;
    if (numCameras == 0) {
        printf("no FLIR camera detected !\n") ;
        Exit() ;
    }
    for (int i = 0; i < numCameras; i++)  {
        CameraPtr pCamera = mCameraList.GetByIndex(i);
        Spinnaker::GenApi::CIntegerPtr ptrIntAddress = pCamera->GevCurrentIPAddress.GetNode();
		    int64_t ip = ptrIntAddress->GetValue() ;
        char ipAddress[32] ;
        sprintf(ipAddress, "%ld.%ld.%ld.%ld",
                ((ip / 256) / 256) / 256, ((ip / 256) / 256) % 256, (ip / 256) % 256, ip % 256 ) ;
        printf("IPAddress: %lx (%s ==> %ld.%ld.%ld.%ld)\n", ip,  ipAddress,
              ((ip / 256) / 256) / 256,
              ((ip / 256) / 256) % 256,
              (ip / 256) % 256,
              ip % 256 ) ;
//      MAC address
        Spinnaker::GenApi::CIntegerPtr ptrIntMAC = pCamera->GevMACAddress.GetNode();
        int64_t macAddr = ptrIntMAC->GetValue();
        printf("MACAddress: %lX (%02lX.%02lX.%02lX.%02lX.%02lX.%02lX)\n\n", macAddr,
              ((macAddr / 256) / 256) / 256 / 256 / 256,
              ((macAddr / 256) / 256 / 256 / 256) % 256,
              (macAddr / 256 / 256 / 256) % 256,
              (macAddr / 256 / 256) % 256,
              (macAddr / 256) % 256,
               macAddr % 256 ) ;
        mmCameraPtr.insert(std::pair<std::string, CameraPtr>(ipAddress, pCamera)) ;
    }
}

/********************************************************************
*
*********************************************************************/
cSpinManager::~cSpinManager()
{
    Exit( ) ;
}

/********************************************************************
*
*********************************************************************/
cSpinManager& cSpinManager::the_manager()
{
    static cSpinManager m;
    return m;
}

/********************************************************************
*
*********************************************************************/
CameraPtr cSpinManager::get_camera(const std::string& serial_number) const
{
    std::cout << mCameraList.GetSize()<< std::endl;
    return mCameraList.GetBySerial(serial_number);
}


/********************************************************************
*
*********************************************************************/
CameraPtr cSpinManager::get_camera(int index) const
{
    std::cout << mCameraList.GetSize()<< std::endl;
    return mCameraList.GetByIndex( index );
}


//----------------------------------------------------------------------------
// exitHandler
//----------------------------------------------------------------------------
void cSpinManager::Exit()
{
    mCameraList.Clear();
    pSpinSystem->ReleaseInstance();
}
