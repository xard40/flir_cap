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
cv::Mat& cFLIRHandler::initFLIRHandler( CameraPtr cameraPtr, cv::Mat& thumbnail )
{
    printf("cFLIRHandler::initFLIRHandler(%s)\n", mDevAddress.c_str()) ;
    pCameraPtr = cameraPtr ;
    INodeMap & nodeMapTLDevice = pCameraPtr->GetTLDeviceNodeMap();
    printDeviceInfo(nodeMapTLDevice);
//------------------------------------------------
    pCameraPtr->Init( ) ;
    INodeMap& nodeMap = pCameraPtr->GetNodeMap();
    configImageSettings( nodeMap ) ;

  Connect( cameraPtr ) ;   // test flirHandler::Connect() function

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
//--------------------------------------------------------
    mPixelFormatEnums = pImage->GetPixelFormat() ;
    ImagePtr convertedImage = pImage ;
//    ImagePtr convertedImage = ConvertImage( pImage ) ;
    uchar* pData = static_cast<uchar *>(convertedImage->GetData());
    convertedImage->Save("flirImage.png") ;
    if (mPixelFormatEnums == PixelFormat_Mono8)
        printf("cFLIRHandler::initFLIRHandler( Mono8 ), Image W.: %d, H.: %d, Stride: %d\n\n", mImageWidth, mImageHeight, mImageStride) ;
    else if (mPixelFormatEnums == PixelFormat_Mono16)
        printf("cFLIRHandler::initFLIRHandler( Mono16 ), Image W.: %d, H.: %d, Stride: %d\n\n",mImageWidth, mImageHeight, mImageStride) ;
    else printf("cFLIRHandler::initFLIRHandler( Pix.Format: %d ), Image W.: %d, H.: %d, Stride: %d\n\n",
        mPixelFormatEnums, mImageWidth, mImageHeight, mImageStride) ;
//-----------------------------------------------------------------
    mFirstFrame = cv::Mat(mImageHeight + mOffsetY, mImageWidth + mOffsetX, CV_8UC1, pData, mImageStride );
    cv::Size thumbnail_size = cv::Size(0, 0);
    thumbnail_size.width = 128 ;
    thumbnail_size.height	= thumbnail_size.width * (mFirstFrame.rows / (float) mFirstFrame.cols) ;
    cv::resize(mFirstFrame, mThumbnail, thumbnail_size, 0, 0, cv::INTER_AREA) ;
    thumbnail = mThumbnail ;
    return mFirstFrame ;
}

