// Copyright 2013 Olivier Gillet.
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

#include <stm32f10x_conf.h>

#include "stmlib/system/system_clock.h"
#include "stmlib/utils/random.h"

#include "frames/drivers/dac.h"
#include "frames/drivers/system.h"
#include "frames/drivers/trigger_output.h"
#include "frames/keyframer.h"
#include "frames/poly_lfo.h"
#include "frames/ui.h"

using namespace frames;
using namespace stmlib;

Dac dac;
Keyframer keyframer;
PolyLfo poly_lfo;
System sys;
TriggerOutput trigger_output;
Ui ui;

// Default interrupt handlers.
extern "C" {

void HardFault_Handler() { while (1); }
void MemManage_Handler() { while (1); }
void BusFault_Handler() { while (1); }
void UsageFault_Handler() { while (1); }
void NMI_Handler() { }
void SVC_Handler() { }
void DebugMon_Handler() { }
void PendSV_Handler() { }

}

extern "C" {

void SysTick_Handler() {
  system_clock.Tick();  // Tick global ms counter.
  ui.Poll();
}

volatile uint16_t refresh = 0;
static uint16_t factory_testing_timer = 0;
static int16_t previous_position = -2;
static int16_t previous_nearest_keyframe = -2;
static uint16_t pulse_counter;
static bool can_fire_trigger = false;
static const uint16_t kPulseDuration = 128;

void TIM1_UP_IRQHandler(void) {
  if (TIM_GetITStatus(TIM1, TIM_IT_Update) == RESET) {
    return;
  }
  TIM_ClearITPendingBit(TIM1, TIM_IT_Update);
  
  dac.Update();
  if (dac.ready()) {
    ++refresh;
  }
  
  if (!ui.sequencer_mode()) {
    int16_t position = keyframer.position();
    if (previous_position != position) {
      previous_position = position;
      if (can_fire_trigger) {
        pulse_counter = kPulseDuration;
        trigger_output.High();
        can_fire_trigger = false;
      }
    }
  }
  
  int16_t nearest_keyframe = keyframer.nearest_keyframe();
  if (previous_nearest_keyframe != nearest_keyframe) {
    previous_nearest_keyframe = nearest_keyframe;
    can_fire_trigger = true;
  }

  if (ui.mode() == UI_MODE_FACTORY_TESTING) {
    ++factory_testing_timer;
    if (factory_testing_timer == 1280) {
      pulse_counter = kPulseDuration;
      trigger_output.High();
      factory_testing_timer = 0;
    }
  }
  
  if (pulse_counter) {
    --pulse_counter;
    if (!pulse_counter) {
      trigger_output.Low();
    }
  }
}

}

void Init() {
  sys.Init(F_CPU / 128000 - 1, true);
  dac.Init();
  trigger_output.Init();
  trigger_output.Low();
  keyframer.Init();
  poly_lfo.Init();
  ui.Init(&keyframer, &poly_lfo);
  sys.StartTimers();
}

inline uint16_t fold_add(uint16_t a, int16_t b) {
  if (a > 0 && b > 65535 - a) {
    return 65535 - a - b - 1;
  } else if (b < 0 && a < - b) {
    return 65535 - a - b + 1;
  } else {
    return a + b;
  }
}

int32_t lp_frame = 0;

int main(void) {
  Init();
  
  while (ui.mode() == UI_MODE_SPLASH) {
    ui.DoEvents();
  }
  
  ui.TryCalibration();
  
  bool trigger_detector_armed = false;
  int32_t dc_offset_frame_modulation = keyframer.dc_offset_frame_modulation();
  int32_t clock_counter = 0;

  while (1) {
    ui.DoEvents();
    
    if (refresh) {
      --refresh;
      int32_t frame = ui.frame();
      int32_t frame_modulation = \
          (ui.frame_modulation() - dc_offset_frame_modulation) << 1;
      frame += frame_modulation;
      if (ui.poly_lfo_mode()) {
        poly_lfo.Render(frame);
        dac.Write(0, poly_lfo.dac_code(0));
        dac.Write(1, poly_lfo.dac_code(1));
        dac.Write(2, poly_lfo.dac_code(2));
        dac.Write(3, poly_lfo.dac_code(3));
      } else {
        if (frame < 0) {
          frame = 0;
        } else if (frame > 65535) {
          frame = 65535;
        }

        if (ui.sequencer_mode()) {
          // Detect a trigger on the FRAME input.
          if (frame_modulation < 21845) {
            trigger_detector_armed = true;
          }

          // on clock
          if (frame_modulation > 43690 && trigger_detector_armed) {
            trigger_detector_armed = false;
            clock_counter++;

            // shift the register and feed it back
            if (ui.shift_divider > 0 &&
                clock_counter % ui.shift_divider == 0) {
              uint16_t t = ui.shift_register[3];
              ui.shift_register[3] = ui.shift_register[2];
              ui.shift_register[2] = ui.shift_register[1];
              ui.shift_register[1] = ui.shift_register[0];
              ui.shift_register[0] = static_cast<uint8_t>(Random::GetWord()) > ui.random_level ?
                t : fold_add(t,
                             static_cast<int16_t>(Random::GetWord()) / 255 * ui.random_level);
              // trigger
              pulse_counter = kPulseDuration;
              trigger_output.High();
            }

            // place in register and go to next sequencer step
            if (ui.mode() != UI_MODE_EDIT_EASING ||
                (ui.step_divider > 0 &&
                 clock_counter % ui.step_divider == 0)) {
              ui.shift_register[0] = keyframer.level(0);
              int32_t max_step = ui.mode() == UI_MODE_EDIT_RESPONSE ?
                (keyframer.num_keyframes() * ui.frame() / 65536) + 1 :
                keyframer.num_keyframes();
              ui.sequencer_step = (ui.sequencer_step + 1) % max_step;
              // trigger
              pulse_counter = kPulseDuration;
              trigger_output.High();
            }

            // output a trigger when sequence resets
            if (ui.mode() != UI_MODE_EDIT_EASING &&
                ui.sequencer_step == 0) {
              pulse_counter = kPulseDuration;
              trigger_output.High();
            }
          }

          frame = keyframer.keyframe(ui.sequencer_step).timestamp;
        } else {
          lp_frame += (frame - lp_frame) >> 6;
          frame = lp_frame;
        }

        keyframer.Evaluate(frame);

        if (ui.sequencer_mode() && ui.mode() == UI_MODE_EDIT_EASING) {
          // shift register mode
          dac.Write(0, Keyframer::ConvertToDacCode(ui.shift_register[0], 0));
          dac.Write(1, Keyframer::ConvertToDacCode(ui.shift_register[1], 0));
          dac.Write(2, Keyframer::ConvertToDacCode(ui.shift_register[2], 0));
          dac.Write(3, Keyframer::ConvertToDacCode(ui.shift_register[3], 0));
        } else {
          // sequencer or keyframer mode
          dac.Write(0, keyframer.dac_code(0));
          dac.Write(1, keyframer.dac_code(1));
          dac.Write(2, keyframer.dac_code(2));
          dac.Write(3, keyframer.dac_code(3));
        }
      }
    }
  }
}
