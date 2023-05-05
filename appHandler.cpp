#include <fstream>
#include <sstream>
#include <iostream>
//---------------------------------------------
#include "appHandler.h"

//*******************************************************************
//
//********************************************************************
const std::vector<cv::Scalar> thermalROIColors = {
  cv::Scalar(  0,   0,   0),
  cv::Scalar( 42,  78, 250),
  cv::Scalar(  0, 255, 255),
  cv::Scalar(180, 224, 192),
  cv::Scalar(255,   0,   0),
  cv::Scalar(255,   0, 255),
//------------------------------------------
  cv::Scalar(128, 196, 214),
  cv::Scalar(214, 192,  96),
  cv::Scalar(128,   0, 192),
  cv::Scalar(0  ,  96, 196),
};
//***********************************************************
//   refer to the struct sCapDevice in thermalDef.h
//************************************************************
namespace YAML {
    template<>
    struct convert<sCapDevice> {
        static Node encode(const sCapDevice &capDevice) {
            Node node;
            node.push_back(capDevice.name);
            node.push_back(capDevice.url);
            printf("devName: %s, URL: %s\n", capDevice.name.c_str(), capDevice.url.c_str()) ;
            return node;
        }
        static bool decode(const Node &node, sCapDevice &capDevice) {
            capDevice.name = node["devName"].as<std::string>();
            capDevice.url = node["devURL"].as<std::string>();
            YAML::Node roiLists = node["ROILists"];
            sCaptureROI  frame_roi ;
            cv::Point cv_pt ;
            int area_id = 0 ;
            capDevice.roiLists.clear() ;
            for (YAML::const_iterator roi_it = roiLists.begin(); roi_it != roiLists.end(); ++roi_it) {
                YAML::Node roiLists_map = roi_it->as<YAML::Node>();
                int list_id = 0 ;
                frame_roi.roiPts.clear() ;
                for (YAML::const_iterator pt_it = roiLists_map.begin(); pt_it != roiLists_map.end(); ++pt_it) {
                    YAML::Node::const_iterator map_it = pt_it->begin();
                    std::string key = map_it->first.as<std::string>();
                    int value = map_it->second.as<int>() ;
                    if (list_id == 0)  {
                        frame_roi.roiName = key ;
                        frame_roi.roiType = value ;
                    }
                    else if (list_id == 1) {
                        frame_roi.signalName = key ;
                        frame_roi.actionType = value ;
                    }
                    else {
                        if ( (list_id - 2) % 2 == 0)  {
                             cv_pt.x = value ;
                        }
                        else  {
                            cv_pt.y = value ;
                            frame_roi.roiPts.push_back( cv_pt ) ;
                        }
                    }
                    list_id += 1 ;
                }
                capDevice.roiLists.push_back( frame_roi ) ;
                area_id += 1 ;
            }
            return true ;
        }
    };
}

//***********************************************************
//
//************************************************************
cAppHandler::cAppHandler( const char *app_path )
{
    mAppPath = app_path ;
    printf("cAppHandler::cAppHandler( %s )\n", mAppPath.c_str()) ;
}

//***********************************************************
//
//************************************************************
cAppHandler::~cAppHandler( )
{
    ExitHandler( ) ;
}

//***********************************************************************
//
//  function support of Server side
//
//***********************************************************************
int cAppHandler::InitHandler( std::string configFile )
{
  int detectedCameras, configNumber = 0 ;
//-------------------------------------------
    YAML::Node capConfig = YAML::LoadFile(configFile) ;
    if ( readCaptureConfig( capConfig ) > 0)  {
        detectedCameras = getSpinSystemConfig( ) ;
        if ( detectedCameras > 0)
            configNumber = configFLIRHandler( ) ;

        vmPaletteLists.push_back("Linear_red_palettes") ;
        vmPaletteLists.push_back("Linear_palettes") ;
        vmPaletteLists.push_back("GammaLog_red_palettes") ;
        vmPaletteLists.push_back("GammaLog_palettes") ;
        vmPaletteLists.push_back("Inversion_red_palette") ;
        vmPaletteLists.push_back("Inversion_palette") ;
        vmPaletteLists.push_back("False_color_palette1") ;
        vmPaletteLists.push_back("False_color_palette2") ;
        vmPaletteLists.push_back("False_color_palette3") ;
        vmPaletteLists.push_back("False_color_palette4") ;
        vmPaletteLists.push_back("False_color_palette5") ;
        vmPaletteLists.push_back("False_color_palette6") ;

        mPalette = GetPalette( vmPaletteLists[11] );
    }
    return configNumber ;
}