/**********************************************************
*
***********************************************************/
void cFLIRHandler::startCapture( )
{
    printf("cFLIRHandler::startCapture( %s )\n", mDevAddress.c_str()) ;
		mCBCapStarted = true ;
    pCBCapThread = new std::thread(&cFLIRHandler::cbCaptureImage, this, mDevName + "cbRead") ;
    mThreadMap[mDevName + "cbRead"] = pCBCapThread->native_handle( ) ;
    pCBCapThread->detach() ;
    std::this_thread::sleep_for(std::chrono::milliseconds(400));
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
		printf("cFLIRHandler::cbCaptureFrame( start ), waiting for CMD request\n") ;
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
    ImagePtr convertedImage = pSrcImage ;
//    ImagePtr convertedImage = ConvertImage( pSrcImage ) ;
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
        static int cnt ;
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
int cFLIRHandler::configImageSettings(INodeMap& nodeMap)
{
  int result = 0;

    printf("\n*** ConfigImageSettings(%s) : CONFIGURING IMAGE SETTINGS ***\n", mDevAddress.c_str()) ;
    try   {
//        CStringPtr pStringNode = (CStringPtr) nodeMap.GetNode("DeviceSerialNumber");
//        cout << "Serial number: " << pStringNode->GetValue() << endl;
//        CFloatPtr pNode = (CFloatPtr) nodeMap.GetNode("DeviceTemperature");
//        int numLoops = 5;
//    	  for (int i = 0; i < numLoops; i++) {
//    	      printf("Loop = %d, Temperature = %f\n", i, pNode->GetValue());
//    	      sleep(2000);
//    	  }
        //
        // Apply mono 8 pixel format
        //
        // *** NOTES ***
        // Enumeration nodes are slightly more complicated to set than other
        // nodes. This is because setting an enumeration node requires working
        // with two nodes instead of the usual one.
        //
        // As such, there are a number of steps to setting an enumeration node:
        // retrieve the enumeration node from the nodemap, retrieve the desired
        // entry node from the enumeration node, retrieve the integer value from
        // the entry node, and set the new value of the enumeration node with
        // the integer value from the entry node.
        //
        CEnumerationPtr ptrPixelFormat = nodeMap.GetNode("PixelFormat");
        if (IsAvailable(ptrPixelFormat) && IsWritable(ptrPixelFormat))  {
            CEnumEntryPtr ptrPixelFormatBayerRG8 = ptrPixelFormat->GetEntryByName("BayerRG8");
            if (!IsAvailable(ptrPixelFormatBayerRG8) || !IsReadable(ptrPixelFormatBayerRG8))   {
//=========================================================================================
                CEnumEntryPtr ptrPixelFormatMono16 = ptrPixelFormat->GetEntryByName("Mono16");
                if (!IsAvailable(ptrPixelFormatMono16) || !IsReadable(ptrPixelFormatMono16))  {
                    cout << "Unable to set pixel format to Mono16 ..." << endl << endl;
                    return result ;
                }
                else ptrPixelFormat->SetIntValue(ptrPixelFormatMono16->GetValue());
                cout << "Pixel format set to: " << ptrPixelFormatMono16->GetSymbolic() << " (ptrPixelFormatMono16)..." << endl;
//---------------------------------------------------------------
//                CEnumEntryPtr ptrPixelFormatMono8 = ptrPixelFormat->GetEntryByName("Mono8");
//                if (!IsAvailable(ptrPixelFormatMono8) || !IsReadable(ptrPixelFormatMono8))  {
//                    cout << "Unable to set pixel format to BayerRG8 or Mono8. Aborting..." << endl << endl;
//                    return result ;
//                }
//                ptrPixelFormat->SetIntValue(ptrPixelFormatMono8->GetValue());
//                cout << "Pixel format set to: " << ptrPixelFormatMono8->GetSymbolic() << " (ptrPixelFormatMono8)..." << endl;
//=============================================================================================
            }
            else  {
                ptrPixelFormat->SetIntValue(ptrPixelFormatBayerRG8->GetValue());
                cout << "Pixel format set to: " << ptrPixelFormatBayerRG8->GetSymbolic() << "( ptrPixelFormatBayerRG8)..." << endl;
            }
        }
        else
            std::cout << "Pixel format not available..." << std::endl;
//-------------------------------------------------------------
//        CEnumerationPtr bayerPatternPtr = nodeMap.GetNode("PixelColorFilter");
//        std::string bayerPattern = bayerPatternPtr->GetCurrentEntry()->GetName().c_str();
//        std::string dstPixelFormat;
//        if (bayerPattern.compare("EnumEntry_PixelColorFilter_BayerRG") == 0) {
//            dstPixelFormat = "BayerRG8";
//        }
//        else if (bayerPattern.compare("EnumEntry_PixelColorFilter_BayerBG") == 0) {
//            dstPixelFormat = "BayerBG8";
//        }
//        else if (bayerPattern.compare("EnumEntry_PixelColorFilter_BayerGB") == 0) {
//            dstPixelFormat = "BayerGB8";
//        }
//        else if (bayerPattern.compare("EnumEntry_PixelColorFilter_BayerGR") == 0) {
//            dstPixelFormat = "BayerGR8";
//        }
//        printf("bayerPattern: %s, Pixel format: %s\n", bayerPattern.c_str(), dstPixelFormat.c_str()) ;
//        int setVal = ptrPixelFormat->GetEntryByName(Spinnaker::GenICam::gcstring(dstPixelFormat.c_str()))->GetValue();
//        if (IsWritable(ptrPixelFormat)) {
//            ptrPixelFormat->SetIntValue(setVal);
//        }
  //      else {
  //          SysUtil::warningOutput("PixelFormat is not writable !");
  //          SysUtil::warningOutput("Current PixelFormat is " + dstPixelFormat);
  //      }


//-------------------------------------------------------------
/*
        CEnumEntryPtr ptrPixelFormatRGB8 = ptrPixelFormat->GetEntryByName("BGR8");
        if (IsAvailable(ptrPixelFormatRGB8) && IsReadable(ptrPixelFormatRGB8))  {
            int64_t pixelFormatRGB8 = ptrPixelFormatRGB8->GetValue();
            ptrPixelFormat->SetIntValue(pixelFormatRGB8);
            std::cout << "Pixel format set to " << ptrPixelFormat->GetCurrentEntry()->GetSymbolic() << "..." << std::endl;
        }
        else  {
            std::cout << "PixelFormatRGB8 not available..." << std::endl;
            std::cout << mDevName << ": " << mDevAddress << "  Current available Pixel format is: " << ptrPixelFormat->GetCurrentEntry()->GetSymbolic() << std::endl ;
        }
*/
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
            cout << "  Width  set to: " << mImageWidth << endl;
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
            std::cout << " Height  set to: " << mImageHeight << std::endl;
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

        CStringPtr pStringNode = (CStringPtr) nodeMap.GetNode("DeviceModelName");
        std::cout << "DeviceModelName: " << pStringNode->GetValue() << std::endl;
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
    std::cout << "\n*** end of CONFIGURING CUSTOM IMAGE SETTINGS ***" << std::endl << std::endl ;
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
void cFLIRHandler::Connect( CameraPtr m_pCamera )
{
	  if (m_pCamera == 0)
		    return ;
    printf("\n******  cFLIRHandler::Connect( %s )  Device Info.: ************\n\n", mDevAddress.c_str()) ;
// Device connection, packet size negotiation and stream opening
// Connect device
	  Spinnaker::GenApi::CStringPtr ptrStringManufacturer = m_pCamera->DeviceVendorName.GetNode();
    std::string lManufacturerStr = std::string(ptrStringManufacturer->GetValue().c_str());
    std::cout << "DeviceVendorName: "  << lManufacturerStr << std::endl ;

	  Spinnaker::GenApi::CStringPtr ptrStringModel = m_pCamera->DeviceModelName.GetNode();
    std::string lModelStr = std::string(ptrStringModel->GetValue().c_str());
    std::cout << "Device ModelName: "  << lModelStr << std::endl ;

	  Spinnaker::GenApi::CStringPtr ptrStringName = m_pCamera->DeviceUserID.GetNode();
    std::string lNameStr = std::string(ptrStringName->GetValue().c_str());
    std::cout << "Device    UserID: "  << lNameStr << std::endl ;

    Spinnaker::GenApi::CEnumerationPtr ptrIRFormat = m_pCamera->GetNodeMap().GetNode("IRFormat");
    int64_t lValue = ptrIRFormat->GetIntValue();
    std::cout << "  IRFormat : "  << lValue << std::endl ;

    Spinnaker::GenApi::CEnumerationPtr ptrFrameRate = m_pCamera->GetNodeMap().GetNode("IRFrameRate");
//    Spinnaker::GenApi::CIntegerPtr ptrFrameRate = m_pCamera->IRFrameRate.GetNode();
    int64_t	frameRate = ptrFrameRate->GetIntValue();
    std::cout << "IRFrameRate: "  << frameRate << std::endl ;
//---------------------------------------------------
//     Spinnaker::GenApi::CStringPtr ptrStringSerial = m_pCamera->DeviceSerialNumber.GetNode();
//     std::string deviceSerialNumber = std::string(ptrStringSerial->GetValue().c_str());
//     std::cout << "Device serial number retrieved as: " << deviceSerialNumber << "..." << std::endl;

//     Spinnaker::GenApi::CFloatPtr ptrAcquisitionFrameRate = m_pCamera->AcquisitionFrameRate.GetNode();
//     float frameRateToSet = static_cast<float>(ptrAcquisitionFrameRate->GetValue());
//     std::cout << "Frame rate to be set to " << frameRateToSet << "..." << std::endl;

//    Spinnaker::GenApi::CFloatPtr ptrTemperature = m_pCamera->DeviceTemperature.GetNode();
//    float temperature = static_cast<float>(ptrTemperature->GetValue());
//    printf("device Temperature = %f\n", temperature);
//------------------------------------------------

	  Spinnaker::GenApi::CIntegerPtr ptrWidth = m_pCamera->Width.GetNode();
    int lptrWidth = ptrWidth->GetValue();
    std::cout << "imageWidth: "  << lptrWidth << std::endl ;

	  Spinnaker::GenApi::CIntegerPtr ptrHeight = m_pCamera->Height.GetNode();
    int lptrHeight = ptrHeight->GetValue();
    std::cout << "imageHeight: "  << lptrHeight << std::endl ;

	  Spinnaker::GenApi::CIntegerPtr ptrSensorWidth = m_pCamera->SensorWidth.GetNode();
    int lSensorWidth = ptrSensorWidth->GetValue();
    std::cout << "SensorWidth: "  << lSensorWidth << std::endl ;

	  Spinnaker::GenApi::CIntegerPtr ptrSensorHeight = m_pCamera->SensorHeight.GetNode();
    int lSensorHeight = ptrSensorHeight->GetValue();
    std::cout << "SensorHeight: "  << lSensorHeight << std::endl ;

    Spinnaker::GenApi::CEnumerationPtr ptrPixelFormat = m_pCamera->PixelFormat.GetNode();
    int64_t pixelFormat = ptrPixelFormat->GetIntValue();
    printf("pixelFormat: 0x%lx (0x01100007)\n", pixelFormat) ;

//    Spinnaker::GenApi::CIntegerPtr ptrIPAddress = m_pCamera->GetNodeMap().GetNode("GevDeviceIPAddress");
//    int64_t ipAddress = ptrIPAddress->GetValue();
//    std::cout << "ipAddress: " << ipAddress << std::endl ;
//    printf("ipAddress: 0x%\n", ipAddress) ;
//       CValuePtr pValue = nodeMapTLDevice.GetNode("GevDeviceIPAddress");
	// Manufacturer and model

	//m_ManufacturerEdit.SetWindowText(lManufacturerStr);
	//m_ModelEdit.SetWindowText(lModelStr);
	//m_NameEdit.SetWindowText(lNameStr);
//-------------------------------------------------------------------
	  Spinnaker::GenApi::CStringPtr ptrManufInfo = m_pCamera->DeviceManufacturerInfo.GetNode();
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
		    Spinnaker::GenApi::CEnumerationPtr ptrWindowing = m_pCamera->GetNodeMap().GetNode("IRWindowing");
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
		    Spinnaker::GenApi::CEnumerationPtr ptrDigOut = m_pCamera->GetNodeMap().GetNode("DigitalOutput");
		    Spinnaker::GenApi::CEnumerationPtr ptrCMOSBitDepth = m_pCamera->GetNodeMap().GetNode("CMOSBitDepth");
		    ptrDigOut->SetIntValue(3); // Enable
		    if (ptrCMOSBitDepth)
			      ptrCMOSBitDepth->SetIntValue(0); // 14 bit

		    {
			       Spinnaker::GenApi::CIntegerPtr ptrR = m_pCamera->GetNodeMap().GetNode("R");
			       Spinnaker::GenApi::CFloatPtr ptrB = m_pCamera->GetNodeMap().GetNode("B");
			       Spinnaker::GenApi::CFloatPtr ptrF = m_pCamera->GetNodeMap().GetNode("F");
			       Spinnaker::GenApi::CFloatPtr ptrO = m_pCamera->GetNodeMap().GetNode("O");

			       if (ptrR && ptrB && ptrF && ptrO)  {
				         m_R = (int)ptrR->GetValue();
				         m_B = ptrB->GetValue();
				         m_F = ptrF->GetValue();
				         m_O = ptrO->GetValue();
				         m_bMeasCapable = true;
			       }
		     }

		     Spinnaker::GenApi::CEnumerationPtr ptrTLUTMode = m_pCamera->GetNodeMap().GetNode("TemperatureLinearMode");
		     Spinnaker::GenApi::CIntegerPtr ptrOptions = m_pCamera->GetNodeMap().GetNode("CameraOptions");
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
		    Spinnaker::GenApi::CFloatPtr ptrEmissivity = m_pCamera->GetNodeMap().GetNode("ObjectEmissivity");
		    Spinnaker::GenApi::CFloatPtr ptrExtOptTransm = m_pCamera->GetNodeMap().GetNode("ExtOpticsTransmission");
		    Spinnaker::GenApi::CFloatPtr ptrRelHum = m_pCamera->GetNodeMap().GetNode("RelativeHumidity");
		    Spinnaker::GenApi::CFloatPtr ptrDist = m_pCamera->GetNodeMap().GetNode("ObjectDistance");
		    Spinnaker::GenApi::CFloatPtr ptrAmb = m_pCamera->GetNodeMap().GetNode("ReflectedTemperature");
		    Spinnaker::GenApi::CFloatPtr ptrAtm = m_pCamera->GetNodeMap().GetNode("AtmosphericTemperature");
		    Spinnaker::GenApi::CFloatPtr ptrEOT = m_pCamera->GetNodeMap().GetNode("ExtOpticsTemperature");

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
		    Spinnaker::GenApi::CFloatPtr ptrR = m_pCamera->GetNodeMap().GetNode("R");
		    Spinnaker::GenApi::CFloatPtr ptrB = m_pCamera->GetNodeMap().GetNode("B");
		    Spinnaker::GenApi::CFloatPtr ptrF = m_pCamera->GetNodeMap().GetNode("F");

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
		    Spinnaker::GenApi::CIntegerPtr ptrJ0 = m_pCamera->GetNodeMap().GetNode("J0");
		    Spinnaker::GenApi::CFloatPtr ptrJ1 = m_pCamera->GetNodeMap().GetNode("J1");
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
		    Spinnaker::GenApi::CFloatPtr ptrX = m_pCamera->GetNodeMap().GetNode("X");
		    Spinnaker::GenApi::CFloatPtr ptrAlpha1 = m_pCamera->GetNodeMap().GetNode("alpha1");
		    Spinnaker::GenApi::CFloatPtr ptrAlpha2 = m_pCamera->GetNodeMap().GetNode("alpha2");
		    Spinnaker::GenApi::CFloatPtr ptrBeta1 = m_pCamera->GetNodeMap().GetNode("beta1");
		    Spinnaker::GenApi::CFloatPtr ptrBeta2 = m_pCamera->GetNodeMap().GetNode("beta2");
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
		    Spinnaker::GenApi::CFloatPtr ptrEmissivity = m_pCamera->GetNodeMap().GetNode("ObjectEmissivity");
		    Spinnaker::GenApi::CFloatPtr ptrAmb = m_pCamera->GetNodeMap().GetNode("ReflectedTemperature");
		    Spinnaker::GenApi::CFloatPtr ptrAtm = m_pCamera->GetNodeMap().GetNode("AtmosphericTemperature");
		    Spinnaker::GenApi::CFloatPtr ptrEOT = m_pCamera->GetNodeMap().GetNode("WindowTemperature");
		    Spinnaker::GenApi::CFloatPtr ptrExtOptTransm = m_pCamera->GetNodeMap().GetNode("WindowTransmission");

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
			      Spinnaker::GenApi::CEnumerationPtr ptrIRFormat = m_pCamera->GetNodeMap().GetNode("IRFormat");
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
			      Spinnaker::GenApi::CEnumerationPtr ptrRange = m_pCamera->GetNodeMap().GetNode("SensorGainMode");
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

		    Spinnaker::GenApi::CEnumerationPtr ptrSpeed = m_pCamera->GetNodeMap().GetNode("SensorFrameRate");
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
				        Spinnaker::GenApi::CEnumerationPtr ptrRate = m_pCamera->GetNodeMap().GetNode("SensorVideoStandard");
				        lValue = 0;
				        if (ptrRate) lValue = ptrRate->GetIntValue() ;
//				m_ctrlFrameRate.SetCurSel(lValue == 4 ? 0 : 1);
                printf("SensorVideoStandard: %ld\n", lValue) ;
			      }
		    }
	  }

	  Spinnaker::GenApi::CIntegerPtr ptrPktSize = m_pCamera->GetNodeMap().GetNode("GevSCPSPacketSize");
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
//
//==================================================
void cFLIRHandler::capProperty( )
{

}
