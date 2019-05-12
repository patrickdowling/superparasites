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
// User interface

#ifndef CLOUDS_UI_H_
#define CLOUDS_UI_H_

#include "stmlib/stmlib.h"

#include "stmlib/ui/event_queue.h"

#include "supercell/drivers/leds.h"
#include "supercell/drivers/switches.h"
#include "supercell/drivers/dac.h"

namespace clouds {

enum UiMode {
  UI_MODE_VU_METER,
  UI_MODE_BLEND_METER,
  UI_MODE_QUALITY,
  UI_MODE_BLENDING,
  UI_MODE_PLAYBACK_MODE,
  UI_MODE_LOAD,
  UI_MODE_SAVE,
  UI_MODE_PANIC,
  UI_MODE_CALIBRATION_1,
  UI_MODE_CALIBRATION_2,
  UI_MODE_SAVING,
  UI_MODE_SPLASH,
  UI_MODE_LAST
};

enum SwitchIndex {
  SWITCH_MODE,
  SWITCH_WRITE,
  SWITCH_CAPTURE,
  SWITCH_FREEZE,
  SWITCH_MUTE_OUT,
  SWITCH_MUTE_IN
};

enum FactoryTestingCommand {
  FACTORY_TESTING_READ_POT,
  FACTORY_TESTING_READ_CV,
  FACTORY_TESTING_READ_GATE,
  FACTORY_TESTING_SET_BYPASS,
  FACTORY_TESTING_CALIBRATE
};

class GranularProcessor;
class CvScaler;
class Meter;
class Settings;

class Ui {
 public:
  Ui() { }
  ~Ui() { }
  
  void Init(
      Settings* settings,
      CvScaler* cv_scaler,
      GranularProcessor* processor,
      Meter* inmeter,
      Meter* outmeter);
  void Poll();
  void DoEvents();
  void FlushEvents();
  void Panic() {
    mode_ = UI_MODE_PANIC;
  }
  
  
  uint8_t HandleFactoryTestingRequest(uint8_t command);
  
 private:
  void OnSwitchPressed(const stmlib::Event& e);
  void OnSwitchReleased(const stmlib::Event& e);
  void OnSecretHandshake();
  
  void CalibrateC1();
  void CalibrateC3();

  void SaveState();
  void PaintLeds();
  void LoadSampleMemory();
  void SaveSampleMemory();

  stmlib::EventQueue<16> queue_;

  Settings* settings_;
  CvScaler* cv_scaler_;
  Dac dac_; // For modulation output
  
  Leds leds_;
  Switches switches_;
  uint32_t press_time_[kNumSwitches];
  uint32_t long_press_time_[kNumSwitches];
  uint32_t quality_changed_time_;
  UiMode mode_;

  uint32_t blink_;
  uint32_t fade_;
  bool skip_first_cal_press_;
  
  GranularProcessor* processor_;
  Meter* inmeter_;
  Meter* outmeter_;
  // Additional Data for UI behavior,
  //   and additional edit functions.
  uint8_t load_save_location_;
  uint8_t last_load_save_location_;
  float press_pan_pos_;
  float noise_freq_;
  bool noise_state_;
  bool tracking_noise_ctrl_;
  bool save_alt_menu_;
  bool mode_alt_menu_;
  uint32_t save_menu_time_;
  uint32_t mode_menu_time_;
  DISALLOW_COPY_AND_ASSIGN(Ui);
};

}  // namespace clouds

#endif  // CLOUDS_UI_H_
