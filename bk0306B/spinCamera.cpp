#include <string>
#include <exception>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <chrono>
#include <sys/stat.h>
#include <opencv2/opencv.hpp>
//---------------------------------------------
#include "Spinnaker.h"
#include "SpinGenApi/SpinnakerGenApi.h"

#include "spinManager.h"
#include "spinCamera.h"
using namespace Spinnaker;
using namespace Spinnaker::GenApi;
using namespace Spinnaker::GenICam;

/********************************************************************
*
*
*
*
*********************************************************************/
cSpinCamera::cSpinCamera(std::string serialNumber)
{
    pCameraPtr = cSpinManager::the_manager().get_camera(serialNumber);
    pCameraPtr->Init();
    pNodeMap = &pCameraPtr->GetNodeMap();
}


/********************************************************************
*
*********************************************************************/
cSpinCamera::cSpinCamera(int index)
{
    pCameraPtr = cSpinManager::the_manager().get_camera( index );
    pCameraPtr->Init();
    pNodeMap = &pCameraPtr->GetNodeMap();
}


/********************************************************************
*
*********************************************************************/
cSpinCamera::~cSpinCamera()
{
    pCameraPtr->DeInit();
}

// Print Device Info
/********************************************************************
*
*
*
*********************************************************************/
void cSpinCamera::print_device_info()
{
    INodeMap &device_node_map = pCameraPtr->GetTLDeviceNodeMap();
    FeatureList_t features;
    CCategoryPtr category = device_node_map.GetNode("DeviceInformation");
    if (IsAvailable(category) && IsReadable(category)) {
        category->GetFeatures(features);
        for (auto it = features.begin(); it != features.end(); ++it) {
            CNodePtr feature_node = *it;
            std::cout << feature_node->GetName() << " : ";
            CValuePtr value_ptr = (CValuePtr)feature_node;
            std::cout << (IsReadable(value_ptr) ? value_ptr->ToString() : "Node not readable");
            std::cout << std::endl;
        }
    } else {
        std::cout << "Device control information not available." << std::endl;
    }
    printf("\nEnd of cSpinCamera::print_device_info() \n\n") ;
}

  // Start/End camera

/********************************************************************
*
*********************************************************************/
void cSpinCamera::start()
{
    pCameraPtr->AcquisitionMode.SetValue(AcquisitionModeEnums::AcquisitionMode_Continuous);
    pCameraPtr->BeginAcquisition();
    mImageProcessor.SetColorProcessing(SPINNAKER_COLOR_PROCESSING_ALGORITHM_HQ_LINEAR) ;
}

/********************************************************************
*
*********************************************************************/
void cSpinCamera::end()
{
    pCameraPtr->EndAcquisition();
}

  // Setting Hardware/Software Trigger

/********************************************************************
*
*********************************************************************/
void cSpinCamera::enable_hardware_trigger()
{
    this->disable_trigger();
    pCameraPtr->TriggerSource.SetValue(TriggerSourceEnums::TriggerSource_Line0);
    pCameraPtr->TriggerMode.SetValue(TriggerModeEnums::TriggerMode_On);
    pCameraPtr->TriggerSelector.SetValue(TriggerSelectorEnums::TriggerSelector_FrameStart);
    pCameraPtr->TriggerActivation.SetValue(TriggerActivationEnums::TriggerActivation_RisingEdge);
}


/********************************************************************
*
*********************************************************************/
void cSpinCamera::enable_software_trigger()
{
    this->disable_trigger();
    pCameraPtr->TriggerSource.SetValue(TriggerSourceEnums::TriggerSource_Software);
    pCameraPtr->TriggerMode.SetValue(TriggerModeEnums::TriggerMode_On);
}

/********************************************************************
*
*********************************************************************/
void cSpinCamera::disable_trigger()
{
    if (pCameraPtr->TriggerMode == NULL || pCameraPtr->TriggerMode.GetAccessMode() != RW) {
        std::cout << "Unable to disable trigger mode. Aborting..." << std::endl;
        exit(-1);
    }
    pCameraPtr->TriggerMode.SetValue(TriggerModeEnums::TriggerMode_Off);
}

  // Setting Black Level

/********************************************************************
*
*********************************************************************/
void cSpinCamera::enable_black_level_auto()
{
    pCameraPtr->BlackLevelAuto.SetValue(BlackLevelAutoEnums::BlackLevelAuto_Continuous);
}

