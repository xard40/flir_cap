#include <cmath>
#include <map>
#include <vector>
#include <iostream>

#include "flirUtils.h"

typedef struct {
	unsigned char  r ;
	unsigned char  g ;
	unsigned char  b ;
} sRGB ;

static sRGB palatteIron[128] = {
				{0,   0,    0},
				{0,   0,    0},
				{0,   0,   36},
				{0,   0,   51},
				{0,   0,   66},
				{0,   0,   81},
				{2,   0,   90},
				{4,   0,   99},
				{7,   0,  106},
				{11,   0, 115},
				{14,   0, 119},
				{20,   0, 123},
				{27,   0, 128},
				{33,   0, 133},
				{41,   0, 137},
				{48,   0, 140},
				{55,   0, 143},
				{61,   0, 146},
				{66,   0, 149},
				{72,   0, 150},
				{78,   0, 151},
				{84,   0, 152},
				{91,   0, 153},
				{97,   0, 155},
				{104,   0, 155},
				{110,   0, 156},
				{115,   0, 157},
				{122,   0, 157},
				{128,   0, 157},
				{134,   0, 157},
				{139,   0, 157},
				{146,   0, 156},
				{152,   0, 155},
				{157,   0, 155},
				{162,   0, 155},
				{167,   0, 154},
				{171,   0, 153},
				{175,   1, 152},
				{178,   1, 151},
				{182,   2, 149},
				{185,   4, 149},
				{188,   5, 147},
				{191,   6, 146},
				{193,   8, 144},
				{195,  11, 142},
				{198,  13, 139},
				{201,  17, 135},
				{203,  20, 132},
				{206,  23, 127},
				{208,  26, 121},
				{210,  29, 116},
				{212,  33, 111},
				{214,  37, 103},
				{217,  41,  97},
				{219,  46,  89},
				{221,  49,  78},
				{223,  53,  66},
				{224,  56,  54},
				{226,  60,  42},
				{228,  64,  30},
				{229,  68,  25},
				{231,  72,  20},
				{232,  76,  16},
				{234,  78,  12},
				{235,  82,  10},
				{236,  86,   8},
				{237,  90,   7},
				{238,  93,   5},
				{239,  96,   4},
				{240, 100,   3},
				{241, 103,   3},
				{241, 106,   2},
				{242, 109,   1},
				{243, 113,   1},
				{244, 116,   0},
				{244, 120,   0},
				{245, 125,   0},
				{246, 129,   0},
				{247, 133,   0},
				{248, 136,   0},
				{248, 139,   0},
				{249, 142,   0},
				{249, 145,   0},
				{250, 149,   0},
				{251, 154,   0},
				{252, 159,   0},
				{253, 163,   0},
				{253, 168,   0},
				{253, 172,   0},
				{254, 176,   0},
				{254, 179,   0},
				{254, 184,   0},
				{254, 187,   0},
				{254, 191,   0},
				{254, 195,   0},
				{254, 199,   0},
				{254, 202,   1},
				{254, 205,   2},
				{254, 208,   5},
				{254, 212,   9},
				{254, 216,  12},
				{255, 219,  15},
				{255, 221,  23},
				{255, 224,  32},
				{255, 227,  39},
				{255, 229,  50},
				{255, 232,  63},
				{255, 235,  75},
				{255, 238,  88},
				{255, 239, 102},
				{255, 241, 116},
				{255, 242, 134},
				{255, 244, 149},
				{255, 245, 164},
				{255, 247, 179},
				{255, 248, 192},
				{255, 249, 203},
				{255, 251, 216},
				{255, 253, 228},
				{255, 254, 239},
				{255, 255, 249},
				{255, 255, 249},
				{255, 255, 249},
				{255, 255, 249},
				{255, 255, 249},
				{255, 255, 249},
				{255, 255, 249},
				{255, 255, 249}
};