/************************************************************************************
*
************************************************************************************/
int cAppHandler::readCaptureConfig( const YAML::Node &capConfig )
{
  int totalDevices = 0 ;

    if (capConfig["flirDevices"]) {
        vmCapDevices = capConfig["flirDevices"].as<std::vector<sCapDevice>>();
        for (std::vector<sCapDevice>::iterator it = vmCapDevices.begin(); it != vmCapDevices.end(); ++it) {
            totalDevices += 1 ;
        }
    }
    return totalDevices ;
}

/************************************************************************************
*
************************************************************************************/
int cAppHandler::getSpinSystemConfig( )
{
    std::cout << std::endl << "======  Spinnaker System Information : =============== "  << std::endl ;
    mSpinSystem = Spinnaker::System::GetInstance() ;
    const LibraryVersion spinnakerLibraryVersion = mSpinSystem->GetLibraryVersion();
    std::cout << "Spinnaker library version: " << spinnakerLibraryVersion.major
              << "." << spinnakerLibraryVersion.minor << "."
              << spinnakerLibraryVersion.type << "."
              << spinnakerLibraryVersion.build << std::endl ;

    mCameraList = mSpinSystem->GetCameras();
    int numCameras = mCameraList.GetSize();
    if (numCameras == 0) {
        ExitHandler() ;
        printf("no FLIR camera detected !\n") ;
        return  0 ;
    }
    else printf("%d FLIR camera(s) detected !\n", numCameras) ;
    for (int id = 0; id < numCameras; id++)  {
        CameraPtr cameraPtr = mCameraList.GetByIndex(id);
        CValuePtr ptr_ipAddress = cameraPtr->GetTLDeviceNodeMap().GetNode("GevDeviceIPAddress");
        std::string ipString = ptr_ipAddress->ToString().c_str() ;
        unsigned long ip = stoul(ipString, 0, 16) ;
        char ipAddress[32] ;
        sprintf(ipAddress, "%ld.%ld.%ld.%ld",
                ((ip / 256) / 256) / 256, ((ip / 256) / 256) % 256, (ip / 256) % 256, ip % 256 ) ;
				printf("CamId. %d, GevDeviceIPAddress: %s (%s)\n", id, ipString.c_str(), ipAddress) ;
        mmCameraPtr.insert(std::pair<std::string, CameraPtr>(ipAddress, cameraPtr)) ;
    }
    return numCameras ;
}

//============================================================================
//    std::vector<sCapDevice>   vmCapDevices ;
//============================================================================
int cAppHandler::configFLIRHandler(  )
{
  cv::Mat *firstFrame, thumbnail ;
    for (sCapDevice capDevice : vmCapDevices)   {
        std::string devURL = capDevice.url ;
        std::string devName = capDevice.name ;
//---------------------------------------------------------
        std::map<std::string, CameraPtr>::iterator camera_it = mmCameraPtr.begin() ;
        while (camera_it != mmCameraPtr.end())   {
            if ( camera_it->first == devURL) {
                cFLIRHandler* flirHandler = new cFLIRHandler(devName, devURL) ;
                CameraPtr cameraPtr = camera_it->second ;
                firstFrame = flirHandler->initFLIRHandler( cameraPtr, thumbnail ) ;
                if ( firstFrame )   {
                    checkROILists(capDevice, firstFrame->cols, firstFrame->rows) ;
                    mmCapDevices.insert(std::pair<std::string, sCapDevice>(devURL, capDevice)) ;
                    mmFLIRHandler.insert(std::pair<std::string, cFLIRHandler*>(devURL, flirHandler)) ;
                    mCaptureCBId = flirHandler->regCapNotify(this, &cAppHandler::captureNotify, flirHandler) ;

                    capDevice.property = flirHandler->GetThermalProperty( cameraPtr ) ;
//                    printf("R               : %d\n", property.mR) ;
//                    printf("B               : %.6f\n", property.mB) ;
//                    printf("F               : %.6f\n", property.mF) ;
//                    printf("ObjectEmissivity: %.6f\n", property.mEmissivity) ;
//                    printf("ObjectDistance  : %.6f\n", property.mObjectDistance) ;
//                    printf("beta2           : %.6f\n", property.mBeta2) ;

                    flirHandler->startCapture( ) ;
                }
                else
                    delete flirHandler ;
            }
            else
                printf("cAppHandler::configFLIRHandler( %s != %s)\n", camera_it->first.c_str(), devURL.c_str()) ;
            camera_it++ ;
        }
		}
    return (int) mmFLIRHandler.size( ) ;
}