/********************************************************************
*
*********************************************************************/
void cSpinCamera::disable_black_level_auto()
{
    pCameraPtr->BlackLevelAuto.SetValue(BlackLevelAutoEnums::BlackLevelAuto_Off);
}

/********************************************************************
*
*********************************************************************/
void cSpinCamera::set_black_level(double black_level)
{
    pCameraPtr->BlackLevel.SetValue(black_level);
}

/********************************************************************
*
*********************************************************************/
double cSpinCamera::get_black_level() const
{
    return pCameraPtr->BlackLevel.GetValue();
}

  // Setting Frame Rate

/********************************************************************
*
*********************************************************************/
void cSpinCamera::enable_frame_rate_auto()
{
    CBooleanPtr AcquisitionFrameRateEnable = pNodeMap->GetNode("AcquisitionFrameRateEnable");
    if (!IsAvailable(AcquisitionFrameRateEnable) || !IsReadable(AcquisitionFrameRateEnable)) {
        std::cout << "Unable to enable frame rate." << std::endl;
        return;
    }
    AcquisitionFrameRateEnable->SetValue(0);
}

/********************************************************************
*
*********************************************************************/
void cSpinCamera::disable_frame_rate_auto()
    {
        CBooleanPtr AcquisitionFrameRateEnable = pNodeMap->GetNode("AcquisitionFrameRateEnable");
        if (!IsAvailable(AcquisitionFrameRateEnable) || !IsReadable(AcquisitionFrameRateEnable)) {
            std::cout << "Unable to enable frame rate." << std::endl;
            return;
        }
        AcquisitionFrameRateEnable->SetValue(1);
    }

/********************************************************************
*
*********************************************************************/
void cSpinCamera::set_frame_rate(double frame_rate)
{
    std::cout << "cSpinCamera::set_frame_rate" << std::endl;
    pCameraPtr->AcquisitionFrameRate.SetValue(frame_rate);
}

/********************************************************************
*
*********************************************************************/
double cSpinCamera::get_frame_rate() const
{
    return pCameraPtr->AcquisitionFrameRate.GetValue();
}

  // Setting Exposure Time, us

/********************************************************************
*
*********************************************************************/
void cSpinCamera::enable_exposure_auto()
{
    pCameraPtr->ExposureAuto.SetValue(ExposureAutoEnums::ExposureAuto_Continuous);
}

/********************************************************************
*
*********************************************************************/
void cSpinCamera::disable_exposure_auto()
{
    pCameraPtr->ExposureAuto.SetValue(ExposureAutoEnums::ExposureAuto_Off);
    pCameraPtr->ExposureMode.SetValue(ExposureModeEnums::ExposureMode_Timed);
}

/********************************************************************
*
*********************************************************************/
void cSpinCamera::set_exposure_time(double exposure_time)
{
    std::cout << "cSpinCamera::set_exposure_time" << std::endl;
    pCameraPtr->ExposureTime.SetValue(exposure_time);
}

/********************************************************************
*
*********************************************************************/
double cSpinCamera::get_exposure_time() const
{
    return pCameraPtr->ExposureTime.GetValue();
}

/********************************************************************
*
*********************************************************************/
void cSpinCamera::set_exposure_upperbound(double value)
{
    CFloatPtr AutoExposureExposureTimeUpperLimit = pNodeMap->GetNode("AutoExposureExposureTimeUpperLimit");
    AutoExposureExposureTimeUpperLimit->SetValue(value);
}

  // Setting Gain

/********************************************************************
*
*********************************************************************/
void cSpinCamera::enable_gain_auto()
{
    pCameraPtr->GainAuto.SetValue(GainAutoEnums::GainAuto_Continuous);
}

/********************************************************************
*
*********************************************************************/
void cSpinCamera::disable_gain_auto()
{
    pCameraPtr->GainAuto.SetValue(GainAutoEnums::GainAuto_Off);
}

/********************************************************************
*
*********************************************************************/
void cSpinCamera::set_gain(double gain)
{
    pCameraPtr->Gain.SetValue(gain);
}

/********************************************************************
*
*********************************************************************/
double cSpinCamera::get_gain() const
{
    return pCameraPtr->Gain.GetValue();
}

  // Setting Gamma

/********************************************************************
*
*********************************************************************/
void cSpinCamera::set_gamma(double gamma)
{
    pCameraPtr->Gamma.SetValue(gamma);
}