static sRGB palatteRain[128] =  {
			{0,   0,   0},
			{0,   0,   0},
			{15,   0,  15},
			{31,   0,  31},
			{47,   0,  47},
			{63,   0,  63},
			{79,   0,  79},
			{95,   0,  95},
			{111,   0, 111},
			{127,   0, 127},
			{143,   0, 143},
			{159,   0, 159},
			{175,   0, 175},
			{191,   0, 191},
			{207,   0, 207},
			{223,   0, 223},
			{239,   0, 239},
			{255,   0, 255},
			{239,   0, 250},
			{223,   0, 245},
			{207,   0, 240},
			{191,   0, 236},
			{175,   0, 231},
			{159,   0, 226},
			{143,   0, 222},
			{127,   0, 217},
			{111,   0, 212},
			{95,   0, 208},
			{79,   0, 203},
			{63,   0, 198},
			{47,   0, 194},
			{31,   0, 189},
			{15,   0, 184},
			{0,   0, 180},
			{0,  15, 184},
			{0,  31, 189},
			{0,  47, 194},
			{0,  63, 198},
			{0,  79, 203},
			{0,  95, 208},
			{0, 111, 212},
			{0, 127, 217},
			{0, 143, 222},
			{0, 159, 226},
			{0, 175, 231},
			{0, 191, 236},
			{0, 207, 240},
			{0, 223, 245},
			{0, 239, 250},
			{0, 255, 255},
			{0, 245, 239},
			{0, 236, 223},
			{0, 227, 207},
			{0, 218, 191},
			{0, 209, 175},
			{0, 200, 159},
			{0, 191, 143},
			{0, 182, 127},
			{0, 173, 111},
			{0, 164,  95},
			{0, 155,  79},
			{0, 146,  63},
			{0, 137,  47},
			{0, 128,  31},
			{0, 119,  15},
			{0, 110,   0},
			{15, 118,   0},
			{30, 127,   0},
			{45, 135,   0},
			{60, 144,   0},
			{75, 152,   0},
			{90, 161,   0},
			{105, 169,  0},
			{120, 178,  0},
			{135, 186,  0},
			{150, 195,  0},
			{165, 203,  0},
			{180, 212,  0},
			{195, 220,  0},
			{210, 229,  0},
			{225, 237,  0},
			{240, 246,  0},
			{255, 255,  0},
			{251, 240,  0},
			{248, 225,  0},
			{245, 210,  0},
			{242, 195,  0},
			{238, 180,  0},
			{235, 165,  0},
			{232, 150,  0},
			{229, 135,  0},
			{225, 120,  0},
			{222, 105,  0},
			{219,  90,  0},
			{216,  75,  0},
			{212,  60,  0},
			{209,  45,  0},
			{206,  30,  0},
			{203,  15,  0},
			{200,   0,  0},
			{202,  11,  11},
			{205,  23,  23},
			{207,  34,  34},
			{210,  46,  46},
			{212,  57,  57},
			{215,  69,  69},
			{217,  81,  81},
			{220,  92,  92},
			{222, 104, 104},
			{225, 115, 115},
			{227, 127, 127},
			{230, 139, 139},
			{232, 150, 150},
			{235, 162, 162},
			{237, 173, 173},
			{240, 185, 185},
			{242, 197, 197},
			{245, 208, 208},
			{247, 220, 220},
			{250, 231, 231},
			{252, 243, 243},
			{252, 243, 243},
			{252, 243, 243},
			{252, 243, 243},
			{252, 243, 243},
			{252, 243, 243},
			{252, 243, 243},
			{252, 243, 243}
};

//***************************************************************
//
//***************************************************************
#define DEG2RAD 0.01745329
sPalette GetPalette(sPalette::palettetypes pal)
{
  sPalette ret;
  int i, r, g, b;
  float f;

    switch (pal)
    {
        case sPalette::Linear_red_palettes :
        /*
        * Linear red palettes.
        */
            for (i = 0; i < 256; i++)    {
                ret.colors[i].rgbBlue = 0;
                ret.colors[i].rgbGreen = 0;
                ret.colors[i].rgbRed = i;
            }
          break;
        case sPalette::GammaLog_red_palettes:
        /*
        * GammaLog red palettes.
        */
            for (i = 0; i < 256; i++)    {
                f = log10(pow((i / 255.0), 1.0) * 9.0 + 1.0) * 255.0;
                ret.colors[i].rgbBlue = 0;
                ret.colors[i].rgbGreen = 0;
                ret.colors[i].rgbRed = f;
            }
          break;
        case sPalette::Inversion_red_palette:
        /*
        * Inversion red palette.
        */
            for (i = 0; i < 256; i++)    {
                ret.colors[i].rgbBlue = 0;
                ret.colors[i].rgbGreen = 0;
                ret.colors[i].rgbRed = 255 - i;
            }
          break;
        case sPalette::Linear_palettes:
        /*
        * Linear palettes.
        */
            for (i = 0; i < 256; i++)    {
                ret.colors[i].rgbBlue = ret.colors[i].rgbGreen = ret.colors[i].rgbRed = i;
            }
          break;
        case sPalette::GammaLog_palettes:
        /*
        * GammaLog palettes.
        */
            for (i = 0; i < 256; i++)   {
                f = log10(pow((i / 255.0), 1.0) * 9.0 + 1.0) * 255.0;
                ret.colors[i].rgbBlue = ret.colors[i].rgbGreen = ret.colors[i].rgbRed = f;
            }
          break;
        case sPalette::Inversion_palette:
        /*
        * Inversion palette.
        */
            for (i = 0; i < 256; i++)   {
                ret.colors[i].rgbBlue = ret.colors[i].rgbGreen = ret.colors[i].rgbRed = 255 - i;
            }
          break;
        case sPalette::False_color_palette1:
        /*
        * False color palette #1.
        */
            for (i = 0; i < 256; i++)   {
                r = (sin((i / 255.0 * 360.0 - 120.0 > 0 ? i / 255.0 * 360.0 - 120.0 : 0) * DEG2RAD) * 0.5 + 0.5) * 255.0;
                g = (sin((i / 255.0 * 360.0 + 60.0) * DEG2RAD) * 0.5 + 0.5) * 255.0;
                b = (sin((i / 255.0 * 360.0 + 140.0) * DEG2RAD) * 0.5 + 0.5) * 255.0;
                ret.colors[i].rgbBlue = b;
                ret.colors[i].rgbGreen = g;
                ret.colors[i].rgbRed = r;
            }
          break;
        case sPalette::False_color_palette2:
        /*
        * False color palette #2.
        */
            for (i = 0; i < 256; i++)   {
                r = (sin((i / 255.0 * 360.0 + 120.0) * DEG2RAD) * 0.5 + 0.5) * 255.0;
                g = (sin((i / 255.0 * 360.0 + 240.0) * DEG2RAD) * 0.5 + 0.5) * 255.0;
                b = (sin((i / 255.0 * 360.0 + 0.0) * DEG2RAD) * 0.5 + 0.5) * 255.0;
                ret.colors[i].rgbBlue = b;
                ret.colors[i].rgbGreen = g;
                ret.colors[i].rgbRed = r;
            }
          break;
        case sPalette::False_color_palette3:
        /*
        * False color palette #3.
        */
            for (i = 0; i < 256; i++)    {
                r = (sin((i / 255.0 * 360.0 + 240.0) * DEG2RAD) * 0.5 + 0.5) * 255.0;
                g = (sin((i / 255.0 * 360.0 + 0.0) * DEG2RAD) * 0.5 + 0.5) * 255.0;
                b = (sin((i / 255.0 * 360.0 + 120.0) * DEG2RAD) * 0.5 + 0.5) * 255.0;
                ret.colors[i].rgbBlue = b;
                ret.colors[i].rgbGreen = g;
                ret.colors[i].rgbRed = r;
            }
          break;

        case sPalette::False_color_palette4:
        /*
        * False color palette #4. Matlab JET
        */
            enum
            {
                nsep = 64, nvals = 192, n = 256
            };

            std::vector<double> vals;
            vals.resize(nvals, 0);

            int idx = 0;
            for (int i = 0; i < nsep; ++i)   {
                vals.at(idx++) = (i / (double)nsep);
            }

            for (int i = 0; i < nsep; ++i) {
                vals.at(idx + i) = 1.;
            }

            idx += nsep;
            for (int i = nsep - 1; i >= 0; --i)   {
                vals.at(idx++) = i / (double)nsep;
            }

            std::vector<int> r;
            r.resize(nvals);
            std::vector<int> g;
            g.resize(nvals);
            std::vector<int> b;
            b.resize(nvals);
            for (std::size_t i = 0; i < nvals; ++i)   {
                g.at(i) = ceil(nsep / 2) - 1 + i;
                r.at(i) = g.at(i) + nsep;
                b.at(i) = g.at(i) - nsep;
            }

            int idxr = 0;
            int idxg = 0;

            for (int i = 0; i < nvals; ++i)   {
                if (r.at(i) >= 0 && r.at(i) < n)
                    ret.colors[r.at(i)].rgbRed = vals.at(idxr++) * 255.;

            if (g.at(i) >= 0 && g.at(i) < n)
                ret.colors[g.at(i)].rgbGreen = vals.at(idxg++) * 255.;
            }

            int idxb = 0;
            int cntblue = 0;
            for (int i = 0; i < nvals; ++i)   {
                if (b.at(i) >= 0 && b.at(i) < n)
                    cntblue++;
            }

            for (int i = 0; i < nvals; ++i)  {
                if (b.at(i) >= 0 && b.at(i) < n)
                    ret.colors[b.at(i)].rgbBlue = vals.at(nvals - 1 - cntblue + idxb++) * 255.;
            }
          break;
      }
      return ret;
}

