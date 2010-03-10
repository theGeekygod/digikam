/* ============================================================
 *
 * This file is a part of digiKam project
 * http://www.digikam.org
 *
 * Date        : 2009-08-09
 * Description : Enhance image with local contrasts (as human eye does).
 *               LDR ToneMapper <http://zynaddsubfx.sourceforge.net/other/tonemapping>
 *
 * Copyright (C) 2009 by Nasca Octavian Paul <zynaddsubfx at yahoo dot com>
 * Copyright (C) 2009 by Julien Pontabry <julien dot pontabry at gmail dot com>
 * Copyright (C) 2009-2010 by Gilles Caulier <caulier dot gilles at gmail dot com>
 *
 * This program is free software; you can redistribute it
 * and/or modify it under the terms of the GNU General
 * Public License as published by the Free Software Foundation;
 * either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * ============================================================ */

#include "localcontrastfilter.h"

// KDE includes

#include <kdebug.h>

namespace Digikam
{

class LocalContrastFilterPriv
{
public:

    LocalContrastFilterPriv()
    {
        current_process_power_value = 20.0;
    }

    // preprocessed values
    float                  current_process_power_value;

    LocalContrastContainer par;
};

LocalContrastFilter::LocalContrastFilter(DImg* image, QObject* parent, const LocalContrastContainer& par)
                   : DImgThreadedFilter(image, parent, "LocalContrast"),
                     d(new LocalContrastFilterPriv)
{
    set_parameters(par);
    initFilter();
}

LocalContrastFilter::~LocalContrastFilter()
{
    delete d;
}

void LocalContrastFilter::filterImage()
{
    postProgress(0);

    // Process image

    if(!m_orgImage.isNull())
    {
        int size = m_orgImage.width()*m_orgImage.height()*3;
        int i, j;

        if(m_orgImage.sixteenBit())
        {
            // sixteen bit image
            unsigned short* data    = new unsigned short[size];
            unsigned short* dataImg = (unsigned short*)(m_orgImage.bits());

            for (i=0, j=0; !m_cancel && (i < size); i+=3, j+=4)
            {
                data[i]   = dataImg[j];
                data[i+1] = dataImg[j+1];
                data[i+2] = dataImg[j+2];
            }

            postProgress(10);

            process_16bit_rgb_image(data, m_orgImage.width(), m_orgImage.height());

            for (uint x=0; !m_cancel && (x < m_orgImage.width()); x++)
            {
                for (uint y=0; !m_cancel && (y < m_orgImage.height()); y++)
                {
                    i = (m_orgImage.width() * y + x)*3;
                    m_destImage.setPixelColor(x, y, DColor((unsigned short)data[i+2],
                                                           (unsigned short)data[i+1],
                                                           (unsigned short)data[i],
                                                           65535, true));
                }
            }

            delete [] data;
        }
        else
        {
            // eight bit image

            uchar* data = new uchar[size];

            for (i=0, j=0; !m_cancel && (i < size); i+=3, j+=4)
            {
                data[i]   = m_orgImage.bits()[j];
                data[i+1] = m_orgImage.bits()[j+1];
                data[i+2] = m_orgImage.bits()[j+2];
            }

            postProgress(10);

            process_8bit_rgb_image(data, m_orgImage.width(), m_orgImage.height());

            for (uint x=0; !m_cancel && (x < m_orgImage.width()); x++)
            {
                for (uint y=0; !m_cancel && (y < m_orgImage.height()); y++)
                {
                    i = (m_orgImage.width() * y + x)*3;
                    m_destImage.setPixelColor(x, y, DColor(data[i+2], data[i+1], data[i], 255, false));
                }
            }

            delete [] data;
        }
    }

    postProgress(100);
}

void LocalContrastFilter::set_parameters(const LocalContrastContainer& par)
{
    d->par = par;
    set_low_saturation(d->par.low_saturation);
    set_high_saturation(d->par.high_saturation);
    set_stretch_contrast(d->par.stretch_contrast);
    set_function_id(d->par.function_id);

    for (int i=0 ; i < TONEMAPPING_MAX_STAGES ; i++)
    {
        set_power(i, d->par.stage[i].power);
        set_blur(i, d->par.stage[i].blur);
    };

    update_preprocessed_values();
}

LocalContrastContainer LocalContrastFilter::get_parameters() const
{
    return d->par;
}

void LocalContrastFilter::process_8bit_rgb_image(unsigned char* img, int sizex, int sizey)
{
    int size            = sizex*sizey;
    float* tmpimage     = new float[size*3];
    const float inv_256 = 1.0/256.0;

    for (int i=0 ; !m_cancel && (i < size*3) ; i++)
    {
        // convert to floating point
        tmpimage[i] = (float)(img[i]/255.0);
    }

    process_rgb_image(tmpimage, sizex, sizey);

    // convert back to 8 bits (with dithering)
    int pos=0;

    for (int i=0 ; !m_cancel && (i < size) ; i++)
    {
        float dither = ((rand()/256)%256)*inv_256;
        img[pos]     = (int)(tmpimage[pos]  *255.0+dither);
        img[pos+1]   = (int)(tmpimage[pos+1]*255.0+dither);
        img[pos+2]   = (int)(tmpimage[pos+2]*255.0+dither);
        pos += 3;
    }

    delete [] tmpimage;
    postProgress(90);
}

void LocalContrastFilter::process_16bit_rgb_image(unsigned short int* img, int sizex, int sizey)
{
    int size              = sizex*sizey;
    float* tmpimage       = new float[size*3];
    const float inv_65536 = 1.0/65536.0;

    for (int i=0 ; !m_cancel && (i < size*3) ; i++)
    {
        // convert to floating point
        tmpimage[i] = (float)(img[i]/65535.0);
    }

    process_rgb_image(tmpimage, sizex, sizey);

    // convert back to 8 bits (with dithering)
    int pos = 0;

    for (int i=0 ; !m_cancel && (i < size) ; i++)
    {
        float dither = ((rand()/65536)%65536)*inv_65536;
        img[pos]     = (int)(tmpimage[pos]  *65535.0+dither);
        img[pos+1]   = (int)(tmpimage[pos+1]*65535.0+dither);
        img[pos+2]   = (int)(tmpimage[pos+2]*65535.0+dither);
        pos+=3;
    }

    delete [] tmpimage;

    postProgress(90);
}

float LocalContrastFilter::func(float x1, float x2)
{
    float result = 0.5;
    float p;

    /*
    //test function
    if (d->par.function_id==1)
    {
        p=pow(0.1,fabs((x2*2.0-1.0))*d->current_process_power_value*0.02);
        if (x2<0.5) result=pow(x1,p);
        else result=1.0-pow(1.0-x1,p);
        return result;
    };
    //test function
    if (function_id==1)
    {
        p=d->current_process_power_value*0.3+1e-4;
        x2=1.0/(1.0+exp(-(x2*2.0-1.0)*p*0.5));
        float f=1.0/(1.0+exp((1.0-(x1-x2+0.5)*2.0)*p));
        float m0=1.0/(1.0+exp((1.0-(-x2+0.5)*2.0)*p));
        float m1=1.0/(1.0+exp((1.0-(-x2+1.5)*2.0)*p));
        result=(f-m0)/(m1-m0);
        return result;
    };
    */

    switch (d->par.function_id)
    {
        case 0:  //power function
            p = (float)(pow((double)10.0,(double)fabs((x2*2.0-1.0))*d->current_process_power_value*0.02));
            if (x2 >= 0.5) result = pow(x1,p);
            else result = (float)(1.0-pow((double)1.0-x1,(double)p));
            break;
        case 1:  //linear function
            p = (float)(1.0/(1+exp(-(x2*2.0-1.0)*d->current_process_power_value*0.04)));
            result = (x1 < p) ? (float)(x1*(1.0-p)/p) : (float)((1.0-p)+(x1-p)*p/(1.0-p));
            break;
    };

    return result;
}

void LocalContrastFilter::process_rgb_image(float* img, int sizex, int sizey)
{
    update_preprocessed_values();

    int size         = sizex*sizey;
    float* blurimage = new float[size];
    float* srcimg    = new float[size*3];

    for (int i=0 ; i < (size*3) ; i++)
        srcimg[i] = img[i];

    if (d->par.stretch_contrast)
    {
        stretch_contrast(img, size*3);
    }

    int pos = 0;

    for (int nstage=0 ; !m_cancel && (nstage < TONEMAPPING_MAX_STAGES) ; nstage++)
    {
        if (d->par.stage[nstage].enabled)
        {
            // compute the desatured image

            pos = 0;

            for (int i=0 ; !m_cancel && (i < size) ; i++)
            {
                blurimage[i] = (float)((img[pos]+img[pos+1]+img[pos+2])/3.0);
                pos += 3;
            }

            d->current_process_power_value = d->par.get_power(nstage);

            // blur

            inplace_blur(blurimage, sizex, sizey, d->par.get_blur(nstage));

            pos = 0;

            for (int i=0 ; !m_cancel && (i<size) ; i++)
            {
                float src_r  = img[pos];
                float src_g  = img[pos+1];
                float src_b  = img[pos+2];

                float blur   = blurimage[i];

                float dest_r = func(src_r, blur);
                float dest_g = func(src_g, blur);
                float dest_b = func(src_b, blur);

                img[pos]     = dest_r;
                img[pos+1]   = dest_g;
                img[pos+2]   = dest_b;

                pos += 3;
            }
        }

        postProgress(30 + nstage*10);
    }

    int high_saturation_value = 100-d->par.high_saturation;
    int low_saturation_value  = 100-d->par.low_saturation;

    if ((d->par.high_saturation != 100) || (d->par.low_saturation != 100))
    {
        int pos = 0;

        for (int i=0 ; !m_cancel && (i < size) ; i++)
        {
            float src_h, src_s, src_v;
            float dest_h, dest_s, dest_v;
            rgb2hsv(srcimg[pos], srcimg[pos+1], srcimg[pos+2], src_h, src_s, src_v);
            rgb2hsv(img[pos], img[pos+1], img[pos+2], dest_h, dest_s, dest_v);

            float dest_saturation = (float)((src_s*high_saturation_value+dest_s*(100.0-high_saturation_value))*0.01);
            if (dest_v>src_v)
            {
                float s1        = (float)(dest_saturation*src_v/(dest_v+1.0/255.0));
                dest_saturation = (float)((low_saturation_value*s1+d->par.low_saturation*dest_saturation)*0.01);
            }

            hsv2rgb(dest_h, dest_saturation, dest_v, img[pos], img[pos+1], img[pos+2]);

            pos += 3;
        }
    }

    postProgress(70);

    // Unsharp Mask filter

    if (d->par.unsharp_mask.enabled)
    {
        float* val = new float[size];

        // compute the desatured image

        int pos = 0;

        for (int i=0 ; !m_cancel && (i < size) ; i++)
        {
            val[i] = blurimage[i] = (float)((img[pos]+img[pos+1]+img[pos+2])/3.0);
            //val[i] = blurimage[i] = (float)(max3(img[pos],img[pos+1],img[pos+2]));
            pos += 3;
        }

        float blur_value = d->par.get_unsharp_mask_blur();
        inplace_blur(blurimage, sizex, sizey, blur_value);

        pos              = 0;
        float pow        = (float)(2.5*d->par.get_unsharp_mask_power());
        float threshold  = (float)(d->par.unsharp_mask.threshold*pow/250.0);
        float threshold2 = threshold/2;

        for (int i=0 ; !m_cancel && (i < size) ; i++)
        {
            float dval     = (val[i]-blurimage[i])*pow;
            float abs_dval = fabs(dval);
            if (abs_dval < threshold)
            {
                if (abs_dval > threshold2)
                {
                    bool sign = (dval < 0.0);
                    dval      = (float)((abs_dval-threshold2)*2.0);
                    if (sign) dval =- dval;
                }
                else
                {
                    dval = 0;
                }
            }

            float r   = img[pos]  +dval;
            float g   = img[pos+1]+dval;
            float b   = img[pos+2]+dval;

            if (r<0.0) r = 0.0;
            if (r>1.0) r = 1.0;
            if (g<0.0) g = 0.0;
            if (g>1.0) g = 1.0;
            if (b<0.0) b = 0.0;
            if (b>1.0) b = 1.0;

            img[pos]   = r;
            img[pos+1] = g;
            img[pos+2] = b;

            pos += 3;
        }

        delete [] val;
    }

    delete [] srcimg;
    delete [] blurimage;

    postProgress(80);
}

void LocalContrastFilter::update_preprocessed_values()
{
    postProgress(20);
}

void LocalContrastFilter::inplace_blur(float* data, int sizex, int sizey, float blur)
{
    if (blur < 0.3) return;

    float a = (float)(exp(log(0.25)/blur));

    if ((a <= 0.0) || (a >= 1.0)) return;

    a *= a;
    float denormal_remove = (float)(1e-15);

    for (int stage=0 ; !m_cancel && (stage < 2) ; stage++)
    {
        for (int y=0 ; !m_cancel && (y < sizey) ; y++)
        {
            int pos   = y*sizex;
            float old = data[pos];
            pos++;

            for (int x=1 ; !m_cancel && (x < sizex) ; x++)
            {
                old       = (data[pos]*(1-a)+old*a)+denormal_remove;
                data[pos] = old;
                pos++;
            }

            pos = y*sizex+sizex-1;

            for (int x=1 ; !m_cancel && (x < sizex) ; x++)
            {
                old       = (data[pos]*(1-a)+old*a)+denormal_remove;
                data[pos] = old;
                pos--;
            }
        }

        for (int x=0 ; !m_cancel && (x < sizex) ; x++)
        {
            int pos   = x;
            float old = data[pos];

            for (int y=1 ; !m_cancel && (y < sizey) ; y++)
            {
                old       = (data[pos]*(1-a)+old*a)+denormal_remove;
                data[pos] = old;
                pos += sizex;
            }

            pos = x+sizex*(sizey-1);

            for (int y=1 ; !m_cancel && (y < sizey) ; y++)
            {
                old       = (data[pos]*(1-a)+old*a)+denormal_remove;
                data[pos] = old;
                pos -= sizex;
            }
        }
    }
}

void LocalContrastFilter::stretch_contrast(float* data, int datasize)
{
    //stretch the contrast
    const unsigned int histogram_size=256;
    //first, we compute the histogram
    unsigned int histogram[histogram_size];

    for (unsigned int i=0 ; i < histogram_size ; i++)
    histogram[i] = 0;

    for (unsigned int i=0 ; !m_cancel && (i < (unsigned int)datasize) ; i++)
    {
        int m = (int)(data[i]*(histogram_size-1));
        if (m < 0) m = 0;
        if (m > (int)(histogram_size-1)) m = histogram_size-1;
        histogram[m]++;
    }

    //I want to strip the lowest and upper 0.1 procents (in the histogram) of the pixels
    int          min         = 0;
    int          max         = 255;
    unsigned int desired_sum = datasize/1000;
    unsigned int sum_min     = 0;
    unsigned int sum_max     = 0;

    for (unsigned int i=0 ; !m_cancel && (i < histogram_size) ; i++)
    {
        sum_min += histogram[i];
        if (sum_min > desired_sum)
        {
            min = i;
            break;
        }
    }

    for (int i = histogram_size-1 ; !m_cancel && (i >= 0) ; i--)
    {
        sum_max += histogram[i];

        if (sum_max > desired_sum)
        {
            max = i;
            break;
        }
    }

    if (min >= max)
    {
        min = 0;
        max = 255;
    }

    float min_src_val = (float)(min/255.0);
    float max_src_val = (float)(max/255.0);

    for (int i=0 ; !m_cancel && (i < datasize) ; i++)
    {
        //stretch the contrast
        float x = data[i];
        x       = (x-min_src_val)/(max_src_val-min_src_val);

        if (x < 0.0)
            x = 0.0;
        if (x > 1.0)
            x = 1.0;

        data[i] = x;
    }
}

void LocalContrastFilter::set_enabled(int nstage, bool enabled)
{
    d->par.stage[nstage].enabled = enabled;
}

void LocalContrastFilter::set_unsharp_mask_enabled(bool value)
{
    d->par.unsharp_mask.enabled = value;
}

void LocalContrastFilter::set_unsharp_mask_power(float value)
{
    if (value < 0.0) value = 0.0;
    if (value > 100.0) value = 100.0;
    d->par.unsharp_mask.power = value;
}

void LocalContrastFilter::set_unsharp_mask_blur(float value)
{
    if (value < 0.0) value = 0.0;
    if (value > 5000.0) value = 5000.0;
    d->par.unsharp_mask.blur = value;
}

void LocalContrastFilter::set_unsharp_mask_threshold(int value)
{
    if (value < 0) value = 0;
    if (value > 100) value = 100;
    d->par.unsharp_mask.threshold = value;
}

float LocalContrastFilter::get_enabled(int nstage)
{
    return d->par.stage[nstage].enabled;
}

float LocalContrastFilter::get_blur(int nstage)
{
    return d->par.stage[nstage].blur;
}

float LocalContrastFilter::get_power(int nstage)
{
    return d->par.stage[nstage].power;
}

int LocalContrastFilter::get_low_saturation()
{
    return d->par.low_saturation;
}

int LocalContrastFilter::get_high_saturation()
{
    return d->par.high_saturation;
}

bool LocalContrastFilter::get_stretch_contrast()
{
    return d->par.stretch_contrast;
}

int LocalContrastFilter::get_function_id()
{
    return d->par.function_id;
}

bool LocalContrastFilter::get_unsharp_mask_enabled(bool /*value*/)
{
    return d->par.unsharp_mask.enabled;
}

float LocalContrastFilter::get_unsharp_mask_power(float /*value*/)
{
    return d->par.unsharp_mask.power;
}

float LocalContrastFilter::get_unsharp_mask_(float /*value*/)
{
    return d->par.unsharp_mask.blur;
}

int LocalContrastFilter::get_unsharp_mask_threshold(int /*value*/)
{
    return d->par.unsharp_mask.threshold;
}

void LocalContrastFilter::set_current_stage(int nstage)
{
    d->current_process_power_value = d->par.get_power(nstage);
}

void LocalContrastFilter::set_blur(int nstage, float value)
{
    if (value < 0) value = 0;
    if (value > 10000.0) value = 10000.0;
    d->par.stage[nstage].blur = value;
}

void LocalContrastFilter::set_power(int nstage, float value)
{
    if (value < 0) value = 0;
    if (value > 100.0) value = 100.0;
    d->par.stage[nstage].power = value;
}

void LocalContrastFilter::set_low_saturation(int value)
{
    if (value < 0) value = 0;
    if (value > 100) value = 100;
    d->par.low_saturation = value;
}

void LocalContrastFilter::set_high_saturation(int value)
{
    if (value < 0) value = 0;
    if (value > 100) value = 100;
    d->par.high_saturation = value;
}

void LocalContrastFilter::set_stretch_contrast(bool value)
{
    d->par.stretch_contrast = value;
}

void LocalContrastFilter::set_function_id (int value)
{
    if (value < 0) value = 0;
    if (value > 1) value = 1;
    d->par.function_id = value;
}

void LocalContrastFilter::rgb2hsv(const float& r, const float& g, const float& b, float& h, float& s, float& v)
{
    float maxrg = (r>g) ? r : g;
    float max   = (maxrg>b) ? maxrg : b;
    float minrg = (r<g) ? r : g;
    float min   = (minrg<b) ? minrg : b;
    float delta = max-min;

    //hue
    if (min == max)
    {
        h = 0.0;
    }
    else
    {
        if (max == r)
        {
            h = (float)(fmod(60.0*(g-b)/delta+360.0, 360.0));
        }
        else
        {
            if (max == g)
            {
                h = (float)(60.0*(b-r)/delta+120.0);
            }
            else
            {
                //max==b
                h = (float)(60.0*(r-g)/delta+240.0);
            }
        }
    }

    //saturation
    if (max < 1e-6)
    {
        s = 0;
    }
    else
    {
        s = (float)(1.0-min/max);
    }

    //value
    v = max;
};

void LocalContrastFilter::hsv2rgb(const float& h, const float& s, const float& v, float& r, float& g, float& b)
{
    float hfi = (float)(floor(h/60.0));
    float f   = (float)((h/60.0)-hfi);
    int hi    = ((int)hfi)%6;
    float p   = (float)(v*(1.0-s));
    float q   = (float)(v*(1.0-f*s));
    float t   = (float)(v*(1.0-(1.0-f)*s));

    switch (hi)
    {
        case 0:
            r = v ; g = t ; b = p;
            break;
        case 1:
            r = q ; g = v ; b = p;
            break;
        case 2:
            r = p ; g = v ; b = t;
            break;
        case 3:
            r = p ; g = q ; b = v;
            break;
        case 4:
            r = t ; g = p; b = v;
            break;
        case 5:
            r = v ; g = p ; b = q;
            break;
    }
}

} // namespace Digikam
