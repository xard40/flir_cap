#include <fstream>
#include <sstream>
#include <iostream>
//---------------------------------------------
#include "appHandler.h"

using namespace std ;
//***********************************************************
//
//************************************************************
namespace YAML {
    template<>
    struct convert<sCapROILists> {
        static Node encode(const sCapROILists &capture) {
            Node node;
            node.push_back(capture.name);
            node.push_back(capture.url);
            printf("devName: %s, URL: %s\n", capture.name.c_str(), capture.url.c_str()) ;
            return node;
        }
        static bool decode(const Node &node, sCapROILists &capture) {
            capture.name = node["devName"].as<std::string>();
            capture.url = node["devURL"].as<std::string>();
            YAML::Node roiLists = node["ROILists"];
            sCapFrameROI  frame_roi ;
            cv::Point cv_pt ;
            int area_id = 0 ;
            capture.regionLists.clear() ;
            for (YAML::const_iterator area_it = roiLists.begin(); area_it != roiLists.end(); ++area_it) {
                YAML::Node roiLists_map = area_it->as<YAML::Node>();
                int list_id = 0 ;
                frame_roi.roiPts.clear() ;
                for (YAML::const_iterator pt_it = roiLists_map.begin(); pt_it != roiLists_map.end(); ++pt_it) {
                    YAML::Node::const_iterator map_it = pt_it->begin();
                    std::string key = map_it->first.as<std::string>();
                    int value = map_it->second.as<int>() ;
                    if (list_id == 0)  {
                        frame_roi.regionName = key ;
                        frame_roi.regionType = value ;
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
                capture.regionLists.push_back( frame_roi ) ;
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
    printf("Number of FLIR cameras detected: %d\n\n", numCameras) ;
    if (numCameras == 0) {
        ExitHandler() ;
        printf("no FLIR camera detected !\n") ;
        return  0 ;
    }
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

/************************************************************************************
*
************************************************************************************/
int cAppHandler::readCaptureConfig( const YAML::Node &capConfig )
{
  int totalDevices = 0 ;

    if (capConfig["captureDevices"]) {
        vmCapROILists = capConfig["captureDevices"].as<std::vector<sCapROILists>>();
        for (std::vector<sCapROILists>::iterator it = vmCapROILists.begin(); it != vmCapROILists.end(); ++it) {
//            std::vector<sCapFrameROI>& region_lists = it->regionLists ;
//            int r_id = 0 ;
//            for (sCapFrameROI frame_region : region_lists )   {
//                std::vector<cv::Point>  roi_pts = frame_region.roiPts ;
//                printf("%s: %s, Reg.%d:%s(%d): Sig:%s(%d), ",
//                    it->name.c_str(), it->url.c_str(),
//                    r_id, frame_region.regionName.c_str(), frame_region.regionType,
//                          frame_region.signalName.c_str(), frame_region.actionType) ;
//                int pt_id = 0 ;
//                for (cv::Point pt : roi_pts)  {
//                    printf("P%d.X:%3d Y:%3d, ", pt_id, pt.x, pt.y) ;
//                    pt_id += 1 ;
//                }
//                r_id += 1 ;
//                printf("\n") ;
//            }
            totalDevices += 1 ;
        }
    }
    return totalDevices ;
}

//============================================================================
//    std::vector<CameraPtr>      vpCameraPtr ;
//    std::vector<sCapROILists>   vmCapROILists ;
//  --> generate map members of mmCapROILists && mmFLIRHandler with the same URL key
//============================================================================
int cAppHandler::configFLIRHandler(  )
{
  cv::Mat firstFrame, thumbnail ;
    for (sCapROILists capROIList : vmCapROILists)   {
        std::string devURL = capROIList.url ;
        std::string devName = capROIList.name ;
//---------------------------------------------------------
        std::map<std::string, CameraPtr>::iterator it = mmCameraPtr.begin() ;
        while (it != mmCameraPtr.end())   {
            if ( it->first == devURL) {
                mmCapROILists.insert(std::pair<std::string, sCapROILists>(devURL, capROIList)) ;
                cFLIRHandler* flirHandler = new cFLIRHandler(devName, devURL) ;
                mmFLIRHandler.insert(std::pair<std::string, cFLIRHandler*>(devURL,flirHandler)) ;

                mCaptureCBId = flirHandler->regCapNotify(this, &cAppHandler::captureNotify, flirHandler) ;

                CameraPtr cameraPtr = it->second ;
                firstFrame = flirHandler->initFLIRHandler( cameraPtr, thumbnail ) ;
                sThermalProperty property = flirHandler->GetThermalProperty( cameraPtr ) ;
                printf("R           : %d\n", property.mR) ;
                printf("B           : %.6f\n", property.mB) ;
                printf("F           : %.6f\n", property.mF) ;
                printf("O           : %.6f\n", property.mO) ;
                printf("Emissivity  : %.6f\n", property.mEmissivity) ;
                printf("ObjectDistance: %.6f\n", property.mObjectDistance) ;
                printf("beta2       : %.6f\n", property.mBeta2) ;

//                cv::imshow(devName, firstFrame ) ;

                flirHandler->startCapture( ) ;
                break ;
            }
            else
                printf("cAppHandler::configFLIRHandler( %s != %s)\n", it->first.c_str(), devURL.c_str()) ;
            it++ ;
        }
		}
    return (int) mmFLIRHandler.size( ) ;
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
//
//==============================================================
void cAppHandler::GoHandler( )
{
  static std::map<std::string, cFLIRHandler*>::iterator it = mmFLIRHandler.begin() ;

    if (it == mmFLIRHandler.end())
        it = mmFLIRHandler.begin() ;
    cFLIRHandler* flirHandler = it->second ;
    if ( flirHandler ) {
//===============================================================
        cv::Mat srcFrame ;
		    ImagePtr pImage = flirHandler->getSrcImage( srcFrame ) ;
//---------------------------------------------------------
        if (pImage)  {
//================================================================
//         uchar* pData = static_cast<uchar *>(pImage->GetData());
      /*   ImageUtilityHeatmap::SetHeatmapColorGradient(
                SPINNAKER_HEATMAP_COLOR_BLACK, SPINNAKER_HEATMAP_COLOR_WHITE) ;
            ImageUtilityHeatmap::SetHeatmapRange(0, 100);
            const ImagePtr heatmapImage = ImageUtilityHeatmap::CreateHeatmap(pImage) ;
            const auto w = heatmapImage->GetWidth();
            const auto h = heatmapImage->GetHeight();
            const auto s = heatmapImage->GetStride() ;
            uchar* pHeatData = static_cast<uchar *>(heatmapImage->GetData());
            cv::Mat heatFrame = cv::Mat((int)h, (int)w, CV_8UC1, pHeatData, s) ;
            showImageMat( (void *) &heatFrame, "HeatFrame") ;
      */
//          cv::Mat srcFrame = cv::Mat((int)(height + YPadding), (int)(width + XPadding),
//                                      CV_8UC1, pData, stride) ;
//---------------------------------------------------------------------
            // Print the first 16 bytes of the buffer in hex
//            uchar max, min ;
            sCapFrameROI roi ;
            roi.p1.x = 96,  roi.p1.y = 128 ;
            roi.p2.x = 192, roi.p2.y = 200 ;
//            max = min = pData[roi.p1.y * width + roi.p1.x] ;
//            for (int row = roi.p1.y; row < roi.p2.y; row++) {
//                for (int col = roi.p1.x; col < roi.p2.x; col++) {
//                    int id = row * width + col ;
//                    if (pData[id] > max)   max = pData[id] ;
//                    if (pData[id] < min)   min = pData[id] ;
////                    printf("%3d ", pData[id]) ;
//                }
////                printf("\n") ;
//            }
////            printf("Min.: %3d, Max.: %3d\n", min, max);
            cv::rectangle(srcFrame, roi.p1, roi.p2, cv::Scalar(255,   0, 255), 1, cv::LINE_8) ;
//------------------------------------------------------------------------
            showImageMat( (void *) &srcFrame, flirHandler->getDevAddress().c_str()) ;
//===============================================================================
            bool IS_TO_TEMP = true ;
            cv::Mat mono16IRFrame ;
            mono16IRFrame.create(srcFrame.rows, srcFrame.cols, CV_16UC1);
            // converter_16_8::Instance().convert_to8bit(srcFrame, mono16IRFrame, IS_TO_TEMP);
            converter_16_8::Instance().convertTo16bit(srcFrame, mono16IRFrame, IS_TO_TEMP) ;
            showImageMat( (void *) &mono16IRFrame, "Mono16IRFrame") ;
//-----------------------------------------------------------------------
            cv::Mat mono16Frame ;
            convertFalseColor16(srcFrame, mono16Frame, mPalette, IS_TO_TEMP,
                              converter_16_8::Instance().getMin(),
                              converter_16_8::Instance().getMax());
//            convertFalseColor(srcFrame, mono16Frame, mPalette, IS_TO_TEMP,
//                              converter_16_8::Instance().getMin(),
//                              converter_16_8::Instance().getMax());
//            convertFalseColor(srcFrame, mono16Frame, mPalettte, IS_TO_TEMP, 0.0, 300.0) ;
            showImageMat( (void *) &mono16Frame, ("FLIR@" + flirHandler->getDevAddress()).c_str()) ;
//            printf("Min.: %3d, Max.: %3d, converter_16_8::Instance() Min.: %.2f, Max.: %.2f\n",
//                    min, max, converter_16_8::Instance().getMin(), converter_16_8::Instance().getMax());
//========================================================================
//        std::this_thread::sleep_for(std::chrono::milliseconds(40));
        }
//==============================================================
        it++ ;
    }
}

//====================================================
/**
@brief parallel capture function
*/
void parallel_capture_(Spinnaker::CameraPtr & pCam, cv::Mat & img) {
	// capture
	Spinnaker::ImagePtr image = pCam->GetNextImage();
	void* data = image->GetData();
	std::memcpy(img.data, data, sizeof(unsigned char) * img.rows * img.cols);
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