//***************************************************************
//
//***************************************************************
sPalette GetPalette(const std::string &pal_choice)
{
  sPalette ret;
  int i, r, g, b;
  float f;

    if (pal_choice == "Linear_red_palettes")  {
    /*
      * Linear red palettes.
      */
        for (i = 0; i < 256; i++)   {
            ret.colors[i].rgbBlue = 0;
            ret.colors[i].rgbGreen = 0;
            ret.colors[i].rgbRed = i;
        }
    }
    else if (pal_choice == "GammaLog_red_palettes")  {
    /*
      * GammaLog red palettes.
      */
        for (i = 0; i < 256; i++)    {
            f = log10(pow((i / 255.0), 1.0) * 9.0 + 1.0) * 255.0;
            ret.colors[i].rgbBlue = 0;
            ret.colors[i].rgbGreen = 0;
            ret.colors[i].rgbRed = f;
        }
    }
    else if(pal_choice == "Inversion_red_palette")  {
    /*
      * Inversion red palette.
      */
        for (i = 0; i < 256; i++)    {
            ret.colors[i].rgbBlue = 0;
            ret.colors[i].rgbGreen = 0;
            ret.colors[i].rgbRed = 255 - i;
        }
    }
    else if(pal_choice == "Linear_palettes")  {
    /*
      * Linear palettes.
      */
        for (i = 0; i < 256; i++)   {
            ret.colors[i].rgbBlue = ret.colors[i].rgbGreen = ret.colors[i].rgbRed = i;
        }
    }
    else if(pal_choice == "GammaLog_palettes")  {
    /*
      * GammaLog palettes.
      */
        for (i = 0; i < 256; i++)    {
            f = log10(pow((i / 255.0), 1.0) * 9.0 + 1.0) * 255.0;
            ret.colors[i].rgbBlue = ret.colors[i].rgbGreen = ret.colors[i].rgbRed = f;
        }
    }
    else if(pal_choice == "Inversion_palette")  {
    /*
      * Inversion palette.
      */
        for (i = 0; i < 256; i++)  {
            ret.colors[i].rgbBlue = ret.colors[i].rgbGreen = ret.colors[i].rgbRed = 255 - i;
        }
    }
    else if(pal_choice == "False_color_palette1")  {
    /*
      * False color palette #1.
      */
        for (i = 0; i < 256; i++)   {
            r = (sin((i / 255.0 * 360.0 - 120.0 > 0 ? i / 255.0 * 360.0 - 120.0 : 0) * DEG2RAD) * 0.5 + 0.5) * 255.0;
            g = (sin((i / 255.0 * 360.0 + 60.0) * DEG2RAD) * 0.5 + 0.5) * 255.0;
            b = (sin((i / 255.0 * 360.0 + 140.0) * DEG2RAD) * 0.5 + 0.5) * 255.0;
            ret.colors[i].rgbBlue = b;
            ret.colors[i].rgbGreen = g;
            ret.colors[i].rgbRed = r;
        }
    }
    else if(pal_choice == "False_color_palette2")  {
    /*
      * False color palette #2.
      */
        for (i = 0; i < 256; i++)   {
            r = (sin((i / 255.0 * 360.0 + 120.0) * DEG2RAD) * 0.5 + 0.5) * 255.0;
            g = (sin((i / 255.0 * 360.0 + 240.0) * DEG2RAD) * 0.5 + 0.5) * 255.0;
            b = (sin((i / 255.0 * 360.0 + 0.0) * DEG2RAD) * 0.5 + 0.5) * 255.0;
            ret.colors[i].rgbBlue = b;
            ret.colors[i].rgbGreen = g;
            ret.colors[i].rgbRed = r;
        }
    }
    else if(pal_choice == "False_color_palette3")  {
    /*
      * False color palette #3.
      */
        for (i = 0; i < 256; i++)   {
            r = (sin((i / 255.0 * 360.0 + 240.0) * DEG2RAD) * 0.5 + 0.5) * 255.0;
            g = (sin((i / 255.0 * 360.0 + 0.0) * DEG2RAD) * 0.5 + 0.5) * 255.0;
            b = (sin((i / 255.0 * 360.0 + 120.0) * DEG2RAD) * 0.5 + 0.5) * 255.0;
            ret.colors[i].rgbBlue = b;
            ret.colors[i].rgbGreen = g;
            ret.colors[i].rgbRed = r;
        }
    }
    else if (pal_choice == "False_color_palette4")  {
      /*
       * False color palette #4. Matlab JET
       */
        enum
        {
            nsep = 64,
            nvals = 192,
            n = 256
        };

        std::vector<double> vals;
        vals.resize(nvals, 0);

        int idx = 0;
        for (int i = 0; i < nsep; ++i)  {
            vals.at(idx++) = (i / (double)nsep);
        }

        for (int i = 0; i < nsep; ++i)  {
            vals.at(idx + i) = 1.;
        }

        idx += nsep;
        for (int i = nsep - 1; i >= 0; --i)  {
            vals.at(idx++) = i / (double)nsep;
        }

        std::vector<int> r;
        r.resize(nvals);
        std::vector<int> g;
        g.resize(nvals);
        std::vector<int> b;
        b.resize(nvals);
        for (std::size_t i = 0; i < nvals; ++i)  {
            g.at(i) = ceil(nsep / 2) - 1 + i;
            r.at(i) = g.at(i) + nsep;
            b.at(i) = g.at(i) - nsep;
        }

        int idxr = 0;
        int idxg = 0;

        for (int i = 0; i < nvals; ++i)  {
            if (r.at(i) >= 0 && r.at(i) < n)
                ret.colors[r.at(i)].rgbRed = vals.at(idxr++) * 255.;
            if (g.at(i) >= 0 && g.at(i) < n)
                ret.colors[g.at(i)].rgbGreen = vals.at(idxg++) * 255.;
        }

        int idxb = 0;
        int cntblue = 0;
        for (int i = 0; i < nvals; ++i)  {
            if (b.at(i) >= 0 && b.at(i) < n)
                cntblue++;
        }

        for (int i = 0; i < nvals; ++i)   {
            if (b.at(i) >= 0 && b.at(i) < n)
                ret.colors[b.at(i)].rgbBlue = vals.at(nvals - 1 - cntblue + idxb++) * 255.;
        }
    }
		else  if (pal_choice == "False_color_palette5")  {
			/*
	      * False color palette #5.
	      */
	        for (i = 0; i < 256; i++)   {
	            r = palatteIron[i/2].r ;
	            g = palatteIron[i/2].g ;
	            b = palatteIron[i/2].b ;
	            ret.colors[i].rgbBlue = b;
	            ret.colors[i].rgbGreen = g;
	            ret.colors[i].rgbRed = r;
	        }
		}
		else  if (pal_choice == "False_color_palette6")  {
			/*
	      * False color palette #5.
	      */
	        for (i = 0; i < 256; i++)   {
	            r = palatteRain[i/2].r ;
	            g = palatteRain[i/2].g ;
	            b = palatteRain[i/2].b ;
	            ret.colors[i].rgbBlue = b;
	            ret.colors[i].rgbGreen = g;
	            ret.colors[i].rgbRed = r;
	        }
		}
    return ret;
}
#undef DEG2RAD

