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
// User interface.

#include "supercell/ui.h"

#include "stmlib/system/system_clock.h"

#include "supercell/dsp/granular_processor.h"
#include "supercell/cv_scaler.h"
#include "supercell/meter.h"

namespace clouds {

const int32_t kLongPressDuration = 1000;
const int32_t kVeryLongPressDuration = 4000;

const int32_t kQualityDurations[] = {
    1000, 2000, 4000, 8000
};

using namespace stmlib;

void Ui::Init(
    Settings* settings,
    CvScaler* cv_scaler,
    GranularProcessor* processor,
    Meter* inmeter,
    Meter* outmeter) {
  settings_ = settings;
  cv_scaler_ = cv_scaler;
  leds_.Init();
  switches_.Init();
  
  processor_ = processor;
  inmeter_ = inmeter;
  outmeter_ = outmeter;
  if (!switches_.pressed_immediate(SWITCH_CAPTURE)) {
    mode_ = UI_MODE_CALIBRATION_1;
  } else {
    mode_ = UI_MODE_SPLASH;
  }

  dac_.Init(); 
  //dac_.SetNoiseFreq(0.99f);
  const State& state = settings_->state();
  blink_ = 0; 
  fade_ = 0;
  skip_first_cal_press_ = true;
  // Alt menu indicators
  save_alt_menu_ = false;
  mode_alt_menu_ = false;
  save_menu_time_ = 0;
  mode_menu_time_ = 0;
  // Sanitize saved settings.
  processor_->set_quality(state.quality & 3);
  processor_->set_playback_mode(
      static_cast<PlaybackMode>(state.playback_mode % PLAYBACK_MODE_LAST));
  noise_freq_ = state.random_rate;
  noise_state_ = state.random_state;
  dac_.SetNoiseFreq(noise_freq_);
  if (noise_state_)
    dac_.StartNoise();
  else
    dac_.StopNoise();
}

void Ui::SaveState() {
  State* state = settings_->mutable_state();
  state->quality = processor_->quality();
  state->playback_mode = processor_->playback_mode();
  state->random_rate = noise_freq_;
  state->random_state = dac_.is_generating_noise();
  settings_->Save();
}

void Ui::Poll() {
  system_clock.Tick();
  switches_.Debounce();
  for (uint8_t i = 0; i < kNumSwitches; ++i) {
    if (switches_.just_pressed(i)) {
      queue_.AddEvent(CONTROL_SWITCH, i, 0);
      press_time_[i] = system_clock.milliseconds();
      long_press_time_[i] = system_clock.milliseconds();
    }
    if (switches_.pressed(i) && press_time_[i] != 0) {
      int32_t pressed_time = system_clock.milliseconds() - press_time_[i];
      if (pressed_time > kLongPressDuration) {
        queue_.AddEvent(CONTROL_SWITCH, i, pressed_time);
        press_time_[i] = 0;
      }
    }
    if (switches_.pressed(i) && long_press_time_[i] != 0) {
      int32_t pressed_time = system_clock.milliseconds() - long_press_time_[i];
      if (pressed_time > kVeryLongPressDuration) {
        queue_.AddEvent(CONTROL_SWITCH, i, pressed_time);
        long_press_time_[i] = 0;
      }
    }
    
    if (switches_.released(i) && press_time_[i] != 0) {
      queue_.AddEvent(
          CONTROL_SWITCH,
          i,
          system_clock.milliseconds() - press_time_[i] + 1);
      press_time_[i] = 0;
    }
  }
  // Individual Timeouts for the save/mode alt menus
  if (save_alt_menu_ && system_clock.milliseconds() - save_menu_time_ > 5000) {
    save_alt_menu_ = false;
    load_save_location_ = last_load_save_location_;
  }
  if (mode_alt_menu_ && system_clock.milliseconds() - mode_menu_time_ > 5000) {
    mode_alt_menu_ = false;
  }
  // Rate Adjustment of Aux Random -- value updated on release
  if (switches_.pressed(SWITCH_CAPTURE) && fabsf(cv_scaler_->pan_pot() - press_pan_pos_) > 0.05f) {
        //noise_freq_ = cv_scaler_->pan_pot();
        tracking_noise_ctrl_ = true;
  }
  // This needs to happen even if the button has been held for a while
  // a second release event doesn't seem to get thrown if the button is held
  // for a while (which is ideal for every other action).
  if (switches_.released(SWITCH_CAPTURE) && tracking_noise_ctrl_) {
    tracking_noise_ctrl_ = false;
    dac_.SetNoiseFreq(noise_freq_);
    SaveState();
  }
  if (tracking_noise_ctrl_) {
        noise_freq_ = cv_scaler_->pan_pot();
  }

  // Added for resetting animations
  blink_ += 1;
  fade_ += 1;
  PaintLeds();
}

void Ui::PaintLeds() {
  leds_.Clear();
  uint32_t clock = system_clock.milliseconds();
  // legacy blink and fade
  bool blink = (clock & 127) > 64;
  bool flash = (clock & 511) < 16;
  uint8_t fade = (clock >> 1);
  // resettable blink and fade
  blink = (blink_ & 127) > 64;
  //fade = fade_ % 511;
  fade = (fade_ >> 1);
  fade = static_cast<uint16_t>(fade) * fade >> 8;
  switch (mode_) {
    case UI_MODE_SPLASH:
      {
        uint8_t index = 7 - (((clock >> 8)) & 7);
        leds_.set_intensity(index, fade);
      }
      break;
      
    case UI_MODE_VU_METER:
    case UI_MODE_SAVE:
    case UI_MODE_PLAYBACK_MODE:
    case UI_MODE_QUALITY:
      {
          float out_scalar = (cv_scaler_->output_level() * cv_scaler_->output_level()) * 2.1f; 
          if (out_scalar < 0.15f) {
            out_scalar = 0.0f;
          }
          int32_t output = lut_db[static_cast<int32_t>(out_scalar * static_cast<float>(outmeter_->peak()))>>7];
          //int32_t output = lut_db[outmeter_->peak() >> 5]; // LEFT IN PLACE FOR TESTING PURE OUTPUT LEVEL
          leds_.PaintBar(lut_db[inmeter_->peak() >> 7], \
            output, \
            processor_->mute_in(), \
            processor_->mute_out() \
            );
          uint8_t bright;
          for (uint8_t i = 0; i < 4; i++) {
            if (!mode_alt_menu_) {
                bright = i == processor_->quality() ? 255 : 0;
                int32_t changed_time = clock - quality_changed_time_;
                if (bright == 255) {
                    if (quality_changed_time_ != 0 && changed_time < kQualityDurations[i]) {
                        bright = blink == true ? 255 : 0;
                    } else {
                        quality_changed_time_ = 0;
                    }
                }
                leds_.set_status(15 - i, bright);
            } else {
              // TODO[pld] This is horribly inefficient, but there might be side-effects to adding a separate case
              uint8_t mode = processor_->playback_mode() & 0x3;
              if (processor_->playback_mode() < 4) {
                leds_.set_status(15-i, i == mode ? fade : 0);
              } else {
                leds_.set_status(15-i, i == mode ? 0 : fade);
              }
            }
            if (save_alt_menu_) {
                bright = i == load_save_location_ ? fade : 0;
            } else {
                bright = i == load_save_location_ ? 255 : 0;
            }
            leds_.set_status(11 - i, bright);
          }
      }
      break;
    case UI_MODE_LOAD:
      break;  
    case UI_MODE_SAVING:
      leds_.set_status(load_save_location_ << 4, 255);  // shift from 0..3(BUF) to 4..7(MEM)
      break;

    case UI_MODE_CALIBRATION_1:
      leds_.set_status(7, blink ? 255 : 0);
      leds_.set_status(6, blink ? 255 : 0);
      leds_.set_status(5, blink ? 255 : 0);
      leds_.set_status(4, blink ? 255 : 0);
      break;
    case UI_MODE_CALIBRATION_2:
      leds_.set_status(7, blink ? 255 : 0);
      leds_.set_status(6, blink ? 255 : 0);
      leds_.set_status(5, blink ? 255 : 0);
      leds_.set_status(4, blink ? 255 : 0);
      leds_.set_status(3, blink ? 255 : 0);
      leds_.set_status(2, blink ? 255 : 0);
      leds_.set_status(1, blink ? 255 : 0);
      break;
      
    case UI_MODE_PANIC:
      leds_.set_status(0, 255); //1st from BUF section
      leds_.set_status(4, 255); //1st from MEM section
      leds_.set_status(8, 255); //1st from VU section
      break;

    default:
      break;
  }

  bool freeze = processor_->frozen();
  if (processor_->reversed()) {
    freeze ^= flash;
  }
  // Testing Edit Function 
  if (tracking_noise_ctrl_) {
    leds_.set_freeze(blink);
  } else {
    leds_.set_freeze(freeze);
  }
  //leds_.set_freeze(processor_->frozen());
  if (processor_->bypass()) {
    int32_t output = static_cast<int32_t>(cv_scaler_->output_level() * 65535.0f);
    leds_.PaintBar(lut_db[inmeter_->peak() >> 7], output, processor_->mute_in(), processor_->mute_out());
    leds_.set_freeze(true);
  }
  
  leds_.Write();
}

void Ui::FlushEvents() {
  queue_.Flush();
}

void Ui::OnSwitchPressed(const Event& e) {
  if (e.control_id == SWITCH_CAPTURE) {
    cv_scaler_->set_capture_flag(); 
    // Take note of pan position
    press_pan_pos_ = cv_scaler_->pan_pot();
  }
}

void Ui::CalibrateC1() {
  cv_scaler_->CalibrateC1();
  cv_scaler_->CalibrateOffsets();
  mode_ = UI_MODE_CALIBRATION_2;
}

void Ui::CalibrateC3() {
  bool success = cv_scaler_->CalibrateC3();
  if (success) {
    settings_->Save();
    mode_ = UI_MODE_VU_METER;
  } else {
    mode_ = UI_MODE_PANIC;
  }
}

void Ui::OnSecretHandshake() {
  //mode_ = UI_MODE_PLAYBACK_MODE;
  mode_alt_menu_ = true;
  mode_menu_time_ = system_clock.milliseconds();
  fade_ = 0;
}

void Ui::OnSwitchReleased(const Event& e) {
  switch (e.control_id) {
    case SWITCH_FREEZE:
      if (e.data >= kVeryLongPressDuration) {
      } else if (e.data >= kLongPressDuration) {
        processor_->ToggleReverse();
      } else {
        processor_->ToggleFreeze();
      }
      break;
    case SWITCH_CAPTURE:
      if (e.data >= kLongPressDuration) {
        //mode_ = UI_MODE_CALIBRATION_1; // now checked in Init()
      } else if (mode_ == UI_MODE_CALIBRATION_1) {
        if (!skip_first_cal_press_) {
            CalibrateC1();
        } else {
            skip_first_cal_press_ = false;
        }
      } else if (mode_ == UI_MODE_CALIBRATION_2) {
        CalibrateC3();
      }
      break;

    case SWITCH_MODE:
      if (e.data >= kVeryLongPressDuration) {
      } else if (e.data > kLongPressDuration) {
      } else if (mode_ == UI_MODE_VU_METER || mode_ == UI_MODE_QUALITY) {
        if (mode_alt_menu_) {
            uint8_t mode = (processor_->playback_mode() + 1) % PLAYBACK_MODE_LAST;
            mode_menu_time_ = system_clock.milliseconds();
            processor_->set_playback_mode(static_cast<PlaybackMode>(mode));
            SaveState();
        } else {
            processor_->set_quality((processor_->quality() + 1) & 3);
            quality_changed_time_ = system_clock.milliseconds();
            blink_ = 0;
            SaveState();
        }
      } else if (mode_ == UI_MODE_PLAYBACK_MODE) {
      // left here in case the new menu_ members are broken
      }
      break;
    case SWITCH_WRITE:
      if (e.data >= kLongPressDuration) {
        if (save_alt_menu_) {
            mode_ = UI_MODE_VU_METER; // Return to main UI
            save_alt_menu_ = false;
            PersistentBlock blocks[4];
            size_t num_blocks = 0;
            // Perform the save operation to the currently selected location
            // Silence the processor during the long erase/write.
            processor_->set_silence(true);
            system_clock.Delay(5);
            processor_->PreparePersistentData();
            processor_->GetPersistentData(blocks, &num_blocks);
            settings_->SaveSampleMemory(load_save_location_, blocks, num_blocks);
            processor_->set_silence(false);

            processor_->LoadPersistentData(settings_->sample_flash_data(
                load_save_location_));
            // Update Persistent Bank Selection
            last_load_save_location_ = load_save_location_;
        } else {
            // Enter UI_MODE_SAVE
            save_alt_menu_ = true;
            save_menu_time_ = system_clock.milliseconds();
            fade_ = 0;
            // Set Persistent Bank Selection
            last_load_save_location_ = load_save_location_;
        }
      } else {
        if (switches_.pressed(SWITCH_CAPTURE)) {
            processor_->LoadPersistentData(settings_->sample_flash_data(
                load_save_location_));
        } else {
            load_save_location_ = (load_save_location_ + 1) & 3;
            if (!save_alt_menu_) {
                processor_->LoadPersistentData(settings_->sample_flash_data(
                    load_save_location_));
            } else {
                save_menu_time_ = system_clock.milliseconds();
            }
        }
      }
      break;
    case SWITCH_MUTE_OUT:
        {
            if (e.data < kLongPressDuration) {
                bool mute_state = processor_->mute_out();
                processor_->set_mute_out(!mute_state);
            } else if (e.data >= kVeryLongPressDuration) {
                if (switches_.pressed(SWITCH_MUTE_IN)) {
                    bool dac_state = dac_.is_generating_noise();
                    if (dac_state) {
                        dac_.StopNoise();
                    } else {
                        dac_.StartNoise();
                    }
                    SaveState();
                }
            }
        }
        break;
    case SWITCH_MUTE_IN:
        {
            if (e.data < kLongPressDuration) {
                bool mute_state = processor_->mute_in();
                processor_->set_mute_in(!mute_state);
            }
        }
        break;
    default:
      break;
  }
}

void Ui::DoEvents() {
  while (queue_.available()) {
    Event e = queue_.PullEvent();
    if (e.control_type == CONTROL_SWITCH) {
      if (e.data == 0) {
        OnSwitchPressed(e);
      } else {
        if (e.data >= kLongPressDuration &&
            e.control_id == SWITCH_MODE) {// && 
          OnSecretHandshake();
        } else {
          OnSwitchReleased(e);
        }
      }
    }
  }

  if ((queue_.idle_time() > 2000 && mode_ == UI_MODE_SPLASH) ||
      (queue_.idle_time() > 1000 && mode_ == UI_MODE_PANIC)) {
    queue_.Touch();
    mode_ = UI_MODE_VU_METER;
  }

  if (queue_.idle_time() > 5000) {
    queue_.Touch();
    if (mode_ == UI_MODE_BLENDING || mode_ == UI_MODE_QUALITY ||
        mode_ == UI_MODE_PLAYBACK_MODE || 
        mode_ == UI_MODE_LOAD || mode_ == UI_MODE_BLEND_METER ||
        mode_ == UI_MODE_SPLASH || mode_ == UI_MODE_SAVE) {
        mode_ = UI_MODE_VU_METER;
        mode_alt_menu_ = false;
        save_alt_menu_ = false;
    } 
  }
}

uint8_t Ui::HandleFactoryTestingRequest(uint8_t command) {
  uint8_t argument = command & 0x1f;
  command = command >> 5;
  uint8_t reply = 0;
  switch (command) {
    case FACTORY_TESTING_READ_POT:
    case FACTORY_TESTING_READ_CV:
      reply = cv_scaler_->adc_value(argument);
      break;
      
    case FACTORY_TESTING_READ_GATE:
      if (argument <= 2) {
        return switches_.pressed(argument);
      } else {
        return cv_scaler_->gate(argument - 3);
      }
      break;
      
    case FACTORY_TESTING_SET_BYPASS:
      processor_->set_bypass(argument);
      break;
      
    case FACTORY_TESTING_CALIBRATE:
      if (argument == 0) {
        mode_ = UI_MODE_CALIBRATION_1;
      } else if (argument == 1) {
        CalibrateC1();
      } else {
        CalibrateC3();
        //cv_scaler_->set_blend_parameter(static_cast<BlendParameter>(0));
        SaveState();
      }
      break;
    default:
      break;
  }
  return reply;
}

}  // namespace clouds
