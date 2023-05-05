#include <sys/time.h>  // int gettimeofday(struct timeval *restrict tv, struct timezone *restrict tz);
#include "flirHandler.h"
using namespace std ;

#define CAM_TYPE_A3XX 0
#define CAM_TYPE_A6XX 1
#define CAM_TYPE_ATAU 2
#define CAM_TYPE_T1K  3
#define CAM_TYPE_ACAM 4 // Bellatrix
//**************************************************************
// cFLIRHandler
//**************************************************************
cFLIRHandler::cFLIRHandler( std::string& devName, std::string& devAddress )
{
    mDevName = devName ;
    mDevAddress = devAddress ;
}

/**********************************************************
*
***********************************************************/
cFLIRHandler::~cFLIRHandler()
{
    printf("cFLIRHandler::~cFLIRHandler() destroy threads ....\n") ;
    stopCapture( ) ;
}

//----------------------------------------------------------------------------
// createCapThread
//----------------------------------------------------------------------------
cv::Mat* cFLIRHandler::initFLIRHandler( CameraPtr cameraPtr, cv::Mat& thumbnail )
{
    printf("cFLIRHandler::initFLIRHandler(%s)\n", mDevAddress.c_str()) ;
    pCameraPtr = cameraPtr ;
//    INodeMap & nodeMapTLDevice = pCameraPtr->GetTLDeviceNodeMap();
//    printDeviceInfo(nodeMapTLDevice);
//------------------------------------------------
    pCameraPtr->Init( ) ;
    INodeMap& nodeMap = pCameraPtr->GetNodeMap();
    configCamera( nodeMap ) ;

    pCameraPtr->BeginAcquisition();
    mImageProcessor.SetColorProcessing(SPINNAKER_COLOR_PROCESSING_ALGORITHM_HQ_LINEAR) ;
//================================================================
    this->pImagePtrQueue = std::make_unique<cSharedQueue<ImagePtr>>() ;
//--------------------------------------------------------------------
    ImagePtr pImage = acquireImage() ;
    mOffsetX = pImage->GetXPadding();
    mOffsetY = pImage->GetYPadding();
    mImageWidth = pImage->GetWidth();
    mImageHeight = pImage->GetHeight();
    mImageStride = pImage->GetStride();
    mPayloadType = pImage->GetPayloadType() ;
//--------------------------------------------------------
    mPixelFormatEnums = pImage->GetPixelFormat() ;
    size_t bpp = pImage->GetBitsPerPixel() ;
    ImagePtr convertedImage = ConvertImage( pImage ) ;
    uchar* pData = static_cast<uchar *>(convertedImage->GetData());
    convertedImage->Save("flirImage.png") ;
    if (mPixelFormatEnums == PixelFormat_Mono8)
        printf("initFLIRHandler( Mono8 ), Image W.: %d, H.: %d, Stride: %d, payLoadType: %d\n\n",
                mImageWidth, mImageHeight, mImageStride, mPayloadType) ;
    else if (mPixelFormatEnums == PixelFormat_Mono16)
        printf("initFLIRHandler( Mono16 ), Image W.: %d, H.: %d, Stride: %d, payLoadType: %d, bpp: %ld\n\n",
                mImageWidth, mImageHeight, mImageStride, mPayloadType, bpp) ;
    else
        printf("initFLIRHandler( Pix.Format: %d ), Image W.: %d, H.: %d, Stride: %d, payLoadType: %d\n\n",
                mPixelFormatEnums, mImageWidth, mImageHeight, mImageStride, mPayloadType) ;
//-----------------------------------------------------------------
    mFirstFrame = cv::Mat(mImageHeight + mOffsetY, mImageWidth + mOffsetX, CV_8UC1, pData, mImageStride );
    cv::Size thumbnail_size = cv::Size(0, 0);
    thumbnail_size.width = 128 ;
    thumbnail_size.height	= thumbnail_size.width * (mFirstFrame.rows / (float) mFirstFrame.cols) ;
    cv::resize(mFirstFrame, mThumbnail, thumbnail_size, 0, 0, cv::INTER_AREA) ;
    thumbnail = mThumbnail ;
    return &mFirstFrame ;
}

/**********************************************************
*
***********************************************************/
void cFLIRHandler::startCapture( )
{
    printf("\ncFLIRHandler::startCapture( %s ), create threads for getting camera image ...\n", mDevAddress.c_str()) ;
		mCBCapStarted = true ;
    pCBCapThread = new std::thread(&cFLIRHandler::cbCaptureImage, this, mDevName + "cbRead") ;
    mThreadMap[mDevName + "cbRead"] = pCBCapThread->native_handle( ) ;
    pCBCapThread->detach() ;
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
//------------------------------------------
    sCBHandlerData requestCmd ;
    requestCmd.mCMDId = cFLIRHandler::CAPHANDLER_RUNNING ;
    requestCmd.mMsg = mDevAddress + " Start..." ;
    requestCMD( &requestCmd ) ;

//    pCapThread = new std::thread(&cFLIRHandler::captureFrame, this, mDevName + "read") ;
//    mThreadMap[mDevName + "read"] = pCapThread->native_handle( ) ;
//    pCapThread->detach() ;

//    pPacketThread = new std::thread(&cFLIRHandler::generateFrame, this, mDevName + "packet") ;
//    mThreadMap[mDevName + "packet"] = pPacketThread->native_handle( ) ;
//    pPacketThread->detach() ;
}