//***************************************************************
//
//***************************************************************
void convertFalseColor(const cv::Mat& srcFrame, cv::Mat& dstFrame, const sPalette &pal, bool drawlegend, double mintemp, double maxtemp)
{
    dstFrame.create(srcFrame.rows, srcFrame.cols, CV_8UC3);

    cv::Size srcSize = srcFrame.size();
    const unsigned char* src = srcFrame.data;
    unsigned char* dst = dstFrame.data;


    if (srcFrame.isContinuous() && dstFrame.isContinuous()) {
        srcSize.width *= srcSize.height;
        srcSize.height = 1;
    }

    for (int i = 0; i < srcSize.width; ++i)  {
        for (int j = 0; j < srcSize.height; ++j)   {
            int idx = j * srcSize.width + i;
            uint8_t val = src[idx] ;
            dst[idx * dstFrame.channels() + 0] = pal.colors[val].rgbBlue;
            dst[idx * dstFrame.channels() + 1] = pal.colors[val].rgbGreen;
            dst[idx * dstFrame.channels() + 2] = pal.colors[val].rgbRed;
        }
    }
  //draw a legend if true Temperatures
    if (drawlegend) {

    //get min max to scale the legend
        double max_val;
        double min_val;
        cv::minMaxIdx(srcFrame, &min_val, &max_val);

        enum {
            legenddiscretization = 2,
            legendnumbers = 2,
            legendwidth = 12,
            x_0 = 2,
            y_0 =  10,
        };
        double stepsize;

// std::cout<<"mintemp: "<<mintemp<<", maxtemp: "<<maxtemp<<std::endl;
// draw legend color bar
        for (int i = y_0 ; i < dstFrame.rows - y_0 ; ++i) {
            int py = dstFrame.rows - i;
            int val = (i - y_0) / (double)(dstFrame.rows - y_0 * 2) * 255.;
            cv::rectangle(dstFrame, cv::Point(x_0, py), cv::Point(x_0 + legendwidth, py + 1),
                   CV_RGB(pal.colors[val].rgbRed, pal.colors[val].rgbGreen, pal.colors[val].rgbBlue ), -1);
        }
// draw temp tick labels
        stepsize = (dstFrame.rows - y_0 * 2) / (double)legendnumbers;
        for (int i = 0 ; i <= legendnumbers ; ++i) {
            int py = y_0 + (legendnumbers - i) * stepsize + 5; //bottom up
            double tempval = (mintemp - 273.15) + i * (maxtemp - mintemp) / (double)legendnumbers;
//====================================================
						char dString[16] ;
						sprintf(dString, "%.1f C", tempval) ;
						cv::putText(dstFrame, dString, cv::Point(x_0 + 10, py),
                        cv::FONT_HERSHEY_SIMPLEX, 0.5, CV_RGB(255, 255, 255), 2);
//-------------------------------------------------------------
//            std::stringstream ss;
//            ss << std::setprecision(2) << tempval << " C";
//            cv::putText(dstFrame, ss.str(), cv::Point(x_0 + 12, py),
//                        cv::FONT_HERSHEY_SIMPLEX, 0.5, CV_RGB(255, 255, 255), 1);
//==================================================================
        }

    //draw ticks into legends
        stepsize = (dstFrame.rows - y_0 * 2) / (double)legenddiscretization;
        for (int i = 0 ; i <= legenddiscretization ; ++i) {
            int py = y_0 + (legenddiscretization - i) * stepsize; //bottom up
            cv::line(dstFrame, cv::Point(x_0 - 2, py), cv::Point(x_0 + legendwidth + 2, py),
                     CV_RGB(255, 255, 255), 1);
        }
    }
}


