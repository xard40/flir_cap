#pragma once
#include <opencv2/opencv.hpp>

//***************************************************************
//
//***************************************************************
struct color
{
    unsigned char rgbBlue;
    unsigned char rgbGreen;
    unsigned char rgbRed;
    color()
    {
        rgbBlue = rgbGreen = rgbRed = 0;
    }
};

//***************************************************************
//
//***************************************************************
struct sPalette
{
    enum palettetypes{
        Linear_red_palettes,
        GammaLog_red_palettes,
        Inversion_red_palette,
        Linear_palettes,
        GammaLog_palettes,
        Inversion_palette,
        False_color_palette1,
        False_color_palette2,
        False_color_palette3,
        False_color_palette4
    };
    color colors[256];
};

sPalette GetPalette(sPalette::palettetypes pal);
sPalette GetPalette(const std::string & pal);

void convertFalseColor(const cv::Mat& srcmat, cv::Mat& dstmat, const sPalette &pal, bool drawlegend = false, double mintemp = 0, double maxtemp = 0);
void convertFalseColor16(const cv::Mat& srcmat, cv::Mat& dstmat, const sPalette &pal, bool drawlegend = false, double mintemp = 0, double maxtemp = 0);

//***************************************************************
//
//***************************************************************
class converter_16_8
{
    enum
    {
        histminmembersperbucket = 10,
    };
  public:
    converter_16_8( );
    ~converter_16_8( );
    static converter_16_8& Instance()
    {
        if (!inst_)   {
            inst_ = new converter_16_8;
        }
        return *inst_;
    }
    double getMax();
    double getMin();
    void   convert_to8bit(const cv::Mat& img16, cv::Mat& img8, bool doTempConversion);
    void   convertTo16bit(const cv::Mat& srcImg, cv::Mat& destImg, bool doTempConversion) ;
    void   toneMapping(const cv::Mat& img16, cv::Mat& img8);
  private:
    double max_;
    double min_;
    static converter_16_8* inst_;
    bool firstframe_;
  // boost::shared_ptr<cv::Retina> retina_;
};