//----------------------------------------------------------------------------
//   cbCaptureFrame
//----------------------------------------------------------------------------
void cFLIRHandler::cbCaptureImage( std::string name )
{
		printf("cFLIRHandler::cbCaptureFrame( thread started ), waiting for CMD request , ..... \n") ;
		while ( mCBCapStarted )		{
			  std::unique_lock<std::mutex> lock(mMutex) ;
			  while (qmCmdQueue.empty( ))
				    mReplyCondition.wait( lock ) ;
			  if (qmCmdQueue.empty( ))
				    continue;
			  mReplyCmd = qmCmdQueue.front( ) ;
			  qmCmdQueue.pop( ) ;
			  lock.unlock( ) ;
			  handlerReply( &mReplyCmd ) ;
		}
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
void cFLIRHandler::handlerReply( sCBHandlerData* replyCmd )
{
//		mCBCapture.triggerCallback( replyCmd ) ;
		switch (replyCmd->mCMDId)
		{
			  case CAPHANDLER_RUNNING :  {
        		mCBCapture.triggerCallback( replyCmd ) ;
            ImagePtr pImage = acquireImage() ;

            if (this->pImagePtrQueue->size() == 0)
                this->pImagePtrQueue->push(std::move(pImage));
//            this->pImagePtrQueue->push(pImage);
//====================================================
//            if ( qmImagePtr.size( ) > 0)
//                qmImagePtr.clear( ) ;
//            qmImagePtr.push( pImage ) ;
//==================================================
			    break ;
       }
			 case CAPHANDLER_STOP :  {
					 printf("cVideoHandler::handlerEngineEntry( ), CMD_CAPTURE_STOP, cmdQueueSize: %ld\n", qmCmdQueue.size()) ;
			   break ;
			}
			default :
			   break ;
		}
}

//----------------------------------------------------------------------------
// // Add user command to queue and notify worker thread
//----------------------------------------------------------------------------
void cFLIRHandler::requestCMD( sCBHandlerData* handlerCmd )
{
		std::unique_lock<std::mutex> lock(mMutex) ;
		if ( qmCmdQueue.size( ) == 0)
			  qmCmdQueue.push( *handlerCmd ) ;
		mReplyCondition.notify_one( ) ;
		lock.unlock( ) ;
}

/**********************************************************
*
***********************************************************/
void cFLIRHandler::captureFrame( std::string name)
{
    printf("cFLIRHandler::captureFrame( )  ==> thread start ...\n") ;
    while( true ) {
      ImagePtr pImage = acquireImage() ;
      this->pImagePtrQueue->push(std::move(pImage));

      this->mBufferCondition.notify_one();
//----------------------------------------------------
//      printf("cFLIRHandler::captureFrame( %d RingSize: %ld )\n", cnt++, this->pAVFrameRing->size()) ;
    }
}

/**********************************************************
*   1: wait until every (valid) buffer has at least one frame stored
*   2: forever loop: continuously generate new synchronized frame packets and put them in the output buffer
*      wait until all of the (valid) buffers has an element
***********************************************************/
void cFLIRHandler::generateFrame(std::string name)
{
  static int cnt ;
    std::cout << "cFLIRHandler::generateFrame( ) ==> thread start, Waiting for buffers to fill up... ";
    while( true ) {
      if (this->pImagePtrQueue->size() >= 1)
        break ;
    }
    std::cout << "[OK]" << std::endl;
    while( true ) {
      std::unique_lock<std::mutex>  bufferLock( this->mBufferMutex) ;
      this->mBufferCondition.wait(bufferLock, [this]{ return (this->pImagePtrQueue->size() > 0);}) ;
      bufferLock.unlock() ;
//======================================================
      sCBHandlerData requestCmd ;
      requestCmd.mCMDId = CAPHANDLER_RUNNING ;
      requestCMD( &requestCmd ) ;
//======================================================
      printf("cFLIRHandler::generateFrame( %d, RingSize: )\n", cnt++) ;
    }
}

/**********************************************************
*
***********************************************************/
//ImagePtr cFLIRHandler::getImagePtr( ImagePtr& convertedImage )
//{
//  ImagePtr imagePtr ;
//    this->pImagePtrQueue->pop(imagePtr) ;
//    pSrcImage = std::move(imagePtr) ;
//    convertedImage = ConvertImage( pSrcImage ) ;
//    return pSrcImage ;
//}

/**********************************************************
*
***********************************************************/
ImagePtr cFLIRHandler::getSrcImage( cv::Mat& matFrame )
{
  ImagePtr imagePtr ;
    this->pImagePtrQueue->pop(imagePtr) ;
    pSrcImage = std::move(imagePtr) ;
//    ImagePtr convertedImage = pSrcImage ;
    ImagePtr convertedImage = ConvertImage( pSrcImage ) ;
//--------------------------------------------------------
    uchar* pData = static_cast<uchar *>(convertedImage->GetData());
//-----------------------------------------------------
    mSrcFrame = cv::Mat(mImageHeight + mOffsetY, mImageWidth + mOffsetX, CV_16UC1, pData, mImageStride );
//-----------------------------------------------------------------------
    struct timeval captureTime ;
    gettimeofday(&captureTime, NULL) ;
    struct tm* time_info = localtime(&captureTime.tv_sec ) ;
    char timeString[24] = "" ;
    strftime(timeString, 24, "%H:%M:%S", time_info ) ;
    cv::putText(mSrcFrame, timeString, cv::Point(2, 14),
//              cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 0, 0)) ;
                cv::FONT_HERSHEY_COMPLEX, 0.5, cv::Scalar(0, 0, 0)) ;
//-------------------------------------------------------------
    matFrame = mSrcFrame ;
    return pSrcImage ;
}

/**********************************************************
*
***********************************************************/
ImagePtr cFLIRHandler::acquireImage()
{
    ImagePtr pImage = pCameraPtr->GetNextImage( );
//    uint64_t image_timestamp = pImage->GetTimeStamp();
    if (pImage->IsIncomplete()) {
        const char* msg = Spinnaker::Image::GetImageStatusDescription( pImage->GetImageStatus()) ;
//        static int cnt ;
//        printf("acquireImage() IsIncomplete(%d, Status: %d(%s))...\n",
//                cnt++, pImage->GetImageStatus(), msg) ;
    }
    else
        pAcquiredImage = pImage ;

    pImage->Release() ;
    return pAcquiredImage ;
}

/****************************************************************
*
****************************************************************/
ImagePtr cFLIRHandler::ConvertImage( ImagePtr pImage )
{
    switch ( mPixelFormatEnums ) {
      case PixelFormat_Mono8:
          pConvertedImage = mImageProcessor.Convert(pImage, PixelFormat_Mono8);
        break;
      case PixelFormat_Mono16:
          pConvertedImage = mImageProcessor.Convert(pImage, PixelFormat_Mono16);
        break;
      case PixelFormat_RGB8:
          pConvertedImage = mImageProcessor.Convert(pImage, PixelFormat_RGB8);
        break ;
      case PixelFormat_RGB16:
          pConvertedImage = mImageProcessor.Convert(pImage, PixelFormat_RGB16);
        break ;
      case PixelFormat_Raw16:
          pConvertedImage = mImageProcessor.Convert(pImage, PixelFormat_Raw16);
        break ;

        /*      ImageUtilityHeatmap::SetHeatmapColorGradient(
                  SPINNAKER_HEATMAP_COLOR_BLACK, SPINNAKER_HEATMAP_COLOR_WHITE) ;
              ImageUtilityHeatmap::SetHeatmapRange(0, 100);
              const ImagePtr heatmapImage = ImageUtilityHeatmap::CreateHeatmap(pRawImage) ;
        */
      default:
          pConvertedImage = pImage ;
          throw std::runtime_error("The pixel format is not supported!");
        break ;
    }
    return pConvertedImage ;
}

/****************************************************************
*
****************************************************************/
int cFLIRHandler::printDeviceInfo(INodeMap& nodeMap)
{
    int result = 0;
    std::cout << std::endl << "*** " << mDevName << "@" << mDevAddress
                           << " DEVICE INFORMATION ***"
                           << std::endl << std::endl;
    try  {
        FeatureList_t features;
        CCategoryPtr category = nodeMap.GetNode("DeviceInformation");
        if (IsAvailable(category) && IsReadable(category))  {
            category->GetFeatures(features);
            FeatureList_t::const_iterator it;
            for (it = features.begin(); it != features.end(); ++it)  {
                CNodePtr pfeatureNode = *it;
                std::cout << pfeatureNode->GetName() << " : ";
                CValuePtr pValue = (CValuePtr)pfeatureNode;
                std::cout << (IsReadable(pValue) ? pValue->ToString() : "Node not readable");
                std::cout << std::endl;
            }
        }
        else
            std::cout << "Device control information not available." << std::endl;
    }
    catch (Spinnaker::Exception& e)  {
        std::cout << "Error: " << e.what() << std::endl;
        result = -1;
    }
    return result;
}

/****************************************************************
*
****************************************************************/
int cFLIRHandler::configCamera(INodeMap& nodeMap)
{
  int result = 0;

    printf("\n*** configCamera() : CONFIGURE %s Camera Setting ***\n\n", mDevAddress.c_str()) ;
    try   {
//        CStringPtr pStringNode = (CStringPtr) nodeMap.GetNode("DeviceSerialNumber");
//        cout << "Serial number: " << pStringNode->GetValue() << endl;
//        CEnumerationPtr ptrPixelFormat = pCameraPtr->PixelFormat.GetNode();
        CEnumerationPtr ptrPixelFormat = nodeMap.GetNode("PixelFormat");
        if (IsAvailable(ptrPixelFormat) && IsWritable(ptrPixelFormat))  {
            int64_t pixelFormat = ptrPixelFormat->GetIntValue();
//======================================================================
            CEnumEntryPtr ptrPixelFormatMono16 = ptrPixelFormat->GetEntryByName("Mono16");
            if (!IsAvailable(ptrPixelFormatMono16) || !IsReadable(ptrPixelFormatMono16))  {
                std::cout << "Unable to set pixel format to Mono16 ..." << endl << endl;
            }
            ptrPixelFormat->SetIntValue(ptrPixelFormatMono16->GetValue());
            printf("Pixel format Mono16 (0x%lX) R/W enabled\n", /*ptrPixelFormatMono16->GetSymbolic().c_str(),*/ pixelFormat);
//---------------------------------------------------------------
//            CEnumEntryPtr ptrPixelFormatMono8 = ptrPixelFormat->GetEntryByName("Mono8");
//            if (!IsAvailable(ptrPixelFormatMono8) || !IsReadable(ptrPixelFormatMono8))  {
//                std::cout << "Unable to set pixel format to Mono8. Aborting..." << endl << endl;
//            }
//            ptrPixelFormat->SetIntValue(ptrPixelFormatMono8->GetValue());
//            printf("Pixel format Mono8 (0x%lX) R/W enabled \n", /*ptrPixelFormatMono8->GetSymbolic().c_str(),*/ pixelFormat);
//=============================================================================================
        }
        else
            std::cout << "Pixel format not available..." << std::endl;
//-------------------------------------------------------------
// SetAcquisitionFrameRate(INodeMap& nodeMap)  in examples folder Planar.cpp
//        CBooleanPtr ptrAcquisitionFrameRateEnable = nodeMap.GetNode("AcquisitionFrameRateEnable");
//        if ( IsWritable(ptrAcquisitionFrameRateEnable))   {
//            ptrAcquisitionFrameRateEnable->SetValue(true);
//        }
//        else
//            std::cout << "Unable to set Acquisition Frame Rate Enable to true (enum retrieval)" << std::endl << std::endl;

//        Spinnaker::GenApi::CFloatPtr ptrAcquisitionFrameRate = nodeMap.GetNode("AcquisitionFrameRate");
//        if (!IsReadable(ptrAcquisitionFrameRate) || !IsWritable(ptrAcquisitionFrameRate))  {
//            cout << "Unable to set Acquisition Frame Rate" << endl << endl;
//            return -1;
//        }
//        float frameRate = static_cast<float>(ptrAcquisitionFrameRate->GetValue());
//        const double frameRate = 10.0;
//        ptrAcquisitionFrameRate->SetValue(frameRate);
//        std::cout << "Set Acquisition Frame Rate to  " << frameRate << std::endl;
//-------------------------------------------------------------
        //
        // Apply minimum to offset X
        //
        // *** NOTES ***
        // Numeric nodes have both a minimum and maximum. A minimum is retrieved
        // with the method GetMin(). Sometimes it can be important to check
        // minimums to ensure that your desired value is within range.
        //
        CIntegerPtr ptrOffsetX = nodeMap.GetNode("OffsetX");
        if (IsAvailable(ptrOffsetX) && IsWritable(ptrOffsetX))  {
            mOffsetX = ptrOffsetX->GetMin() ;
            ptrOffsetX->SetValue(mOffsetX);
            std::cout << "Offset X set to: " << mOffsetX << std::endl;
        }
        else
            std::cout << "Offset X not available..." << std::endl;
        //
        // Apply minimum to offset Y
        //
        // *** NOTES ***
        // It is often desirable to check the increment as well. The increment
        // is a number of which a desired value must be a multiple of. Certain
        // nodes, such as those corresponding to offsets X and Y, have an
        // increment of 1, which basically means that any value within range
        // is appropriate. The increment is retrieved with the method GetInc().
        //
        CIntegerPtr ptrOffsetY = nodeMap.GetNode("OffsetY");
        if (IsAvailable(ptrOffsetY) && IsWritable(ptrOffsetY))   {
            mOffsetY = ptrOffsetY->GetMin() ;
            ptrOffsetY->SetValue(mOffsetY);
            cout << "Offset Y set to: " << mOffsetY << endl;
        }
        else
            cout << "Offset Y not available..." << endl;
        //
        // Set maximum width
        //
        // *** NOTES ***
        // Other nodes, such as those corresponding to image width and height,
        // might have an increment other than 1. In these cases, it can be
        // important to check that the desired value is a multiple of the
        // increment. However, as these values are being set to the maximum,
        // there is no reason to check against the increment.
        //
        CIntegerPtr ptrWidth = nodeMap.GetNode("Width");
        if (IsAvailable(ptrWidth) && IsWritable(ptrWidth))  {
            mImageWidth = ptrWidth->GetMax();
            ptrWidth->SetValue(mImageWidth);
            cout << "Width    set to: " << mImageWidth << endl;
        }
        else
            cout << "Width not available..." << endl;
        //
        // Set maximum height
        //
        // *** NOTES ***
        // A maximum is retrieved with the method GetMax(). A node's minimum and
        // maximum should always be a multiple of its increment.
        //
        CIntegerPtr ptrHeight = nodeMap.GetNode("Height");
        if (IsAvailable(ptrHeight) && IsWritable(ptrHeight))  {
            mImageHeight = ptrHeight->GetMax();
            ptrHeight->SetValue(mImageHeight);
            std::cout << "Height   set to: " << mImageHeight << std::endl;
        }
        else
            std::cout << "Height not available..." << std::endl << std::endl;
//----------------------------------------------------------------------
        CEnumerationPtr ptrAcquisitionMode = nodeMap.GetNode("AcquisitionMode");
        if (IsAvailable(ptrAcquisitionMode) && IsWritable(ptrAcquisitionMode)) {
            const int64_t acquisitionMode = ptrAcquisitionMode->GetIntValue();
            std::cout << "AcquisitionMode: " << acquisitionMode << std::endl;
//            ptrAcquisitionMode->SetIntValue(acquisitionMode);
//            std::cout << "AcquisitionMode set to: "  << std::endl;
        }
        else
            std::cout << "Unable to set continuous acquisition mode as ptr is not writable ..." << endl << endl;
      // Retrieve entry node from enumeration node
        CEnumEntryPtr ptrAcquisitionModeContinuous = ptrAcquisitionMode->GetEntryByName("Continuous");
        if (IsAvailable(ptrAcquisitionModeContinuous) && IsReadable(ptrAcquisitionModeContinuous))	{
            const int64_t acquisitionModeContinuous = ptrAcquisitionModeContinuous->GetValue();
//          ptrAcquisitionMode->SetIntValue(acquisitionModeContinuous);
            std::cout << "Continuous Mode: " << acquisitionModeContinuous << std::endl;
        }
        else
            std::cout << "Unable to set continuous acquisition mode as entry is not readable..." << endl << endl;

        CStringPtr pStringModelName = (CStringPtr) nodeMap.GetNode("DeviceModelName");
        std::cout << "DeviceModelName: " << pStringModelName->GetValue() << std::endl;
//---------------------------------------------------------------
//        pStringNode = (CStringPtr) nodeMap.GetNode("DeviceSerialNumber");
//        cout << "Serial number: " << pStringNode->GetValue() << endl;
//        CFloatPtr pNode = (CFloatPtr) nodeMap.GetNode("DeviceTemperature");
//  	    printf("device Temperature = %f\n", pNode->GetValue());
//------------------------------------------------------------
    }
    catch (Spinnaker::Exception& e)  {
        cout << "Error: " << e.what() << endl;
        result = -1;
    }
    std::cout << "\n*** end of CONFIGURE CUSTOM Camera SETTINGS ***" << std::endl << std::endl ;
    return result;
}

//========================================================
//
//========================================================
void cFLIRHandler::stopCapture( )
{
    flirHandlerThreadMap::const_iterator itCBCapture = mThreadMap.find(mDevName + "cbRead") ;
    if (itCBCapture != mThreadMap.end()) {
        pthread_cancel(itCBCapture->second);
        mThreadMap.erase(mDevName + "cbRead");
        std::cout << "Thread " << mDevName + "cbRead" << " killed:" << std::endl << std::endl;
    }

//    flirHandlerThreadMap::const_iterator itRead = mThreadMap.find(mDevName + "read") ;
//    if (itRead != mThreadMap.end()) {
//      pthread_cancel(itRead->second);
//      mThreadMap.erase(mDevName + "read");
//      std::cout << "Thread " << mDevName + "read" << " killed:" << std::endl << std::endl;
//    }

//    flirHandlerThreadMap::const_iterator itPacket = mThreadMap.find(mDevName + "packet") ;
//    if (itPacket != mThreadMap.end()) {
//      pthread_cancel(itPacket->second);
//      mThreadMap.erase(mDevName + "packet");
//      std::cout << "Thread " << mDevName + "packet" << " killed:" << std::endl << std::endl;
//    }

    pCameraPtr->EndAcquisition() ;
		printf("cCapDevice::stopCapture( %s )\n", mDevAddress.c_str( )) ;
}

//----------------------------------------------------------------------------
// exitHandler
//----------------------------------------------------------------------------
void cFLIRHandler::exitHandler()
{
    stopCapture( ) ;
}

// =============================================================================
void cFLIRHandler::Connect( CameraPtr pCamera )
{
	  if (pCamera == 0)
		    return ;
    printf("\n******  cFLIRHandler::Connect( %s )  Device Info.: ************\n\n", mDevAddress.c_str()) ;
// Device connection, packet size negotiation and stream opening
// Connect device
	  Spinnaker::GenApi::CStringPtr ptrStringManufacturer = pCamera->DeviceVendorName.GetNode();
    std::string lManufacturerStr = std::string(ptrStringManufacturer->GetValue().c_str());
    std::cout << "DeviceVendorName: "  << lManufacturerStr << std::endl ;

	  Spinnaker::GenApi::CStringPtr ptrStringModel = pCamera->DeviceModelName.GetNode();
    std::string lModelStr = std::string(ptrStringModel->GetValue().c_str());
    std::cout << "Device ModelName: "  << lModelStr << std::endl ;

	  Spinnaker::GenApi::CStringPtr ptrStringName = pCamera->DeviceUserID.GetNode();
    std::string lNameStr = std::string(ptrStringName->GetValue().c_str());
    std::cout << "Device    UserID: "  << lNameStr << std::endl ;

    Spinnaker::GenApi::CEnumerationPtr ptrIRFormat = pCamera->GetNodeMap().GetNode("IRFormat");
    int64_t lValue = ptrIRFormat->GetIntValue();
    std::cout << "  IRFormat : "  << lValue << std::endl ;

    Spinnaker::GenApi::CEnumerationPtr ptrFrameRate = pCamera->GetNodeMap().GetNode("IRFrameRate");
//    Spinnaker::GenApi::CIntegerPtr ptrFrameRate = pCamera->IRFrameRate.GetNode();
    int64_t	frameRate = ptrFrameRate->GetIntValue();
    std::cout << "IRFrameRate: "  << frameRate << std::endl ;
//---------------------------------------------------
//     Spinnaker::GenApi::CStringPtr ptrStringSerial = pCamera->DeviceSerialNumber.GetNode();
//     std::string deviceSerialNumber = std::string(ptrStringSerial->GetValue().c_str());
//     std::cout << "Device serial number retrieved as: " << deviceSerialNumber << "..." << std::endl;

//     Spinnaker::GenApi::CFloatPtr ptrAcquisitionFrameRate = pCamera->AcquisitionFrameRate.GetNode();
//     float frameRateToSet = static_cast<float>(ptrAcquisitionFrameRate->GetValue());
//     std::cout << "Frame rate to be set to " << frameRateToSet << "..." << std::endl;

//    Spinnaker::GenApi::CFloatPtr ptrTemperature = pCamera->DeviceTemperature.GetNode();
//    float temperature = static_cast<float>(ptrTemperature->GetValue());
//    printf("device Temperature = %f\n", temperature);
//------------------------------------------------

	  Spinnaker::GenApi::CIntegerPtr ptrWidth = pCamera->Width.GetNode();
    int lptrWidth = ptrWidth->GetValue();
    std::cout << "imageWidth: "  << lptrWidth << std::endl ;

	  Spinnaker::GenApi::CIntegerPtr ptrHeight = pCamera->Height.GetNode();
    int lptrHeight = ptrHeight->GetValue();
    std::cout << "imageHeight: "  << lptrHeight << std::endl ;

	  Spinnaker::GenApi::CIntegerPtr ptrSensorWidth = pCamera->SensorWidth.GetNode();
    int lSensorWidth = ptrSensorWidth->GetValue();
    std::cout << "SensorWidth: "  << lSensorWidth << std::endl ;

	  Spinnaker::GenApi::CIntegerPtr ptrSensorHeight = pCamera->SensorHeight.GetNode();
    int lSensorHeight = ptrSensorHeight->GetValue();
    std::cout << "SensorHeight: "  << lSensorHeight << std::endl ;

    Spinnaker::GenApi::CEnumerationPtr ptrPixelFormat = pCamera->PixelFormat.GetNode();
    int64_t pixelFormat = ptrPixelFormat->GetIntValue();
    printf("pixelFormat: 0x%lx (0x01100007)\n", pixelFormat) ;

//    Spinnaker::GenApi::CIntegerPtr ptrIPAddress = pCamera->GetNodeMap().GetNode("GevDeviceIPAddress");
//    int64_t ipAddress = ptrIPAddress->GetValue();
//    std::cout << "ipAddress: " << ipAddress << std::endl ;
//    printf("ipAddress: 0x%\n", ipAddress) ;
//       CValuePtr pValue = nodeMapTLDevice.GetNode("GevDeviceIPAddress");
	// Manufacturer and model

	//m_ManufacturerEdit.SetWindowText(lManufacturerStr);
	//m_ModelEdit.SetWindowText(lModelStr);
	//m_NameEdit.SetWindowText(lNameStr);
//-------------------------------------------------------------------
	  Spinnaker::GenApi::CStringPtr ptrManufInfo = pCamera->DeviceManufacturerInfo.GetNode();
    std::string devInfo = std::string(ptrManufInfo->GetValue().c_str());
    std::cout << "ManufacturerInfo: "  << devInfo << std::endl ;

    std::cout << "\n\n******  cFLIRHandler::Connect() ==> Device Info. found: ************\n" ;
	  if (devInfo.find("A320G") != string::npos) {
        std::cout << "devInfo.(A320G found): " << devInfo << std::endl ;
		    m_CamType = CAM_TYPE_A3XX;
//		    ptrWidth->SetValue(320);
//		    ptrHeight->SetValue(246);
		    m_Width = 320;
		    m_Height = 246;
		    ptrPixelFormat->SetIntValue(0x01100007);
		    m_bTLUTCapable = true;
	  }
    else if (devInfo.find("A645") != string::npos) {
		    m_CamType = CAM_TYPE_A6XX;
//		    ptrWidth->SetValue(640);
//		    ptrHeight->SetValue(483);
		    m_Width = 640;
		    m_Height = 483;
        std::cout << "devInfo.(A645 found): " << devInfo << std::endl ;
//  		ptrPixelFormat->SetIntValue(0x01100007);
//      std::cout << "set pixelFormat: 0x01100007" << std::endl ;
		    m_bTLUTCapable = true;
		    Spinnaker::GenApi::CEnumerationPtr ptrWindowing = pCamera->GetNodeMap().GetNode("IRWindowing");
		    if (ptrWindowing) {
			      int64_t mode = ptrWindowing->GetIntValue();
			      m_WindowingMode = (int) mode;
			      if (m_WindowingMode == 1) {
				        ptrHeight->SetValue(240);
				        m_Height = 240;
			      }
			      else if (m_WindowingMode == 2) {
				        ptrHeight->SetValue(120);
				        m_Height = 120 ;
			      }
            std::cout << "IRWindowing mode: " << m_WindowingMode << " m_Height: " << m_Height << std::endl << std::endl ;
		    }
	  }
	  else if (devInfo.find("T1KX") != string::npos) {
		    m_CamType = CAM_TYPE_T1K;
		    m_Width = 1024;
		    m_Height = 768;
		    m_bTLUTCapable = false;
	  }
	  else if (devInfo.find("ACAM") != string::npos ) {
        std::cout << "devInfo.find(ACAM) " << std::endl ;
		    m_CamType = CAM_TYPE_ACAM;
		    ptrPixelFormat->SetIntValue(0x01100007);
		    int64_t sw = ptrSensorWidth->GetValue();
		    if (sw == 320) {
			      ptrWidth->SetValue(320);
			      ptrHeight->SetValue(246);
			      m_Width = 320;
			      m_Height = 246;
		    }
		    else {
			      ptrWidth->SetValue(640);
			      ptrHeight->SetValue(483);
			      m_Width = 640;
			      m_Height = 483;
		    }
		    m_bTLUTCapable = true;
	  }
	  else if (devInfo.find("ATAU") != string::npos) {
		    m_CamType = CAM_TYPE_ATAU;
        std::cout << "devInfo.find(ATAU) " << std::endl ;
		    ptrPixelFormat->SetIntValue(0x01100007); // Mono16
		    m_Width = (int)ptrWidth->GetValue();
		    m_Height = (int)ptrHeight->GetValue();
		    Spinnaker::GenApi::CEnumerationPtr ptrDigOut = pCamera->GetNodeMap().GetNode("DigitalOutput");
		    Spinnaker::GenApi::CEnumerationPtr ptrCMOSBitDepth = pCamera->GetNodeMap().GetNode("CMOSBitDepth");
		    ptrDigOut->SetIntValue(3); // Enable
		    if (ptrCMOSBitDepth)
			      ptrCMOSBitDepth->SetIntValue(0); // 14 bit

		    {
			       Spinnaker::GenApi::CIntegerPtr ptrR = pCamera->GetNodeMap().GetNode("R");
			       Spinnaker::GenApi::CFloatPtr ptrB = pCamera->GetNodeMap().GetNode("B");
			       Spinnaker::GenApi::CFloatPtr ptrF = pCamera->GetNodeMap().GetNode("F");
			       Spinnaker::GenApi::CFloatPtr ptrO = pCamera->GetNodeMap().GetNode("O");

			       if (ptrR && ptrB && ptrF && ptrO)  {
				         m_R = (int)ptrR->GetValue();
				         m_B = ptrB->GetValue();
				         m_F = ptrF->GetValue();
				         m_O = ptrO->GetValue();
				         m_bMeasCapable = true;
			       }
		     }

		     Spinnaker::GenApi::CEnumerationPtr ptrTLUTMode = pCamera->GetNodeMap().GetNode("TemperatureLinearMode");
		     Spinnaker::GenApi::CIntegerPtr ptrOptions = pCamera->GetNodeMap().GetNode("CameraOptions");
		     if (ptrTLUTMode) {
			       m_bTLUTCapable = true;
			       ptrTLUTMode->SetIntValue(0); // Turn it off
		     }

		     if (ptrOptions) {
			       int64_t opt = 0;
			       opt = ptrOptions->GetValue();
			       m_bSingleRange = opt & 0x2 ? false : true;
		     }
	  }
//=============================================================
	  if (m_CamType != CAM_TYPE_ATAU)  {
        printf("===========   m_CamType != CAM_TYPE_ATAU ==================== \n") ;
		// Get current object parameters
		    Spinnaker::GenApi::CFloatPtr ptrEmissivity = pCamera->GetNodeMap().GetNode("ObjectEmissivity");
		    Spinnaker::GenApi::CFloatPtr ptrExtOptTransm = pCamera->GetNodeMap().GetNode("ExtOpticsTransmission");
		    Spinnaker::GenApi::CFloatPtr ptrRelHum = pCamera->GetNodeMap().GetNode("RelativeHumidity");
		    Spinnaker::GenApi::CFloatPtr ptrDist = pCamera->GetNodeMap().GetNode("ObjectDistance");
		    Spinnaker::GenApi::CFloatPtr ptrAmb = pCamera->GetNodeMap().GetNode("ReflectedTemperature");
		    Spinnaker::GenApi::CFloatPtr ptrAtm = pCamera->GetNodeMap().GetNode("AtmosphericTemperature");
		    Spinnaker::GenApi::CFloatPtr ptrEOT = pCamera->GetNodeMap().GetNode("ExtOpticsTemperature");

		    if (ptrEmissivity && ptrExtOptTransm && ptrRelHum && ptrDist && ptrAmb && ptrAtm && ptrEOT) 	{
			      try {
				        m_Emissivity = ptrEmissivity->GetValue();
				        m_ExtOptTransm = ptrExtOptTransm->GetValue();
				        m_RelHum = ptrRelHum->GetValue();
				        m_ObjectDistance = ptrDist->GetValue();
				        m_AmbTemp = ptrAmb->GetValue();
				        m_AtmTemp = ptrAtm->GetValue();
				        m_ExtOptTemp = ptrEOT->GetValue();
                printf("m_Emissivity    : %.3f\n", m_Emissivity) ;
                printf("m_ExtOptTransm  : %.3f\n", m_ExtOptTransm) ;
                printf("m_RelHum        : %.3f\n", m_RelHum) ;
                printf("m_ObjectDistance: %.3f\n", m_ObjectDistance) ;
                printf("m_AmbTemp       : %.3f\n", m_AmbTemp) ;
                printf("m_AtmTemp       : %.3f\n", m_AtmTemp) ;
                printf("m_ExtOptTemp    : %.3f\n", m_ExtOptTemp) ;
			      }
			      catch (...) {}
		    }
		// Get current measurement parameters
		    Spinnaker::GenApi::CFloatPtr ptrR = pCamera->GetNodeMap().GetNode("R");
		    Spinnaker::GenApi::CFloatPtr ptrB = pCamera->GetNodeMap().GetNode("B");
		    Spinnaker::GenApi::CFloatPtr ptrF = pCamera->GetNodeMap().GetNode("F");

		    if (ptrR && ptrB && ptrF) {
			      try {
				        double tmpR = 0;
				        tmpR = ptrR->GetValue();
				        m_R = (int) tmpR;
				        m_B = ptrB->GetValue();
				        m_F = ptrF->GetValue();
				        m_bMeasCapable = true;
                printf("m_R   : %d\n", m_R) ;
                printf("m_B   : %.3f\n", m_B) ;
                printf("m_F   : %.3f\n", m_F) ;
			      }
			      catch (...) {}
		    }

		// Get gain (J1) and offset (J0)
		    Spinnaker::GenApi::CIntegerPtr ptrJ0 = pCamera->GetNodeMap().GetNode("J0");
		    Spinnaker::GenApi::CFloatPtr ptrJ1 = pCamera->GetNodeMap().GetNode("J1");
		    if (ptrJ0 && ptrJ1) {
			      try {
				        m_J1 = ptrJ1->GetValue();
				        m_J0 = (unsigned long)ptrJ0->GetValue();
				        m_bMeasCapable = true;
                printf("m_J0  : %ld\n", m_J0) ;
                printf("m_J1  : %.3f\n", m_J1) ;
		        }
			      catch (...) {}
		    }

		// Get spectral response
		    Spinnaker::GenApi::CFloatPtr ptrX = pCamera->GetNodeMap().GetNode("X");
		    Spinnaker::GenApi::CFloatPtr ptrAlpha1 = pCamera->GetNodeMap().GetNode("alpha1");
		    Spinnaker::GenApi::CFloatPtr ptrAlpha2 = pCamera->GetNodeMap().GetNode("alpha2");
		    Spinnaker::GenApi::CFloatPtr ptrBeta1 = pCamera->GetNodeMap().GetNode("beta1");
		    Spinnaker::GenApi::CFloatPtr ptrBeta2 = pCamera->GetNodeMap().GetNode("beta2");
		    if (ptrX && ptrAlpha1 && ptrAlpha2 && ptrBeta1 && ptrBeta2) {
			      try {
				        m_X = ptrX->GetValue();
				        m_alpha1 = ptrAlpha1->GetValue();
				        m_alpha2 = ptrAlpha2->GetValue();
				        m_beta1 = ptrBeta1->GetValue();
				        m_beta2 = ptrBeta2->GetValue();
				        m_bMeasCapable = true;
                printf("X     : %.5f\n", m_X) ;
                printf("alpha1: %.5f\n", m_alpha1) ;
                printf("alpha2: %.5f\n", m_alpha2) ;
                printf("beta1 : %.5f\n", m_beta1) ;
                printf("beta2 : %.5f\n", m_beta2) ;
			      }
			      catch (...) {}
		    }
		    if (m_bMeasCapable) ;
//			     doUpdateCalcConst();
	  }
	  else	{
		// Get current object parameters
		    Spinnaker::GenApi::CFloatPtr ptrEmissivity = pCamera->GetNodeMap().GetNode("ObjectEmissivity");
		    Spinnaker::GenApi::CFloatPtr ptrAmb = pCamera->GetNodeMap().GetNode("ReflectedTemperature");
		    Spinnaker::GenApi::CFloatPtr ptrAtm = pCamera->GetNodeMap().GetNode("AtmosphericTemperature");
		    Spinnaker::GenApi::CFloatPtr ptrEOT = pCamera->GetNodeMap().GetNode("WindowTemperature");
		    Spinnaker::GenApi::CFloatPtr ptrExtOptTransm = pCamera->GetNodeMap().GetNode("WindowTransmission");

		    if (ptrEmissivity && ptrAmb && ptrAtm && ptrEOT && ptrExtOptTransm) 	{
			      m_Emissivity = ptrEmissivity->GetValue();
			      m_AmbTemp = ptrAmb->GetValue();
			      m_AtmTemp = ptrAtm->GetValue();
			      m_ExtOptTemp = ptrEOT->GetValue();
			      m_ExtOptTransm = ptrExtOptTransm->GetValue();
		    }

		// Initiate spectral response
		    m_X = 1.9;
		    m_alpha1 = 0.006569;
		    m_beta1 = -0.002276;
		    m_alpha2 = 0.01262;
		    m_beta2 = -0.00667;

//		if (m_bMeasCapable)
//			doUpdateCalcConst();
	  }

//	m_ctrlFrameRate.ResetContent();
	  if (m_CamType == CAM_TYPE_A6XX)  {
        printf("\n m_CamType == CAM_TYPE_A6XX ： %s\n", devInfo.c_str()) ;
		    m_bFrameRateControlEnabled = true;
//		    int ix;
//		ix = m_ctrlFrameRate.AddString(_T("50.0  Hz"));
//		m_ctrlFrameRate.SetItemData(ix, 0);
//		ix = m_ctrlFrameRate.AddString(_T("25.0  Hz"));
//		m_ctrlFrameRate.SetItemData(ix, 1);
//		ix = m_ctrlFrameRate.AddString(_T("12.5  Hz"));
//		m_ctrlFrameRate.SetItemData(ix, 2);
//		ix = m_ctrlFrameRate.AddString(_T("6.25  Hz"));
//		m_ctrlFrameRate.SetItemData(ix, 4);
//		ix = m_ctrlFrameRate.AddString(_T("3.125 Hz"));
//		m_ctrlFrameRate.SetItemData(ix, 5);
	  }
	  else if (m_CamType == CAM_TYPE_A3XX) {
		    m_bFrameRateControlEnabled = true;
//		int ix;
//		ix = m_ctrlFrameRate.AddString(_T("60.0  Hz"));
//		m_ctrlFrameRate.SetItemData(ix, 0);
//		ix = m_ctrlFrameRate.AddString(_T("30.0  Hz"));
//		m_ctrlFrameRate.SetItemData(ix, 1);
//		ix = m_ctrlFrameRate.AddString(_T("15.0  Hz"));
//		m_ctrlFrameRate.SetItemData(ix, 2);
//		ix = m_ctrlFrameRate.AddString(_T("9.0   Hz"));
//		m_ctrlFrameRate.SetItemData(ix, 3);
//		ix = m_ctrlFrameRate.AddString(_T("7.5   Hz"));
//		m_ctrlFrameRate.SetItemData(ix, 4);
//		ix = m_ctrlFrameRate.AddString(_T("3.75  Hz"));
//		m_ctrlFrameRate.SetItemData(ix, 5);
	  }
	  else if (m_CamType == CAM_TYPE_T1K)  {
		    m_bFrameRateControlEnabled = true;
//		int ix;
//		ix = m_ctrlFrameRate.AddString(_T("30.0  Hz"));
//		m_ctrlFrameRate.SetItemData(ix, 1);
///		ix = m_ctrlFrameRate.AddString(_T("120.0  Hz"));
//		m_ctrlFrameRate.SetItemData(ix, 6);
//		ix = m_ctrlFrameRate.AddString(_T("240.0  Hz"));
//		m_ctrlFrameRate.SetItemData(ix, 7);
	  }
	  else if (m_CamType == CAM_TYPE_ACAM)	{
		    m_bFrameRateControlEnabled = true;
//		int ix;
//		ix = m_ctrlFrameRate.AddString(_T("60.0  Hz"));
//		m_ctrlFrameRate.SetItemData(ix, 0);
//		ix = m_ctrlFrameRate.AddString(_T("30.0  Hz"));
//		m_ctrlFrameRate.SetItemData(ix, 1);
//		ix = m_ctrlFrameRate.AddString(_T("15.0  Hz"));
//		m_ctrlFrameRate.SetItemData(ix, 2);
//		ix = m_ctrlFrameRate.AddString(_T("9.0   Hz"));
//		m_ctrlFrameRate.SetItemData(ix, 3);
	  }

//	m_ctrlPresentation.SetCurSel(0);
	  if (m_CamType != CAM_TYPE_ATAU) 	{
		    int64_t lValue = 0;
		    try {
			      Spinnaker::GenApi::CEnumerationPtr ptrIRFormat = pCamera->GetNodeMap().GetNode("IRFormat");
			      lValue = ptrFrameRate->GetIntValue();
			      if (m_CamType == CAM_TYPE_A6XX && lValue > 2) {
				        lValue = lValue - 1;
			      }
			      else if (m_CamType == CAM_TYPE_T1K) {
				        if (lValue == 1)
					          lValue = 0; // 30 Hz
				        else if (lValue == 6)
					          lValue = 1; // 120 Hz
				        else
					          lValue = 2; // 240 Hz
			      }

//			m_ctrlFrameRate.SetCurSel((int)lValue);

			      lValue = 0;
			      lValue = ptrIRFormat->GetIntValue();
		    }
		    catch (...) {}
//		m_ctrlIRFormat.SetCurSel((int)lValue);
//		m_ctrlUnit.ShowWindow((int)lValue > 0 ? SW_SHOW : SW_HIDE);
//		m_IRFormat = (int)lValue;
//		EnumCases();
//		m_ctrlPresentation.SetCurSel(m_IRFormat == IRFMT_SIGNAL ? 0 : 1);
//		m_iPresentation = (m_IRFormat == IRFMT_SIGNAL ? PRES_SIGNAL : PRES_CELSIUS);
//		m_ctrlPresentation.EnableWindow((m_IRFormat > IRFMT_SIGNAL) || m_bMeasCapable);
	  }
	  else {
//		m_ctrlIRFormat.ResetContent();
//		m_ctrlIRFormat.AddString(_T("Signal"));
		    if (m_bTLUTCapable) {
//			m_ctrlIRFormat.AddString(_T("Temp. linear"));
		    }

//		m_ctrlPresentation.ResetContent();
//		m_ctrlPresentation.AddString(_T("Signal"));
		    if (m_bMeasCapable) {
//			m_ctrlPresentation.AddString(_T("Celsius"));
//			m_ctrlPresentation.AddString(_T("Fahrenheit"));
//			m_ctrlPresentation.AddString(_T("Kelvin"));
		    }
//		m_ctrlPresentation.SetCurSel(0);

//		m_ctrlIRFormat.SetCurSel(0);
//		m_IRFormat = IRFMT_SIGNAL;

//		m_ctrlRange.ResetContent();
//		int ix = m_ctrlRange.AddString(_T("High Gain"));
//		m_ctrlRange.SetItemData(ix, 0);
		    if (!m_bSingleRange)	{
//			ix = m_ctrlRange.AddString(_T("Low Gain"));
//			m_ctrlRange.SetItemData(ix, 1);
		    }

		    if (m_bSingleRange)	{
//			m_ctrlRange.SetCurSel(0);
		    }
		    else	{
			      Spinnaker::GenApi::CEnumerationPtr ptrRange = pCamera->GetNodeMap().GetNode("SensorGainMode");
			      if (ptrRange) {
				        int64_t lValue = ptrRange->GetIntValue();
				        if (lValue == 1) {
					// Low gain
//					m_ctrlRange.SetCurSel(1);
					          m_WT = 303.0;
					          m_W4WT = 150.0;
				        }
				        else {
					// High gain
//					m_ctrlRange.SetCurSel(0);
					          m_WT = 573.0;
					          m_W4WT = 1542.0;
				        }
                printf("SensorGainMode: %ld\n", lValue) ;
			      }
		    }

		    Spinnaker::GenApi::CEnumerationPtr ptrSpeed = pCamera->GetNodeMap().GetNode("SensorFrameRate");
		    if (ptrSpeed) {
			      int64_t lValue = ptrSpeed->GetIntValue();
			      m_bFast = (lValue == 4); // Used for ATAU
            printf("SensorFrameRate: %ld\n", lValue) ;
			      if (m_bFast) {
				        m_bFrameRateControlEnabled = true;
//				        int ix;
//			ix = m_ctrlFrameRate.AddString(_T("60.0  Hz"));
//				m_ctrlFrameRate.SetItemData(ix, 4);
//				ix = m_ctrlFrameRate.AddString(_T("30.0  Hz"));
//				m_ctrlFrameRate.SetItemData(ix, 0);
				        Spinnaker::GenApi::CEnumerationPtr ptrRate = pCamera->GetNodeMap().GetNode("SensorVideoStandard");
				        lValue = 0;
				        if (ptrRate) lValue = ptrRate->GetIntValue() ;
//				m_ctrlFrameRate.SetCurSel(lValue == 4 ? 0 : 1);
                printf("SensorVideoStandard: %ld\n", lValue) ;
			      }
		    }
	  }

	  Spinnaker::GenApi::CIntegerPtr ptrPktSize = pCamera->GetNodeMap().GetNode("GevSCPSPacketSize");
//	if (ptrPktSize) ptrPktSize->SetValue(1500);

////	if (m_Timer == 0)
//		m_Timer = (UINT)SetTimer(0x40, 500, NULL);

//	// Ready image reception
//	StartStreaming();

//	// Sync up UI
//	EnableInterface();
    std::cout << "\n******  cFLIRHandler::Connect()  End of Device Info.: ************\n\n" ;
}

//==================================================
//   mmThermalProperty.insert(std::pair<std::string, double>(mName, value)) ;
//   std::map<std::string, double>::iterator it = mmThermalProperty.begin() ;
//   while (it != mmThermalProperty.end())   {
//   if ( it->first == "m_B") {
//       double value = it->second ;
//       it++ ;
//   }
//==================================================
sThermalProperty& cFLIRHandler::GetThermalProperty( CameraPtr pCamera )
{
    mmThermalProperty.clear( ) ;
    printf("\n******  cFLIRHandler::GetThermalProperty( %s )  Device Info.: ************\n\n", mDevAddress.c_str()) ;

    // Device connection, packet size negotiation and stream opening
    // Connect device
	  Spinnaker::GenApi::CStringPtr ptrStringManufacturer = pCamera->DeviceVendorName.GetNode();
    std::string lManufacturerStr = std::string(ptrStringManufacturer->GetValue().c_str());
    std::cout << "DeviceVendorName: "  << lManufacturerStr << std::endl ;

	  Spinnaker::GenApi::CStringPtr ptrStringModel = pCamera->DeviceModelName.GetNode();
    std::string lModelStr = std::string(ptrStringModel->GetValue().c_str());
    std::cout << "Device ModelName: "  << lModelStr << std::endl ;

	  Spinnaker::GenApi::CStringPtr ptrStringName = pCamera->DeviceUserID.GetNode();
    std::string lNameStr = std::string(ptrStringName->GetValue().c_str());
    std::cout << "Device    UserID: "  << lNameStr << std::endl ;

    Spinnaker::GenApi::CIntegerPtr ptrWidth = pCamera->Width.GetNode();
    int lptrWidth = ptrWidth->GetValue();
    Spinnaker::GenApi::CIntegerPtr ptrWidthMax = pCamera->WidthMax.GetNode();
    int lptrWidthMax = ptrWidthMax->GetValue();
    printf("imageWidth: %d, WidthMax: %d\n", lptrWidth, lptrWidthMax) ;

    Spinnaker::GenApi::CIntegerPtr ptrHeight = pCamera->Height.GetNode();
    int lptrHeight = ptrHeight->GetValue();
    Spinnaker::GenApi::CIntegerPtr ptrHeightMax = pCamera->HeightMax.GetNode();
    int lptrHeightMax = ptrHeightMax->GetValue();
    printf("imageHeight: %d, HeightMax: %d\n", lptrHeight, lptrHeightMax) ;

    Spinnaker::GenApi::CIntegerPtr ptrSensorWidth = pCamera->SensorWidth.GetNode();
    int lSensorWidth = ptrSensorWidth->GetValue();
    std::cout << "SensorWidth : "  << lSensorWidth << std::endl ;

    Spinnaker::GenApi::CIntegerPtr ptrSensorHeight = pCamera->SensorHeight.GetNode();
    int lSensorHeight = ptrSensorHeight->GetValue();
    std::cout << "SensorHeight: "  << lSensorHeight << std::endl ;

//    Spinnaker::GenApi::CIntegerPtr ptrPixelDynamicRangeMax = pCamera->PixelDynamicRangeMax.GetNode();
//    int lPixelDynamicRangeMax = ptrPixelDynamicRangeMax->GetValue();
//    std::cout << "PixelDynamicRangeMax: "  << lPixelDynamicRangeMax << std::endl ;

//    Spinnaker::GenApi::CIntegerPtr ptrStride = pCamera->Stride.GetNode();
//    int lStride = ptrStride->GetValue();
//    std::cout << "Stride      : "  << lStride << std::endl ;

//    Spinnaker::GenApi::CIntegerPtr ptrBitsPerPixel = pCamera->BitsPerPixel.GetNode();
//    int lBitsPerPixel = ptrBitsPerPixel->GetValue();
//    std::cout << "BitsPerPixel: "  << lBitsPerPixel << std::endl ;
//=========================================================================================
    CEnumerationPtr ptrPixelFormat = pCameraPtr->PixelFormat.GetNode();
    int64_t pixelFormat = ptrPixelFormat->GetIntValue();
    printf("PixelFormat: 0x%08lX\n", pixelFormat);
    if (IsAvailable(ptrPixelFormat) && IsWritable(ptrPixelFormat))  {
//-----------------------------------------------------------------------------
        CEnumEntryPtr ptrPixelFormatMono16 = ptrPixelFormat->GetEntryByName("Mono16");
        if (!IsAvailable(ptrPixelFormatMono16) || !IsReadable(ptrPixelFormatMono16))  {
            CEnumEntryPtr ptrPixelFormatMono8 = ptrPixelFormat->GetEntryByName("Mono8");
            if (!IsAvailable(ptrPixelFormatMono8) || !IsReadable(ptrPixelFormatMono8))
                printf("Unable to set pixel format to Mono8, Mono16 ...\n") ;
            else  {
                int64_t pixelFormat = ptrPixelFormat->GetIntValue();
  //                    ptrPixelFormat->SetIntValue( pixelFormat ) ;
  //                    ptrPixelFormat->SetIntValue(ptrPixelFormatMono16->GetValue());
                printf("PixelFormat: %s (0x%08lX)\n",
                      ptrPixelFormatMono8->GetSymbolic().c_str(), pixelFormat);
            }
        }
        else {
            int64_t pixelFormat = ptrPixelFormat->GetIntValue();
//                    ptrPixelFormat->SetIntValue( pixelFormat ) ;
//                    ptrPixelFormat->SetIntValue(ptrPixelFormatMono16->GetValue());
            printf("PixelFormat: %s (0x%08lX)\n",
                    ptrPixelFormatMono16->GetSymbolic().c_str(), pixelFormat);
        }
    }
//============================================================================
//  https://flir.custhelp.com/app/answers/detail/a_id/1021/~/temperature-linear-mode
    Spinnaker::GenApi::CEnumerationPtr ptrIRFormat = pCamera->GetNodeMap().GetNode("IRFormat");
    int64_t lValue = ptrIRFormat->GetIntValue();
    mmThermalProperty.insert(std::pair<std::string, double>("IRFormat", (double) lValue)) ;
    if (emIRFormatType == eIRFormatType::LINEAR_10MK) {
        Spinnaker::GenApi::CEnumEntryPtr ptrTempLinearHigh = ptrIRFormat->GetEntryByName("TemperatureLinear10mK");
        int64_t	tempHigh = ptrTempLinearHigh->GetValue() ;
        ptrIRFormat->SetIntValue(tempHigh) ;
        printf("IRFormat(%ld):LINEAR_10MK(TempHigh) --> %ld\n", lValue, tempHigh) ;
    }
    else if (emIRFormatType == eIRFormatType::LINEAR_100MK) {
        Spinnaker::GenApi::CEnumEntryPtr ptrTempLinearLow = ptrIRFormat->GetEntryByName("TemperatureLinear100mK");
        int64_t	tempLow = ptrTempLinearLow->GetValue() ;
        ptrIRFormat->SetIntValue(tempLow) ;
        printf("IRFormat(%ld):LINEAR_100MK(TempLow ) --> %ld\n", lValue, tempLow) ;
    }
    else if (emIRFormatType == eIRFormatType::RADIOMETRIC) {
        Spinnaker::GenApi::CEnumEntryPtr ptrTempRadiometric = ptrIRFormat->GetEntryByName("Radiometric");
        int64_t	tempRadiometric = ptrTempRadiometric->GetValue() ;
        ptrIRFormat->SetIntValue(tempRadiometric) ;
        printf("IRFormat(%ld):RADIOMETRICK --> %ld\n", lValue, tempRadiometric) ;
    }

//-----------------------------------------------------------------------------
// SetAcquisitionFrameRate(INodeMap& nodeMap)  in examples folder Planar.cpp
//    CBooleanPtr ptrAcquisitionFrameRateEnable = nodeMap.GetNode("AcquisitionFrameRateEnable");
//    if (!IsWritable(ptrAcquisitionFrameRateEnable))   {
//        std::cout << "Unable to set Acquisition Frame Rate Enable to true (enum retrieval). Aborting..." << std::endl << std::endl;
////            return -1;
//    }
//    ptrAcquisitionFrameRateEnable->SetValue(true);

//    Spinnaker::GenApi::CFloatPtr ptrAcquisitionFrameRate = pCamera->GetNodeMap().GetNode("AcquisitionFrameRate");
//    if (!IsReadable(ptrAcquisitionFrameRate) || !IsWritable(ptrAcquisitionFrameRate))  {
//        cout << "Unable to set Acquisition Frame Rate" << endl << endl;
////            return -1;
//    }
//    float frameRate = static_cast<float>(ptrAcquisitionFrameRate->GetValue());
//    std::cout << "Acquisition Frame Rate: " << frameRate << std::endl;
////        const double frameRate = 10.0;
////    ptrAcquisitionFrameRate->SetValue(frameRate);
////    std::cout << "Set Acquisition Frame Rate to  " << frameRate << std::endl;
//-------------------------------------------------------------
    Spinnaker::GenApi::CEnumerationPtr ptrIRFrameRate = pCamera->GetNodeMap().GetNode("IRFrameRate");
    //    Spinnaker::GenApi::CIntegerPtr ptrFrameRate = pCamera->IRFrameRate.GetNode();
    int64_t	IRFrameRate = ptrIRFrameRate->GetIntValue();
    std::cout << "IRFrameRate: "  << IRFrameRate << std::endl ;
    mmThermalProperty.insert(std::pair<std::string, double>("IRFrameRate", (double) IRFrameRate)) ;
//------------------------------------------------------------------------
//    Spinnaker::GenApi::CEnumerationPtr ptrTriggerMode = pCamera->GetNodeMap().GetNode("TriggerMode");
//    if (!IsReadable(ptrTriggerMode)) {
//        std::cout << "Unable to disable trigger mode (node retrieval). Aborting..." << std::endl;
//    }

//    Spinnaker::GenApi::CEnumEntryPtr ptrTriggerModeOff = ptrTriggerMode->GetEntryByName("Off");
//    if (!IsReadable(ptrTriggerModeOff))  {
//        cout << "Unable to disable trigger mode (enum entry retrieval). Aborting..." << endl;
//    }
//    ptrTriggerMode->SetIntValue(ptrTriggerModeOff->GetValue());
//    cout << "Trigger mode disabled..." << endl;
//-------------------------------------------------------------------
	  Spinnaker::GenApi::CStringPtr ptrManufInfo = pCamera->DeviceManufacturerInfo.GetNode();
    std::string devInfo = std::string(ptrManufInfo->GetValue().c_str());
    std::cout << "ManufacturerInfo: "  << devInfo << std::endl ;
    if (devInfo.find("A320G") != string::npos) {
        std::cout << "devInfo.(A320G found): " << devInfo << std::endl ;
		    m_CamType = CAM_TYPE_A3XX;
//		    ptrWidth->SetValue(320);
//		    ptrHeight->SetValue(246);
		    m_Width = 320;
		    m_Height = 246;
		    ptrPixelFormat->SetIntValue(0x01100007);
		    m_bTLUTCapable = true;
	  }
    else if (devInfo.find("A645") != string::npos) {
		    m_CamType = CAM_TYPE_A6XX;
//		    ptrWidth->SetValue(640);
//		    ptrHeight->SetValue(483);
		    m_Width = 640;
		    m_Height = 483;
        std::cout << "devInfo.(A645 found): " << devInfo << std::endl ;
    }
	  else if (devInfo.find("T1KX") != string::npos) {
		    m_CamType = CAM_TYPE_T1K;
		    m_Width = 1024;
		    m_Height = 768;
		    m_bTLUTCapable = false;
	  }
	  else if (devInfo.find("ACAM") != string::npos ) {
        std::cout << "devInfo.find(ACAM) " << std::endl ;
		    m_CamType = CAM_TYPE_ACAM;
		    ptrPixelFormat->SetIntValue(0x01100007);
		    int64_t sw = ptrSensorWidth->GetValue();
		    if (sw == 320) {
			      ptrWidth->SetValue(320);
			      ptrHeight->SetValue(246);
			      m_Width = 320;
			      m_Height = 246;
		    }
		    else {
			      ptrWidth->SetValue(640);
			      ptrHeight->SetValue(483);
			      m_Width = 640;
			      m_Height = 483;
		    }
		    m_bTLUTCapable = true;
	  }
	  else if (devInfo.find("ATAU") != string::npos) {
		    m_CamType = CAM_TYPE_ATAU;
        std::cout << "devInfo.find(ATAU) " << std::endl ;
		    ptrPixelFormat->SetIntValue(0x01100007); // Mono16
		    m_Width = (int)ptrWidth->GetValue();
		    m_Height = (int)ptrHeight->GetValue();
        Spinnaker::GenApi::CEnumerationPtr ptrDigOut = pCamera->GetNodeMap().GetNode("DigitalOutput");
		    Spinnaker::GenApi::CEnumerationPtr ptrCMOSBitDepth = pCamera->GetNodeMap().GetNode("CMOSBitDepth");
		    ptrDigOut->SetIntValue(3); // Enable
		    if (ptrCMOSBitDepth)
			      ptrCMOSBitDepth->SetIntValue(0); // 14 bit

		    {
			       Spinnaker::GenApi::CIntegerPtr ptrR = pCamera->GetNodeMap().GetNode("R");
			       Spinnaker::GenApi::CFloatPtr ptrB = pCamera->GetNodeMap().GetNode("B");
			       Spinnaker::GenApi::CFloatPtr ptrF = pCamera->GetNodeMap().GetNode("F");
			       Spinnaker::GenApi::CFloatPtr ptrO = pCamera->GetNodeMap().GetNode("O");

			       if (ptrR && ptrB && ptrF && ptrO)  {
				         m_R = (int)ptrR->GetValue();
				         m_B = ptrB->GetValue();
				         m_F = ptrF->GetValue();
				         m_O = ptrO->GetValue();
				         m_bMeasCapable = true;
			       }
		     }

		     Spinnaker::GenApi::CEnumerationPtr ptrTLUTMode = pCamera->GetNodeMap().GetNode("TemperatureLinearMode");
		     Spinnaker::GenApi::CIntegerPtr ptrOptions = pCamera->GetNodeMap().GetNode("CameraOptions");
		     if (ptrTLUTMode) {
			       m_bTLUTCapable = true;
			       ptrTLUTMode->SetIntValue(0); // Turn it off
		     }

		     if (ptrOptions) {
			       int64_t opt = 0;
			       opt = ptrOptions->GetValue();
			       m_bSingleRange = opt & 0x2 ? false : true;
		     }
	  }
//====================================================================
  // Get current measurement parameters
    Spinnaker::GenApi::CFloatPtr ptrR = pCamera->GetNodeMap().GetNode("R");
    if ( ptrR ) {
        double tmpR = ptrR->GetValue();
        m_R = (int) tmpR ;
        smProperty.mR = tmpR ;
        mmThermalProperty.insert(std::pair<std::string, double>("R", smProperty.mR)) ;
        printf("R               : %d\n", m_R) ;
    }
    Spinnaker::GenApi::CFloatPtr ptrB = pCamera->GetNodeMap().GetNode("B");
    if ( ptrB )  {
        m_B = ptrB->GetValue();
        smProperty.mB = m_B ;
        mmThermalProperty.insert(std::pair<std::string, double>("B", smProperty.mB)) ;
        printf("B               : %.3f\n", m_B) ;
    }
    Spinnaker::GenApi::CFloatPtr ptrF = pCamera->GetNodeMap().GetNode("F");
    if ( ptrF ) {
        m_F = ptrF->GetValue();
        smProperty.mF = m_F ;
        mmThermalProperty.insert(std::pair<std::string, double>("F", smProperty.mF)) ;
        printf("F               : %.3f\n", m_F) ;
    }
  // Get current object parameters
    Spinnaker::GenApi::CFloatPtr ptrEmissivity = pCamera->GetNodeMap().GetNode("ObjectEmissivity");
    if (ptrEmissivity)  {
        m_Emissivity = ptrEmissivity->GetValue();
        smProperty.mEmissivity = m_Emissivity ;
        mmThermalProperty.insert(std::pair<std::string, double>("ObjectEmissivity", smProperty.mEmissivity)) ;

    ptrEmissivity->SetValue( 0.97 );

        printf("ObjectEmissivity: %.3f, and set to 0.97\n", m_Emissivity) ;

    }
    Spinnaker::GenApi::CFloatPtr ptrExtOptTransm = pCamera->GetNodeMap().GetNode("ExtOpticsTransmission");
    if ( ptrExtOptTransm )  {
        m_ExtOptTransm = ptrExtOptTransm->GetValue();
        smProperty.mExtOptTransm = m_ExtOptTransm ;
        mmThermalProperty.insert(std::pair<std::string, double>("ExtOpticsTransmission", smProperty.mExtOptTransm)) ;
        printf("m_ExtOptTransm  : %.3f\n", m_ExtOptTransm) ;
    }
    Spinnaker::GenApi::CFloatPtr ptrRelHum = pCamera->GetNodeMap().GetNode("RelativeHumidity");
    if ( ptrRelHum )  {
        m_RelHum = ptrRelHum->GetValue();
        smProperty.mRelHum = m_RelHum ;
        mmThermalProperty.insert(std::pair<std::string, double>("RelativeHumidity", smProperty.mRelHum)) ;
        printf("m_RelHum        : %.3f\n", m_RelHum) ;
    }
    Spinnaker::GenApi::CFloatPtr ptrDist = pCamera->GetNodeMap().GetNode("ObjectDistance");
    if ( ptrDist ) {
        m_ObjectDistance = ptrDist->GetValue();
        smProperty.mObjectDistance = m_ObjectDistance ;
        printf("m_ObjectDistance: %.3f\n", m_ObjectDistance) ;
    }
    Spinnaker::GenApi::CFloatPtr ptrAmb = pCamera->GetNodeMap().GetNode("ReflectedTemperature");
    if ( ptrAmb )   {
        m_AmbTemp = ptrAmb->GetValue();
        smProperty.mAmbTemp = m_AmbTemp ;
        printf("m_AmbTemp       : %.3f\n", m_AmbTemp) ;
    }
    Spinnaker::GenApi::CFloatPtr ptrAtm = pCamera->GetNodeMap().GetNode("AtmosphericTemperature");
    if ( ptrAtm )   {
        m_AtmTemp = ptrAtm->GetValue();
        smProperty.mAtmTemp = m_AtmTemp ;
        printf("m_AtmTemp       : %.3f\n", m_AtmTemp) ;
    }
    Spinnaker::GenApi::CFloatPtr ptrEOT = pCamera->GetNodeMap().GetNode("ExtOpticsTemperature");
    if ( ptrEOT) 	{
        m_ExtOptTemp = ptrEOT->GetValue();
        smProperty.mExtOptTemp = m_ExtOptTemp ;
        printf("m_ExtOptTemp    : %.3f\n", m_ExtOptTemp) ;
    }

    Spinnaker::GenApi::CFloatPtr ptrWinTemp = pCamera->GetNodeMap().GetNode("WindowTemperature");
    if (ptrWinTemp) 	{
        double windowTemp = ptrWinTemp->GetValue();
        printf("m_WindowTemp.   : %.3f\n", windowTemp) ;
    }
    Spinnaker::GenApi::CFloatPtr ptrWinTransm = pCamera->GetNodeMap().GetNode("WindowTransmission");
    if (ptrWinTransm) 	{
        double windowTransm = ptrWinTransm->GetValue();
        printf("WinTransmission : %.3f\n", windowTransm) ;
    }
	// Get gain (J1) and offset (J0)
    Spinnaker::GenApi::CIntegerPtr ptrJ0 = pCamera->GetNodeMap().GetNode("J0");
    if (ptrJ0) {
        m_J0 = (unsigned long)ptrJ0->GetValue();
        printf("m_J0 (Offset)   : %ld\n", m_J0) ;
    }
    Spinnaker::GenApi::CFloatPtr ptrJ1 = pCamera->GetNodeMap().GetNode("J1");
    if (ptrJ1) {
        m_J1 = ptrJ1->GetValue();
        printf("m_J1 ( Gain )   : %.3f\n", m_J1) ;
    }
  		// Get spectral response
    Spinnaker::GenApi::CFloatPtr ptrX = pCamera->GetNodeMap().GetNode("X");
    if ( ptrX ) {
        m_X = ptrX->GetValue();
        smProperty.mX = m_X ;
        printf("X               : %.5f\n", m_X) ;
    }
    Spinnaker::GenApi::CFloatPtr ptrAlpha1 = pCamera->GetNodeMap().GetNode("alpha1");
    if ( ptrAlpha1 ) {
        m_alpha1 = ptrAlpha1->GetValue();
        smProperty.mAlpha1 = m_alpha1 ;
        printf("alpha1          : %.6f\n", m_alpha1) ;
    }
    Spinnaker::GenApi::CFloatPtr ptrAlpha2 = pCamera->GetNodeMap().GetNode("alpha2");
    if ( ptrAlpha2 ) {
        m_alpha2 = ptrAlpha2->GetValue();
        smProperty.mAlpha2 = m_alpha2 ;
        printf("alpha2          : %.6f\n", m_alpha2) ;
    }
    Spinnaker::GenApi::CFloatPtr ptrBeta1 = pCamera->GetNodeMap().GetNode("beta1");
    if ( ptrBeta1 ) {
        m_beta1 = ptrBeta1->GetValue();
        smProperty.mBeta1 = m_beta1 ;
        printf("beta1           : %.6f\n", m_beta1) ;
    }
    Spinnaker::GenApi::CFloatPtr ptrBeta2 = pCamera->GetNodeMap().GetNode("beta2");
    if ( ptrBeta2) {
        m_beta2 = ptrBeta2->GetValue();
        smProperty.mBeta2 = m_beta2 ;
        printf("beta2           : %.6f\n", m_beta2) ;
    }
//===================================================
//#include <SpinGenApi/RegisterPortImpl.h>
//    uint32_t 	registers = 0x71C ;
//    int64_t   address = 0x71C ;
//    int64_t   length = 1 ;
//    pCamera->ReadRegister(	&registers, address, length ) ;
//    int64_t regLength = Spinnaker::GenApi::GetLength() ;
//    int64_t regAddress = Spinnaker::GenApi::GetAddress() ;

//    unsigned int ulValue;
//    pCamera->ReadRegister(0x71C, &ulValue);
    printf("\n***  End of GetThermalPropertity( %s ) ************\n", mDevAddress.c_str()) ;
    return smProperty ;
}