/****************************************************************
*
****************************************************************/
void cAppHandler::checkROILists(sCapDevice& capDevice, int img_width, int img_height)
{
//    for (sCaptureROI roi : capDevices.capDevices ) {
    for (int o_id = 0; o_id < (int) capDevice.roiLists.size(); o_id++ ) {
        sCaptureROI* roi = &capDevice.roiLists[o_id] ;
        roi->p1.x = roi->p2.x = roi->roiPts[0].x ;
        roi->p1.y = roi->p2.y = roi->roiPts[0].y ;
        for (int p_id = 0; p_id < (int)roi->roiPts.size(); p_id++) {
            if (roi->roiPts[p_id].x < 0)
                roi->roiPts[p_id].x = 0 ;
            if (roi->roiPts[p_id].x > img_width)
                roi->roiPts[p_id].x = img_width ;
            if (roi->roiPts[p_id].y > img_height)
                roi->roiPts[p_id].y = img_height ;
            if (roi->roiPts[p_id].y < 0)
                roi->roiPts[p_id].y = 0 ;
//-------------------------------------------------
            if (roi->roiPts[p_id].x < roi->p1.x)
                roi->p1.x = roi->roiPts[p_id].x ;
            if (roi->roiPts[p_id].y < roi->p1.y)
                roi->p1.y = roi->roiPts[p_id].y ;
            if (roi->roiPts[p_id].x > roi->p2.x)
                roi->p2.x = roi->roiPts[p_id].x ;
            if (roi->roiPts[p_id].y > roi->p2.y)
                roi->p2.y = roi->roiPts[p_id].y ;
        }
//      printf("P1.X:%3d P1Y:%3d, P2.X:%3d, P2.Y:%3d\n", roi->p1.x, roi->p1.y, roi->p2.x, roi->p2.y) ;
    }
}

/****************************************************************
*
****************************************************************/
void cAppHandler::captureNotify(void* trigger_data, void* user_data)
{
    cFLIRHandler* flirHandler = (cFLIRHandler* ) user_data ;
    sCBHandlerData* captureCBData = ( sCBHandlerData* ) trigger_data ;
    int cmd_id = captureCBData->mCMDId ;
    switch(cmd_id)
    {
      default :
//          printf("cAppHandler::captureNotify( %d ), %s\n", event_id, captureCBData->mMsg.c_str()) ;
        break ;
    }
    sCBHandlerData requestCmd ;
    requestCmd.mCMDId = cFLIRHandler::CAPHANDLER_RUNNING ;
    requestCmd.mMsg = flirHandler->getDevAddress() ;
    flirHandler->requestCMD( &requestCmd ) ;
//	  printf("cAppHandler::captureNotify( from flirHandler( %d: %s ) )....\n", captureCBData->mCMDId, captureCBData->mMsg.c_str( )) ;
}