//***************************************************************
//
//***************************************************************
void convertFalseColor16(const cv::Mat& srcFrame, cv::Mat& dstFrame, const sPalette &pal, bool drawlegend, double mintemp, double maxtemp)
{
	  if (dstFrame.empty())  {
    		dstFrame.create(srcFrame.rows, srcFrame.cols, CV_8UC3);
		}

    cv::Size srcSize = srcFrame.size();
    unsigned char* src = srcFrame.data;
//    uint16_t* src = static_cast<uint16_t *>(srcFrame.data);
    unsigned char* dst = dstFrame.data;


//    if (srcFrame.isContinuous() && dstFrame.isContinuous()) {
//        srcSize.width *= srcSize.height;
//        srcSize.height = 1;
//    }

    for (int row = 0; row < srcFrame.rows; ++row)  {
        for (int col = 0; col < srcFrame.cols; ++col)   {
            int idx = row * srcFrame.cols + col ;
						uint16_t* pVal = (uint16_t *)  src + idx ;
            uint8_t val = *pVal / 255 ;
//            uint8_t val = src[idx] ;
            dst[idx * dstFrame.channels() + 0] = pal.colors[val].rgbBlue;
            dst[idx * dstFrame.channels() + 1] = pal.colors[val].rgbGreen;
            dst[idx * dstFrame.channels() + 2] = pal.colors[val].rgbRed;
        }
    }
  //draw a legend if true Temperatures
    if (drawlegend) {

    //get min max to scale the legend
        double max_val;
        double min_val;
        cv::minMaxIdx(srcFrame, &min_val, &max_val);

        enum {
            legenddiscretization = 2,
            legendnumbers = 1,
            legendwidth = 12,
            x_0 = 2,
            y_0 =  10,
        };
        double stepsize;

// std::cout<<"mintemp: "<<mintemp<<", maxtemp: "<<maxtemp<<std::endl;
// draw legend color bar
        for (int i = y_0 ; i < dstFrame.rows - y_0 ; ++i) {
            int py = dstFrame.rows - i;
            int val = (i - y_0) / (double)(dstFrame.rows - y_0 * 2) * 255.;
            cv::rectangle(dstFrame, cv::Point(x_0, py), cv::Point(x_0 + legendwidth, py + 1),
                   CV_RGB(pal.colors[val].rgbRed, pal.colors[val].rgbGreen, pal.colors[val].rgbBlue ), -1);
        }
// draw temp tick labels
        stepsize = (dstFrame.rows - y_0 * 2) / (double)legendnumbers;
        for (int i = 0 ; i <= legendnumbers ; ++i) {
            int py = y_0 + (legendnumbers - i) * stepsize + 5; //bottom up
            double tempval = (mintemp - 273.15) + i * (maxtemp - mintemp) / (double)legendnumbers;
//====================================================
						char dString[16] ;
						sprintf(dString, "%.1f C", tempval) ;
						cv::putText(dstFrame, dString, cv::Point(x_0 + 10, py),
                        cv::FONT_HERSHEY_SIMPLEX, 0.5, CV_RGB(255, 255, 255), 2);
//-------------------------------------------------------------
//            std::stringstream ss;
//            ss << std::setprecision(2) << tempval << " C";
//            cv::putText(dstFrame, ss.str(), cv::Point(x_0 + 12, py),
//                        cv::FONT_HERSHEY_SIMPLEX, 0.5, CV_RGB(255, 255, 255), 1);
//==================================================================
        }

    //draw ticks into legends
        stepsize = (dstFrame.rows - y_0 * 2) / (double)legenddiscretization;
        for (int i = 0 ; i <= legenddiscretization ; ++i) {
            int py = y_0 + (legenddiscretization - i) * stepsize; //bottom up
            cv::line(dstFrame, cv::Point(x_0 - 2, py), cv::Point(x_0 + legendwidth + 2, py),
                     CV_RGB(255, 255, 255), 1);
        }
    }
}