/********************************************************************
*
*********************************************************************/
double cSpinCamera::get_gamme() const
{
    return pCameraPtr->Gamma.GetValue();
}

  // Setting White Balance

/********************************************************************
*
*********************************************************************/
void cSpinCamera::enable_white_balance_auto()
{
    pCameraPtr->BalanceWhiteAuto.SetValue(BalanceWhiteAutoEnums::BalanceWhiteAuto_Continuous);
}

/********************************************************************
*
*********************************************************************/
void cSpinCamera::disable_white_balance_auto()
{
    pCameraPtr->BalanceWhiteAuto.SetValue(BalanceWhiteAutoEnums::BalanceWhiteAuto_Off);
}

/********************************************************************
*
*********************************************************************/
void cSpinCamera::set_white_balance_blue(double value)
{
    pCameraPtr->BalanceRatioSelector.SetValue(BalanceRatioSelectorEnums::BalanceRatioSelector_Blue);
    CFloatPtr BalanceRatio = pNodeMap->GetNode("BalanceRatio");
    BalanceRatio->SetValue(value);
}

/********************************************************************
*
*********************************************************************/
void cSpinCamera::set_white_balance_red(double value)
{
    pCameraPtr->BalanceRatioSelector.SetValue(BalanceRatioSelectorEnums::BalanceRatioSelector_Red);
    CFloatPtr BalanceRatio = pNodeMap->GetNode("BalanceRatio");
    BalanceRatio->SetValue(value);
}

/********************************************************************
*
*********************************************************************/
double cSpinCamera::get_white_balance_blue() const
{
    pCameraPtr->BalanceRatioSelector.SetValue(BalanceRatioSelectorEnums::BalanceRatioSelector_Blue);
    CFloatPtr BalanceRatio = pNodeMap->GetNode("BalanceRatio");
    return BalanceRatio->GetValue();
}

/********************************************************************
*
*********************************************************************/
double cSpinCamera::get_white_balance_red() const
{
    pCameraPtr->BalanceRatioSelector.SetValue(BalanceRatioSelectorEnums::BalanceRatioSelector_Red);
    CFloatPtr BalanceRatio = pNodeMap->GetNode("BalanceRatio");
    return BalanceRatio->GetValue();
}

// Get Image Timestamp
/********************************************************************
*
*********************************************************************/
uint64_t cSpinCamera::get_system_timestamp() const
{
    return mSystemTimestamp;
}
/********************************************************************
*
*********************************************************************/
uint64_t cSpinCamera::get_image_timestamp() const
{
    return mImageTimestamp;
}

  // Grab Next Image
/********************************************************************
*
*********************************************************************/
cv::Mat cSpinCamera::grab_next_image(const std::string& format)
{
    ImagePtr image_ptr= pCameraPtr->GetNextImage();
    mSystemTimestamp = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
    mImageTimestamp = image_ptr->GetTimeStamp();
    int width = image_ptr->GetWidth();
    int height = image_ptr->GetHeight();
    cv::Mat result;
    if (format == "bgr") {
//        ImagePtr converted_image_ptr = image_ptr->Convert(PixelFormat_BGR8);
        ImagePtr converted_image_ptr = mImageProcessor.Convert(image_ptr, PixelFormat_BGR8);
        cv::Mat temp_img(height, width, CV_8UC3, converted_image_ptr->GetData());
        result = temp_img.clone();
    } else if (format == "rgb") {
//        ImagePtr converted_image_ptr = image_ptr->Convert(PixelFormat_RGB8);
        ImagePtr converted_image_ptr = mImageProcessor.Convert(image_ptr, PixelFormat_RGB8);
        cv::Mat temp_img(height, width, CV_8UC3, converted_image_ptr->GetData());
        result = temp_img.clone();
    } else if (format == "gray") {
//        ImagePtr converted_image_ptr = image_ptr->Convert(PixelFormat_Mono8);
        ImagePtr converted_image_ptr = mImageProcessor.Convert(image_ptr, PixelFormat_Mono8);
        cv::Mat temp_img(height, width, CV_8UC1, converted_image_ptr->GetData(), converted_image_ptr->GetStride());
        result = temp_img.clone();
    } else {
        throw std::invalid_argument("Invalid argument: format = " + format + ". Expected bgr, rgr, or gray.");
    }
    image_ptr->Release();
    return result;
}

/********************************************************************
*
*********************************************************************/
void cSpinCamera::trigger_software_execute()
{
    pCameraPtr->TriggerSoftware.Execute();
}
