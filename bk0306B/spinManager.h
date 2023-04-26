#pragma once
#include <string>
#include <vector>
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
class cSpinManager
{
  public:
    cSpinManager() ;
    ~cSpinManager() ;
    static cSpinManager& the_manager() ;
    CameraPtr   get_camera(const std::string& serial_number) const ;
    CameraPtr   get_camera(int index) const ;
    void 				Exit( ) ;
  private:
    SystemPtr       pSpinSystem;
    CameraList      mCameraList;
    std::map<std::string, CameraPtr>      mmCameraPtr ;   // key: url , value: capHandler
};

/********************************************************************
*
*********************************************************************/
struct image_t
{
  cv::Mat     img;
  int         idx;
  uint64_t    sys_ts, img_ts;
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