//***************************************************************
//
//***************************************************************
converter_16_8* converter_16_8::inst_ = NULL;

//***************************************************************
//
//***************************************************************
converter_16_8::converter_16_8()
{
    min_ = std::numeric_limits<uint16_t>::max();
    max_ = 0;
    firstframe_ = true;
}

//***************************************************************
//
//***************************************************************
converter_16_8::~converter_16_8()
{
    delete inst_;
    inst_ = NULL;
}

//***************************************************************
//
//***************************************************************
double converter_16_8::getMin()
{
    return min_;
}

//***************************************************************
//
//***************************************************************
double converter_16_8::getMax()
{
    return max_;
}
// void converter_16_8::toneMapping(const cv::Mat& img16, cv::Mat& img8){
//   if(!retina_){
//     retina_.reset(new cv::Retina(img16.size(), false));
//     retina_->setup(ros::package::getPath("brisk") + "/include/flir/retina_params");
//   }
//   retina_->run(img16);
//   retina_->getParvo(img8);
// }
//***************************************************************
//
//***************************************************************
//adjustment -3 for slightly wrong fit
double powerToK4(double power)
{
    double slope = 2.58357167114001779457e-07;
    double y_0 = 2.26799217314804718626e+03;
    return sqrt(sqrt(((double)power - y_0) / slope)) - 3;
//============================================================
//    uint16_t rawValue = power ;
//		float leptonCalSlope = ;
//		float temp = (leptonCalSlope * rawValue) - 273.15;
			//Convert to Fahrenheit if needed
//			if (tempFormat == tempFormat_fahrenheit)
//				temp = celciusToFahrenheit(temp);
//			return temp;
//-----------------------------------------------
//# Create 5 calculated values using Thermimage raw2temp() function:
//trealc<-raw2temp(r1, E=0.96, OD=1, RTemp=20, ATemp=20, IRT=1, RH=50,PR1 = 21106.77, PB = 1501, PF = 1, PO = -7340, PR2 = 0.012545258)
//tt0.9c<-raw2temp(r1, E=0.96, OD=1, RTemp=20, ATemp=20, IRT=0.9, RH=50, PR1 = 21106.77, PB = 1501, PF = 1, PO = -7340, PR2 = 0.012545258)
//tt0.7c<-raw2temp(r1, E=0.96, OD=1, RTemp=20, ATemp=20, IRT=0.7, RH=50, PR1 = 21106.77, PB = 1501, PF = 1, PO = -7340, PR2 = 0.012545258)
//trt40c<-raw2temp(r1, E=0.96, OD=1, RTemp=40, ATemp=40, IRWTemp=40, IRT=1, RH=50, PR1 = 21106.77, PB = 1501, PF = 1, PO = -7340, PR2 = 0.012545258)
//te0.9c<-raw2temp(r1, E=0.9, OD=1, RTemp=20, ATemp=20, IRT=1, RH=50,PR1 = 21106.77, PB = 1501, PF = 1, PO = -7340, PR2 = 0.012545258)
//==========================================
}

