//
//  PixelCalc.h
//  minecraftpe
//
//  Created by rhino on 10/24/11.
//  Copyright (c) 2011 Mojang AB. All rights reserved.
//

#ifndef minecraftpe_PixelCalc_h
#define minecraftpe_PixelCalc_h

class PixelCalc
{
public:
    PixelCalc(float pixelsPerMillimeter = 1) {
        setPixelsPerMillimeter(pixelsPerMillimeter);
    }
    
    float millimetersToPixels(float mm) const {
        return mm * _pixelsPerMillimeter;
    }
    float pixelsToMillimeters(float pp) const {
        return pp * _millimetersPerPixel;
    }
    
    void setPixelsPerMillimeter(float pixelsPerMillimeter) {
        _pixelsPerMillimeter = pixelsPerMillimeter;
        _millimetersPerPixel = 1.0f / _pixelsPerMillimeter;
    }
    
private:
    float _pixelsPerMillimeter;
    float _millimetersPerPixel;
};

#endif