//==============================================================
//  https://flir.custhelp.com/app/answers/detail/a_id/1021/~/temperature-linear-mode
//==============================================================
void cAppHandler::GoHandler( )
{
  static std::map<std::string, cFLIRHandler*>::iterator handler = mmFLIRHandler.begin() ;

    if (handler == mmFLIRHandler.end())
        handler = mmFLIRHandler.begin() ;
    std::string url = handler->first ;
    cFLIRHandler* flirHandler = handler->second ;
    if ( flirHandler ) {
//===============================================================
        cv::Mat srcFrame ;
		    ImagePtr pImage = flirHandler->getSrcImage( srcFrame ) ;
//---------------------------------------------------------
        if (pImage)  {
//================================================================
            uint16_t* pData = static_cast<uint16_t *>(pImage->GetData());
//===============================================
            sCapDevice capDevice = mmCapDevices[url] ;
            goEngine( pData, srcFrame, capDevice) ;
//------------------------------------------------------------------------
            showImageMat( (void *) &srcFrame, ("FLIR_Source@" + flirHandler->getDevAddress()).c_str()) ;
//=================================================================
//            cv::Mat equalizeFrame ;
//            cv::equalizeHist(srcFrame, equalizeFrame) ;
//            showImageMat( (void *) &equalizeFrame, ("FLIR_equalizeFrame@" + flirHandler->getDevAddress()).c_str()) ;
//===============================================================================
            bool IS_TO_TEMP = true ;
//            cv::Mat mono16IRFrame ;
//            mono16IRFrame.create(srcFrame.rows, srcFrame.cols, CV_16UC1);
//            converter_16_8::Instance().convertTo16bit(srcFrame, mono16IRFrame, IS_TO_TEMP) ;
//            showImageMat( (void *) &mono16IRFrame, ("FLIR_Mono16@" + flirHandler->getDevAddress()).c_str()) ;
//-----------------------------------------------------------------------
            cv::Mat falseC3Frame ;
            falseC3Frame.create(srcFrame.rows, srcFrame.cols, CV_8UC3);
            convertFalseColor16(srcFrame, falseC3Frame, mPalette, IS_TO_TEMP,
                              converter_16_8::Instance().getMin(),
                              converter_16_8::Instance().getMax());
            showImageMat( (void *) &falseC3Frame, ("FLIRFalseC3@" + flirHandler->getDevAddress()).c_str()) ;
//---------------------------------------------------------
            cv::Mat equalizeFrame ;
            cv::cvtColor( falseC3Frame, equalizeFrame, cv::COLOR_BGR2GRAY );
            cv::equalizeHist(equalizeFrame, equalizeFrame) ;
            showImageMat( (void *) &equalizeFrame, ("FLIR_equalizeFrame@" + flirHandler->getDevAddress()).c_str()) ;
//------------------------------------------------------------------------

            cv::Mat srcCFrame ;
//            cv::applyColorMap(srcFrame, srcCFrame, cv::COLORMAP_AUTUMN);
//            cv::applyColorMap(srcFrame, srcCFrame, cv::COLORMAP_BONE);
//            cv::applyColorMap(srcFrame, srcCFrame, cv::COLORMAP_WINTER);
//            cv::applyColorMap(srcFrame, srcCFrame, cv::COLORMAP_JET);
//            cv::applyColorMap(srcFrame, srcCFrame, cv::COLORMAP_RAINBOW);
//            cv::applyColorMap(srcFrame, srcCFrame, cv::COLORMAP_OCEAN);
//            cv::applyColorMap(srcFrame, srcCFrame, cv::COLORMAP_SUMMER);
//            cv::applyColorMap(srcFrame, srcCFrame, cv::COLORMAP_SPRING);
//            cv::applyColorMap(srcFrame, srcCFrame, cv::COLORMAP_COOL);
//            cv::applyColorMap(srcFrame, srcCFrame, cv::COLORMAP_HSV);
//            cv::applyColorMap(srcFrame, srcCFrame, cv::COLORMAP_PINK);
            cv::applyColorMap(equalizeFrame, srcCFrame, cv::COLORMAP_HOT);
            showImageMat( (void *) &srcCFrame, ("Color@" + flirHandler->getDevAddress()).c_str()) ;
//========================================================================
        }
//        std::this_thread::sleep_for(std::chrono::milliseconds(10));
//==============================================================
        handler++ ;
    }
}