//***************************************************************
//
//***************************************************************
void converter_16_8::convert_to8bit(const cv::Mat& img16, cv::Mat& img8, bool doTempConversion)
{
    if (img8.empty())  { //make an image if the user has provided nothing
        img8.create(cv::Size(img16.cols, img16.rows), CV_8UC1) ;
    }
    double min = std::numeric_limits<uint16_t>::max();
    double max = 0;
  //make a histogram of intensities
    typedef std::map<double, int> hist_t;
    hist_t hist;
    double bucketwidth = 2.; //bucketwidth in degrees K

    for (int i = 0; i < img16.cols; ++i)  {
        for (int j = 0; j < img16.rows; ++j)   {
            double power = img16.at<uint16_t>(j, i);
            double temp;
            if (doTempConversion) {
                temp = powerToK4(power);
            } else {
                temp = power;
            }
            temp = round(temp / bucketwidth) * bucketwidth;
            hist[temp]++;
        }
    }

  //find the main section of the histogram
    for (hist_t::const_iterator it = hist.begin(); it != hist.end(); ++it)  {
        if (it->second > histminmembersperbucket)  {
            if (it->first > max)
                max = it->first ;
            if (it->first < min)
                min = it->first ;
        }
    }

    if (firstframe_)  {
        min_ = min;
        max_ = max;
    }
  //  std::cout<<"min: "<<min-273.15<<" max: "<<max-273.15<<" sm: min: "<<min_-273.15<<" max: "<<max_-273.15<<std::endl;
  //  exp smoothing
    double expsm = 0.95;
    min_ = expsm * min_ + (1. - expsm) * min;
    max_ = expsm * max_ + (1. - expsm) * max;

    for (int i = 0; i < img16.cols; ++i)  {
        for (int j = 0; j < img16.rows; ++j)   {
            double temp;
            if (doTempConversion) {
                temp = powerToK4(img16.at<uint16_t>(j, i));
            } else {
                temp = (double)(img16.at<uint16_t>(j, i));
            }

            int val = (((temp - min_) / (max_ - min_)) * 255);
            val = val > std::numeric_limits<uint8_t>::max() ? std::numeric_limits<uint8_t>::max() : val < 0 ? 0 : val; //saturate
            img8.at<uint8_t>(j, i) = (uint8_t)val;
        }
    }
    firstframe_ = false ;
}

//***************************************************************
//
//***************************************************************
void converter_16_8::convertTo16bit(const cv::Mat& srcImg, cv::Mat& destImg, bool doTempConversion)
{
    if (destImg.empty())  { //make an image if the user has provided nothing
        destImg.create(cv::Size(srcImg.cols, srcImg.rows), CV_16UC1) ;
    }
    double min = std::numeric_limits<uint16_t>::max();
    double max = 0;
  //make a histogram of intensities
    typedef std::map<double, int> hist_t;
    hist_t hist;
    double bucketwidth = 2.; //bucketwidth in degrees K

    for (int i = 0; i < srcImg.cols; ++i)  {
        for (int j = 0; j < srcImg.rows; ++j)   {
            double power = srcImg.at<uint16_t>(j, i);
            double temp;
            if (doTempConversion) {
                temp = powerToK4(power);
            } else {
                temp = power;
            }
            temp = round(temp / bucketwidth) * bucketwidth;
            hist[temp]++;
        }
    }

  //find the main section of the histogram
    for (hist_t::const_iterator it = hist.begin(); it != hist.end(); ++it)  {
        if (it->second > histminmembersperbucket)  {
            if (it->first > max)
                max = it->first ;
            if (it->first < min)
                min = it->first ;
        }
    }

    if (firstframe_)  {
        min_ = min;
        max_ = max;
    }
  //  std::cout<<"min: "<<min-273.15<<" max: "<<max-273.15<<" sm: min: "<<min_-273.15<<" max: "<<max_-273.15<<std::endl;
  //  exp smoothing
    double expsm = 0.95;
    min_ = expsm * min_ + (1. - expsm) * min;
    max_ = expsm * max_ + (1. - expsm) * max;

    for (int i = 0; i < srcImg.cols; ++i)  {
        for (int j = 0; j < srcImg.rows; ++j)   {
            double temp;
            if (doTempConversion) {
                temp = powerToK4(srcImg.at<uint16_t>(j, i));
            } else {
                temp = (double)(srcImg.at<uint16_t>(j, i));
            }

//            int val = (((temp - min_) / (max_ - min_)) * 255);
//            int val = (((temp - min_) / (max_ - min_)) * 16384);
            int val = (((temp - min_) / (max_ - min_)) * 65536);
            val = val > std::numeric_limits<uint16_t>::max() ? std::numeric_limits<uint16_t>::max() : val < 0 ? 0 : val; //saturate
            destImg.at<uint16_t>(j, i) = (uint16_t) val;
        }
    }
    firstframe_ = false ;
}

