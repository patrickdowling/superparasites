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
// Calibration settings.

#ifndef CLOUDS_CV_SCALER_H_
#define CLOUDS_CV_SCALER_H_

#include "stmlib/stmlib.h"

#include "supercell/settings.h"
#include "supercell/drivers/adc.h"
#include "supercell/drivers/pots_adc.h"
#include "supercell/drivers/gate_input.h"
#include "supercell/dsp/parameters.h"

namespace clouds {

#define ADC_CHANNELS_TOTAL (ADC_CHANNEL_LAST + ADC_CHANNEL_POTENTIOMETER_LAST)

struct CvTransformation {
  bool flip;
  bool remove_offset;
  float filter_coefficient;
};

class CvScaler {
 public:
  CvScaler() { }
  ~CvScaler() { }
  
  void Init(CalibrationData* calibration_data);
  void Read(Parameters* parameters);
  
  void CalibrateC1() {
    cv_c1_ = adc_.float_value(ADC_VOCT_CV);
  }

  void CalibrateOffsets() {
    for (size_t i = 0; i < ADC_CHANNELS_TOTAL; ++i) {
      calibration_data_->offset[i] = adc_.float_value(i);
    }
  }
  
  bool CalibrateC3() {
    float c3 = adc_.float_value(ADC_VOCT_CV);  // 0.4848 v0.1 ; 0.3640 v0.2
    float c1 = cv_c1_;  // 0.6666 v0.1 ; 0.6488 v0.2
    float delta = c3 - c1;
    if (delta > -0.5f && delta < -0.0f) {
      calibration_data_->pitch_scale = 24.0f / (c3 - c1);
      calibration_data_->pitch_offset = 12.0f - \
          calibration_data_->pitch_scale * c1;
      return true;
    } else {
      return false;
    }
  }
  
  inline uint8_t adc_value(size_t index) const {
    return adc_.value(index) >> 8;
  }
  
  inline bool gate(size_t index) const {
    return index == 0 ? gate_input_.freeze() : gate_input_.capture();
  }

  inline void set_capture_flag() {
    capture_button_flag_ = true;
  }

  inline float output_level() const {
    return output_level_ * output_level_;
    //return output_level_ < 0.5f ? (1.0f - (output_level_ * 0.667f)) * 2.6667f : 0.0f;
  }

  inline float pan_pot() const {
    return smoothed_adc_value_[ADC_CHANNEL_LAST + ADC_SPREAD_POTENTIOMETER];
  }

 private:
  static const int kAdcLatency = 5;
  
  Adc adc_;
  PotsAdc pots_adc_;
  GateInput gate_input_;
  CalibrationData* calibration_data_;
  bool capture_button_flag_;
  
  float smoothed_adc_value_[ADC_CHANNELS_TOTAL];
  static CvTransformation transformations_[ADC_CHANNELS_TOTAL];
  
  float note_;
  float cv_c1_;

  float output_level_; // Added for better VU meter control.
  
  bool previous_capture_[kAdcLatency];
  bool previous_gate_[kAdcLatency];
  
  DISALLOW_COPY_AND_ASSIGN(CvScaler);
};

}  // namespace clouds

#endif  // CLOUDS_CV_SCALER_H_
