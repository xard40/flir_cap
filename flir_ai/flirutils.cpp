#include <Spinnaker.h>
#include <SpinGenApi/SpinnakerGenApi.h>
#include <opencv2/opencv.hpp>
#include <iostream>

using namespace Spinnaker;
using namespace Spinnaker::GenApi;
using namespace Spinnaker::GenICam;

int main(int argc, char** argv)
{
    // Initialize the FLIR SDK
    SystemPtr system = System::GetInstance();
    // system->Initialize();

    // Retrieve list of connected cameras
    CameraList camList = system->GetCameras();

    // Check that at least one camera is connected
    if (camList.GetSize() == 0) {
        std::cout << "No cameras detected." << std::endl;
        system->ReleaseInstance();
        return 1;
    }

    // Open the first connected camera
    CameraPtr cam = camList.GetByIndex(0);
    cam->Init();

    // Configure camera for acquiring images in Mono8 format
    CEnumerationPtr pixelFormat = cam->GetNodeMap().GetNode("PixelFormat");
    if (!IsAvailable(pixelFormat) || !IsWritable(pixelFormat)) {
        std::cout << "Unable to configure pixel format." << std::endl;
        cam->DeInit();
        camList.Clear();
        system->ReleaseInstance();
        return 1;
    }

    // pixelFormat->SetString("Mono8");

    // Begin image acquisition
    cam->BeginAcquisition();

    // Convert the image from Mono8 to Int8
    cv::Mat outputImage;

    // Retrieve an image from the camera
    ImagePtr rawImage;

    while(true){

        rawImage = cam->GetNextImage();

        // Convert the image to OpenCV format
        cv::Mat inputImage(rawImage->GetHeight(), rawImage->GetWidth(), CV_16UC1, rawImage->GetData());

        
        
        //inputImage.convertTo(outputImage, CV_16UC3);
        //  inputImage.convertTo(outputImage, CV_8S);

        cv::imshow("Image", inputImage);
        // cv::imshow("Output", outputImage);


        int key = cv::waitKey(1);
        if (key == 27) {
            break;
        }
    }




    // Save the Int8 image
    if (!cv::imwrite("output_int8_image.png", outputImage)) {
        std::cout << "Failed to save output image." << std::endl;
        rawImage->Release();
        cam->EndAcquisition();
        cam->DeInit();
        camList.Clear();
        system->ReleaseInstance();
        return 1;
    }

    std::cout << "Conversion complete." << std::endl;

    // Release the image and end acquisition
    rawImage->Release();
    cam->EndAcquisition();

    // Deinitialize the camera and release resources
    cam->DeInit();
    camList.Clear();
    system->ReleaseInstance();

    return 0;
}