/*****************************************************************************
*
******************************************************************************/
void cAppHandler::goEngine( uint16_t* pData, cv::Mat& frame, sCapDevice& capDevices)
{
    for (sCaptureROI roi : capDevices.roiLists )   {
        cv::Scalar color = thermalROIColors[roi.roiType] ;
        if ( roi.roiPts.size() > 1)  {
//-------------------------------------------------------------
            uint16_t  max, min ;
            max = min = pData[roi.p1.y * frame.cols + roi.p1.x] ;
            for (int row = roi.p1.y; row < roi.p2.y; row++) {
                for (int col = roi.p1.x; col < roi.p2.x; col++) {
                    int id = row * frame.cols + col ;
                    if (pData[id] > max)   max = pData[id] ;
                    if (pData[id] < min)   min = pData[id] ;
  //                  printf("%5d ", pData[id]) ;
                }
            }
//
//===========================================================================
//---   IRFormat == RADIOMETRIC ( 0 )
//---   image_Temp = (B / np.log(R / ((image_Radiance / Emiss / Tau) - K2) + F)) - 273.15
////            double image_minRadiance = (min - m_J0) / m_J1 ;
            double image_minRadiance = (min - 4031) / 70.724 ;
            double image_maxRadiance = (max - 4031) / 70.724 ;
            double TAtmC = 293.15 - 273.15 ;
            double H2O = 0.55 * exp(1.5587 + 0.06939 * TAtmC - 0.00027816 * TAtmC * TAtmC + 0.00000068455 * TAtmC * TAtmC * TAtmC) ;
            double Tau = 1.90 * exp(- sqrt(1.0) * (0.006569 + -0.002276 * sqrt(H2O))) + (1 - 1.9) * exp(-sqrt(1.0) * (0.012620 + -0.006670 * sqrt(H2O))) ;

            double r1 = ((1 - 0.950) / 0.950) * (15912 / (exp(1416.1 / 293.15) - 1.0)) ;
            double r2 = ((1 - Tau) / (0.950 * Tau)) * (15912 / (exp(1416.1 / 293.15) - 1.0)) ;
            double r3 = ((1 - 1.0) / (0.95 * Tau * 1.0)) * (15912 / (exp(1416.1 / 293.15) - 1.0)) ;
            double K2 = r1 + r2 + r3 ;
            double image_minTemp = (1416.1 / log(15912 / ((image_minRadiance / 0.95 / Tau) - K2) + 1.0)) - 273.15 ;
            double image_maxTemp = (1416.1 / log(15912 / ((image_maxRadiance / 0.95 / Tau) - K2) + 1.0)) - 273.15 ;

//----------------------------------------------------------------
//---   IRFormat == LINEAR_10MK ( 2 )
//            double image_maxTemp = (max * 0.01) - 273.15 ;
//            double image_minTemp = (min * 0.01) - 273.15 ;
//----------------------------------------------------------------
//---   IRFormat == LINEAR_100MK ( 1 )
//            double image_maxTemp = (max * 0.1) - 273.15 ;
//            double image_minTemp = (min * 0.1) - 273.15 ;
//===========================================================================
              cv::polylines( frame, roi.roiPts, true, color, 1, cv::LINE_4);
              cv::rectangle(frame, roi.p1, roi.p2, color, 2, cv::LINE_8) ;
              char minTempStr[16], maxTempStr[16] ;
              sprintf(minTempStr, "Min.: %.2f", image_minTemp) ;
              sprintf(maxTempStr, "Max: %.2f", image_maxTemp) ;
              cv::putText(frame, roi.roiName, cv::Point(roi.p1.x + 4, roi.p1.y + 12),
                          cv::FONT_HERSHEY_SIMPLEX, 0.4, color) ;
              cv::putText(frame, maxTempStr, cv::Point(roi.p1.x + 4, roi.p1.y + 28),
                          cv::FONT_HERSHEY_SIMPLEX, 0.6, color, 2) ;
              cv::putText(frame, minTempStr, cv::Point(roi.p1.x + 4, roi.p1.y + 48),
                          cv::FONT_HERSHEY_SIMPLEX, 0.6, color, 2) ;

//      cv::fillPoly( frame, roi.roiPts, cv::Scalar(0, 255, 255), cv::LINE_8);
//              printf("ROI %s(%d), MinTemp.: %.3f(%5d), Max.Temp: %.3f(%5d)\n",
//                      roi.roiName.c_str(), roi.roiType,
//                      image_minTemp, min, image_maxTemp, max);
//----------------------------------------------------------------
        }
    }
}

//----------------------------------------------------------------------------
// exitHandler
//----------------------------------------------------------------------------
void cAppHandler::ExitHandler()
{
//    if (pActiveCameraPtr)  {
//        pActiveCameraPtr->EndAcquisition();
//        pActiveCameraPtr->DeInit();
//        pActiveCameraPtr = nullptr;	// without this, spinnaker complains
//    }
    mCameraList.Clear();
    mSpinSystem->ReleaseInstance();
		cv::destroyAllWindows();
}

//----------------------------------------------------------------------------
// exitHandler
//----------------------------------------------------------------------------
void cAppHandler::showImageMat(void* mat_ptr, const char *name)
{
    try {
        if (mat_ptr == NULL) return;
        cv::Mat &mat = *(cv::Mat *)mat_ptr;
//        cv::namedWindow(name, cv::WINDOW_NORMAL);
////        cv::namedWindow(name, cv::WINDOW_AUTOSIZE);
        cv::imshow(name, mat);
    }
    catch (...) {
        std::cerr << "OpenCV exception: show_image_mat \n";
    }
}
