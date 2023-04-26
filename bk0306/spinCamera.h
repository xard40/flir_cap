#pragma once
#include <string>
#include <opencv2/opencv.hpp>
#include <exception>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <chrono>
#include <sys/stat.h>
//--------------------------------------------
#include "Spinnaker.h"
#include "SpinGenApi/SpinnakerGenApi.h"
using namespace Spinnaker;
using namespace Spinnaker::GenApi;
using namespace Spinnaker::GenICam;

/********************************************************************
*
*********************************************************************/
class cCamManager
{
  public:
    cCamManager() ;
    ~cCamManager() ;
    static cCamManager& the_manager() ;
    CameraPtr get_camera(const std::string& serial_number) const ;
    CameraPtr get_camera(int index) const ;
  private:
    SystemPtr   pSystemPtr;
    CameraList  mCameraList;
};

/********************************************************************
*
*********************************************************************/
class cSpinCamera
{
  public:
    cSpinCamera(std::string serialNumber) ;
    cSpinCamera(int index) ;
    ~cSpinCamera() ;
  // Print Device Info
    void    print_device_info() ;
  // Start/End camera
    void    start() ;
    void    end() ;
  // Setting Hardware/Software Trigger
    void    enable_hardware_trigger() ;
    void    enable_software_trigger() ;
    void    disable_trigger() ;
  // Setting Black Level
    void    enable_black_level_auto() ;
    void    disable_black_level_auto() ;
    void    set_black_level(double black_level) ;
    double  get_black_level() const ;
  // Setting Frame Rate
    void    enable_frame_rate_auto() ;
    void    disable_frame_rate_auto() ;
    void    set_frame_rate(double frame_rate) ;
    double  get_frame_rate() const ;
  // Setting Exposure Time, us
    void    enable_exposure_auto() ;
    void    disable_exposure_auto() ;
    void    set_exposure_time(double exposure_time) ;
    double  get_exposure_time() const ;
    void    set_exposure_upperbound(double value) ;

  // Setting Gain
    void    enable_gain_auto() ;
    void    disable_gain_auto() ;
    void    set_gain(double gain) ;
    double  get_gain() const ;
  // Setting Gamma
    void    set_gamma(double gamma) ;
    double  get_gamme() const ;
  // Setting White Balance
    void    enable_white_balance_auto() ;
    void    disable_white_balance_auto() ;
    void    set_white_balance_blue(double value) ;
    void    set_white_balance_red(double value) ;
    double  get_white_balance_blue() const ;
    double  get_white_balance_red() const ;

  // Get Image Timestamp
    uint64_t get_system_timestamp() const ;
    uint64_t get_image_timestamp() const ;
  // Grab Next Image
    cv::Mat  grab_next_image(const std::string& format = "bgr") ;
    void     trigger_software_execute() ;
  private:
    CameraPtr       pCameraPtr;
    INodeMap*       pNodeMap;
    ImageProcessor 	mImageProcessor ;
    uint64_t        mSystemTimestamp;
    uint64_t        mimageTimestamp;
};

/********************************************************************
*
*********************************************************************/
struct image_t
{
    cv::Mat img;
    int idx;
    uint64_t sys_ts, img_ts;
    std::string prefix;
    image_t() noexcept {}
    image_t(const cv::Mat& _img, int _idx, uint64_t _sys_ts, uint64_t _img_ts, const std::string& _prefix) noexcept:
        img(_img), idx(_idx), sys_ts(_sys_ts), img_ts(_img_ts), prefix(_prefix) {}
    image_t(const image_t& rhs) noexcept:
        img(rhs.img), idx(rhs.idx), sys_ts(rhs.sys_ts), img_ts(rhs.img_ts), prefix(rhs.prefix) {}
    image_t(image_t&& rhs) noexcept:
        img(std::move(rhs.img)), idx(std::move(idx)), sys_ts(std::move(rhs.sys_ts)), img_ts(std::move(rhs.img_ts)), prefix(std::move(rhs.prefix)) {}
    image_t& operator=(image_t&& rhs) noexcept {
        img = std::move(rhs.img);
        idx = std::move(rhs.idx);
        sys_ts = std::move(rhs.sys_ts);
        img_ts = std::move(rhs.img_ts);
        prefix = std::move(rhs.prefix);
        return *this;
    }
    void save() const noexcept {
        std::ostringstream filename;
        filename << prefix << (sys_ts / (60 * 1000000)) << '/';
        mkdir(filename.str().c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
        filename << std::setfill('0') << std::setw(8) << idx << '-';
        filename << sys_ts << '-' << img_ts << ".jpg";
        cv::imwrite(filename.str().c_str(), img);
    }
};