//***************************************************************
//  reference: https://github.com/gtatters/Thermimage/blob/master/R/raw2temp.R
//***************************************************************
//double raw2temp(raw, E=1, OD=1, RTemp=20, ATemp=RTemp, IRWTemp=RTemp, IRT=1, RH=50,
//                     PR1=21106.77, PB=1501, PF=1, PO=-7340, PR2=0.012545258,
//         	 					 ATA1=0.006569, ATA2=0.01262, ATB1=-0.002276, ATB2=-0.00667, ATX=1.9)
//{
/*
   how to call this function:
   raw2temp(raw,E,OD,RTemp,ATemp,IRWTemp,IRT,RH,PR1,PB,PF,PO,PR2)
   Example with all settings at default/blackbody levels
   raw2temp(18109,1,0,20,20,20,1,50,PR1,PB,PF,PO,PR2)
   example with emissivity=0.95, distance=1m, window transmission=0.96, all temperatures=20C,
   50% relative humidity
   raw2temp(18109,0.95,1,20,20,20,0.96,50)
   default calibration constants for my FLIR camera will be used if you leave out the
   calibration data

	 raw: A/D bit signal from FLIR file
   FLIR .seq files and .fcf files store data in a 16-bit encoded value.
   This means it can range from 0 up to 65535.  This is referred to as the raw value.
   The raw value is actually what the sensor detects which is related to the radiance hitting
   the sensor.
   At the factory, each sensor has been calibrated against a blackbody radiation source so
   calibration values to conver the raw signal into the expected temperature of a blackbody
   radiator are provided.
   Since the sensors do not pick up all wavelengths of light, the calibration can be estimated
   using a limited version of Planck's law.  But the blackbody calibration is still critical
   to this.

   E: Emissivity - default 1, should be ~0.95 to 0.97 depending on source
   OD: Object distance in metres
   RTemp: apparent reflected temperature - one value from FLIR file (oC), default 20C
   ATemp: atmospheric temperature for tranmission loss - one value from FLIR file (oC) - default = RTemp
   IRWinT: Infrared Window Temperature - default = RTemp (oC)
   IRT: Infrared Window transmission - default 1.  likely ~0.95-0.96. Should be empirically determined.
   RH: Relative humidity - default 50%

   Note: PR1, PR2, PB, PF, and PO are specific to each camera and result from the calibration at factory
   of the camera's Raw data signal recording from a blackbody radiation source
   Calibration Constants                 (A FLIR SC660, A FLIR T300(25o), T300(telephoto), A Mikron 7515)
   PR1: PlanckR1 calibration constant from FLIR file  21106.77       14364.633     14906.216       21106.77
   PB: PlanckB calibration constant from FLIR file    1501           1385.4        1396.5          9758.743281
   PF: PlanckF calibration constant from FLIR file    1              1             1               29.37648768
   PO: PlanckO calibration constant from FLIR file    -7340          -5753         -7261           1278.907078
   PR2: PlanckR2 calibration constant form FLIR file  0.012545258    0.010603162   0.010956882     0.0376637583528285

   Set constants below. Comment out those that are variables in this function
   Keep those that should remain constants.
   These are here to make troubleshooting calculations easier if not running this as a function call
   raw<-19746; E<-0.95; OD<-20; IRT<-0.96
   RTemp<-20; IRWTemp<-RTemp; ATemp<-20; RH<-50
   PR1<-21106.77; PB<-1501; PF<-1; PO<--7340; PR2<-0.012545258

   These humidity parameters were extracted from the FLIR SC660 camera
   ATA1: Atmospheric Trans Alpha 1  0.006569 constant for calculating humidity effects on transmission
   ATA2: Atmospheric Trans Alpha 2  0.012620 constant for calculating humidity effects on transmission
   ATB1: Atmospheric Trans Beta 1  -0.002276 constant for calculating humidity effects on transmission
   ATB2: Atmospheric Trans Beta 2  -0.006670 constant for calculating humidity effects on transmission
   ATX:  Atmospheric Trans X        1.900000 constant for calculating humidity effects on transmission

   Equations to convert to temperature
   See http://130.15.24.88/exiftool/forum/index.php/topic,4898.60.html
   Standard equation: temperature<-PB/log(PR1/(PR2*(raw+PO))+PF)-273.15
   Other source of information: Minkina and Dudzik's Infrared Thermography: Errors and Uncertainties
*/
/*
  emiss.wind<-1-IRT
  refl.wind<-0 // anti-reflective coating on window
  h2o<-(RH/100)*exp(1.5587+0.06939*(ATemp)-0.00027816*(ATemp)^2+0.00000068455*(ATemp)^3)
  // converts relative humidity into water vapour pressure (I think in units mmHg)
  tau1<-ATX*exp(-sqrt(OD/2)*(ATA1+ATB1*sqrt(h2o)))+(1-ATX)*exp(-sqrt(OD/2)*(ATA2+ATB2*sqrt(h2o)))
  tau2<-ATX*exp(-sqrt(OD/2)*(ATA1+ATB1*sqrt(h2o)))+(1-ATX)*exp(-sqrt(OD/2)*(ATA2+ATB2*sqrt(h2o)))
  // transmission through atmosphere - equations from Minkina and Dudzik's Infrared Thermography Book
  // Note: for this script, we assume the thermal window is at the mid-point (OD/2) between the source
  // and the camera sensor

  raw.refl1<-PR1/(PR2*(exp(PB/(RTemp+273.15))-PF))-PO   // radiance reflecting off the object before the window
  raw.refl1.attn<-(1-E)/E*raw.refl1   // attn = the attenuated radiance (in raw units)

  raw.atm1<-PR1/(PR2*(exp(PB/(ATemp+273.15))-PF))-PO // radiance from the atmosphere (before the window)
  raw.atm1.attn<-(1-tau1)/E/tau1*raw.atm1 // attn = the attenuated radiance (in raw units)

  raw.wind<-PR1/(PR2*(exp(PB/(IRWTemp+273.15))-PF))-PO
  raw.wind.attn<-emiss.wind/E/tau1/IRT*raw.wind

  raw.refl2<-PR1/(PR2*(exp(PB/(RTemp+273.15))-PF))-PO
  raw.refl2.attn<-refl.wind/E/tau1/IRT*raw.refl2

  raw.atm2<-PR1/(PR2*(exp(PB/(ATemp+273.15))-PF))-PO
  raw.atm2.attn<-(1-tau2)/E/tau1/IRT/tau2*raw.atm2

  raw.obj<-(raw/E/tau1/IRT/tau2-raw.atm1.attn-raw.atm2.attn-raw.wind.attn-raw.refl1.attn-raw.refl2.attn)

  temp.C<-PB/log(PR1/(PR2*(raw.obj+PO))+PF)-273.15

  temp.C
	*/
//}
