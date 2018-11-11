// Copyright 2014 Olivier Gillet.
//
// Author: Olivier Gillet (ol.gillet@gmail.com)
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
// 
// See http://creativecommons.org/licenses/MIT/ for more information.
//
// -----------------------------------------------------------------------------
//
// Driver for the 15 unicolor LEDs controlled by a 595.

#ifndef CLOUDS_DRIVERS_LEDS_H_
#define CLOUDS_DRIVERS_LEDS_H_

#include <stm32f4xx_conf.h>

#include "stmlib/stmlib.h"

namespace clouds {

class Leds {
 public:
  Leds() { }
  ~Leds() { }
  
  void Init();
  
  void set_status(uint8_t channel, uint8_t led) {
    led_[channel] = led;
  /*  green_[channel] = green;*/
  }

  void set_intensity(uint8_t channel, uint8_t value) { //its not used, all LEDs unipolar
    uint8_t led = 0;
    //uint8_t green = 0;
    if (value < 128) {
    //  green = value << 1;
    //} else if (value < 192) {
    //  green = 255;
      led = (value - 128) << 2;
    } else {
    //  green = 255 - ((value - 192) << 2);
      led = 255;
    }
    set_status(channel, led/*, green*/);
  }
  
  void PaintBar(int32_t db) {
    if (db < 0) {
      return;
    }
    if (db > 32767) {
      db = 32767;
    }
    db <<= 1;
    if (db >= 49152) {
      // Out LEDs
      set_status(0, (db - 49152) >> 6);
      set_status(1, 255);
      set_status(2, 255);
      set_status(3, 255);
      // In LEDs
      set_status(4, (db - 49152) >> 6);
      set_status(5, 255);
      set_status(6, 255);
      set_status(7, 255);
    } else if (db >= 32768) {
      // Out LEDs
      set_status(1, (db - 32768) >> 6);
      set_status(2, 255);
      set_status(3, 255);
      // In LEDs
      set_status(5, (db - 32768) >> 6);
      set_status(6, 255);
      set_status(7, 255);
    } else if (db >= 16384) {
      // Out LEDs
      set_status(2, (db - 16384) >> 6);
      set_status(3, 255);
      // In LEDs
      set_status(6, (db - 16384) >> 6);
      set_status(7, 255);
    } else {
      // Out LEDs
      set_status(3, db >> 6);
      // In LEDs
      set_status(7, db >> 6);
    }
  }

  void PaintBar(int32_t indb, int32_t outdb, uint8_t inmute, uint8_t outmute) {
  /*
    if (indb < 0 && outdb < 0) {
      return;
    }
    */
    if (inmute) {
      indb = 0;
    }
    if (indb > 32767) {
      indb = 32767;
    }
    indb <<= 1;
    if (outdb > 32767) {
      outdb = 32767;
    }
    outdb <<= 1;
    // outs -- TODO: HACK VCA control signal to an ADC
    if (outdb >= 49152) {
      set_status(0, (outdb - 49152) >> 6);
      set_status(1,255);
      set_status(2,255);
      set_status(3,255);
    } else if (outdb >= 32768) {
      set_status(1,(indb - 32768) >> 6);
      set_status(2,255);
      set_status(3,255);
    } else if (outdb >= 16384) {
      set_status(2,(outdb - 16384) >> 6);
      set_status(3,255);
    } else {
      set_status(3,outdb >> 6);
    }
    // ins
    if (indb >= 49152) {
      set_status(4, (indb - 49152) >> 6);
      set_status(5,255);
      set_status(6,255);
      set_status(7,255);
    } else if (indb >= 32768) {
      set_status(5,(indb - 32768) >> 6);
      set_status(6,255);
      set_status(7,255);
    } else if (indb >= 16384) {
      set_status(6,(indb - 16384) >> 6);
      set_status(7,255);
    } else {
      set_status(7,indb >> 6);
    }
    if (inmute > 0) {
        set_status(4, 255);
    }
    if (outmute > 0) {
        set_status(0, 255);
    }

/*
    if (db >= 57337) {
      set_status(0, (db - 57337) >> 6);
      set_status(1, 255);
      set_status(2, 255);
      set_status(3, 255);
      set_status(4, 255);
      set_status(5, 255);
      set_status(6, 255);
      set_status(7, 255);
    } else if (db >= 49146) {
      set_status(1, (db - 49145) >> 6);
      set_status(2, 255);
      set_status(3, 255);
      set_status(4, 255);
      set_status(5, 255);
      set_status(6, 255);
      set_status(7, 255);
    } else if (db >= 40955) {
      set_status(2, (db - 40955) >> 6);
      set_status(3, 255);
      set_status(4, 255);
      set_status(5, 255);
      set_status(6, 255);
      set_status(7, 255);
    } else if (db >= 32763) {
      set_status(3, (db - 32763) >> 6);
      set_status(4, 255);
      set_status(5, 255);
      set_status(6, 255);
      set_status(7, 255);
    } else if (db >= 24573) {
      set_status(4, (db - 24573) >> 6);
      set_status(5, 255);
      set_status(6, 255);
      set_status(7, 255);
    } else if (db >= 16381) {
      set_status(5, (db - 16381) >> 6);
      set_status(6, 255);
      set_status(7, 255);
    } else if (db >= 8191) {
      set_status(6, (db - 8191) >> 6);
      set_status(7, 255);
    } else {
      set_status(7, db >> 6);
    }
    */
  }
  
  void Clear();
  
  void set_freeze(bool status) {
    freeze_led_ = status;
  }
  
  void Write();

  void Test(bool state);

 private:
  bool freeze_led_;
  uint8_t pwm_counter_;
  uint8_t led_[16];
 /* uint8_t green_[4];*/
   
  DISALLOW_COPY_AND_ASSIGN(Leds);
};

}  // namespace clouds

#endif  // CLOUDS_DRIVERS_LEDS_H_
