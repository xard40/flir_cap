#include <Spinnaker.h>
#include <SpinGenApi/SpinnakerGenApi.h>
#include <opencv2/opencv.hpp>
#include <iostream>


using namespace Spinnaker;
using namespace Spinnaker::GenApi;
using namespace Spinnaker::GenICam;

cv::Mat getCVImage(ImagePtr rawImage, bool useMono16){
    ImageProcessor processor;
    int colorFormat = CV_8UC1;

    ImagePtr rawImage_mono8;

    cv::Mat normalizedImage;

    // Convert the image to OpenCV format
    //erase the mono16 format

    // Apply the color map to the normalized image
    cv::Mat colorImage;
    cv::applyColorMap(normalizedImage, colorImage, cv::COLORMAP_JET);

    // rawImage_mono8->Release();
    return colorImage;
}

CameraPtr getCameraFromIPAddress(CameraList camList, std::string ip_addr){
    char ipAddress[32];
    CameraPtr cam;

    char ip_addr_cstr[ip_addr.length() + 1];
    strcpy(ip_addr_cstr, ip_addr.c_str());

    for(int i=0; i < camList.GetSize(); ++i){
        cam = camList.GetByIndex(i);
        std::cout << "Camera " << i << ": "<< std::endl;
        try{
            cam->Init();
        }catch(Spinnaker::Exception e){
            std::cout << "ERROR" << std::endl;
            continue;
        }
        CValuePtr   ptrIntAddress = cam->GetTLDeviceNodeMap().GetNode("GevDeviceIPAddress");
        std::string ipString = ptrIntAddress->ToString().c_str() ;
        std::cout << "  IP address: " << ipString << std::endl;

        unsigned long ip = stoul(ipString, 0, 16);
        
        //erase the printed ID
        
    if(strcmp(ipAddress, ip_addr_cstr) != 0){
        std::cout << std::endl << ip_addr << " --> This IP can't use or other program is using." << std::endl;
        std::cout << "Catched IP: " << ipAddress << std::endl << std::endl;
        return 1;
    }
    return cam;
}

int main(int argc, char** argv)
{
    std::cout << "Start Program" << std::endl;

    // Initialize the FLIR SDK
    SystemPtr system = System::GetInstance();

    // Retrieve list of connected cameras
    CameraList camList = system->GetCameras();

    // Check that at least one camera is connected
    if (camList.GetSize() == 0) {
        std::cout << "No cameras detected." << std::endl;
        system->ReleaseInstance();
        return 1;
    }


    std::string ip_addr = "XXXXX";
    CameraPtr cam = getCameraFromIPAddress(camList, ip_addr);

    // Configure camera for acquiring images in Mono8 format
    CEnumerationPtr pixelFormat = cam->GetNodeMap().GetNode("PixelFormat");
    if (!IsAvailable(pixelFormat) || !IsWritable(pixelFormat)) {
        std::cout << "Unable to configure pixel format." << std::endl;
        cam->DeInit();
        camList.Clear();
        system->ReleaseInstance();
        return 1;
    }

    //pixelFormat->SetString("Mono8");

    // Begin image acquisition
    cam->BeginAcquisition();

    int colorFormat = CV_8UC1;

    // Create a color map for the thermal range display
    cv::Mat colormap;
    // cv::applyColorMap(cv::Mat::zeros(256, 1, colorFormat), colormap, cv::COLORMAP_JET);

    // Retrieve an image from the camera
    ImageProcessor processor;
    ImagePtr rawImage, rawImage_mono8;

    //erase state Flir

    // Release the image and end acquisition
    cam->EndAcquisition();

    // Deinitialize the camera and release resources
    cam->DeInit();
    camList.Clear();
    system->ReleaseInstance();

    return 0;
}
