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

// Shensley Supercell specific notes:
// - LEDs for VU meter are connected to TIM3/4 output pins, allowing them to be hardware PWM controlled.
// - The LEDs for Bank/Time/Hold are all configured to pure GPIO, and will have to have 
//     a more brute-force software PWM (similar to original clouds) implemented.

#include "supercell/drivers/leds.h"

#include <algorithm>

namespace clouds {
  
using namespace std;
  

const uint16_t kPinsTime[4] = { GPIO_Pin_0, GPIO_Pin_1, GPIO_Pin_2, GPIO_Pin_3 };
const uint16_t kPinsBank[4] = { GPIO_Pin_4, GPIO_Pin_5, GPIO_Pin_6, GPIO_Pin_7 };
const uint16_t kPinsVUIn[4] = { GPIO_Pin_12, GPIO_Pin_13, GPIO_Pin_14, GPIO_Pin_15 };
const uint16_t kPinsVUOut[4] = { GPIO_Pin_9, GPIO_Pin_8, GPIO_Pin_7, GPIO_Pin_6 };
const uint16_t kPinFreezeLed = GPIO_Pin_10;


void Leds::Init() {
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD, ENABLE);
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);
  // enable periph clocks for PWM timers.
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE);
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);
  
  GPIO_InitTypeDef gpio_init;
  
  // Generic Output Pins
  gpio_init.GPIO_Mode = GPIO_Mode_OUT;
  gpio_init.GPIO_OType = GPIO_OType_PP;
  gpio_init.GPIO_Speed = GPIO_Speed_25MHz;
  gpio_init.GPIO_PuPd = GPIO_PuPd_NOPULL;

  gpio_init.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2 | GPIO_Pin_3; // Time LEDs
  gpio_init.GPIO_Pin |= GPIO_Pin_4 | GPIO_Pin_5 | GPIO_Pin_6 | GPIO_Pin_7; // Bank LEDs
  gpio_init.GPIO_Pin |= GPIO_Pin_10; // Hold LED
  GPIO_Init(GPIOD, &gpio_init);
  //gpio_init.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7 | GPIO_Pin_8 | GPIO_Pin_9; // VU Meter OUT 
  //GPIO_Init(GPIOC, &gpio_init);

  // TODO: Setup PWM timers/outputs.
  //gpio_init.GPIO_Mode = GPIO_Mode_AF;
  gpio_init.GPIO_Mode = GPIO_Mode_OUT;
  gpio_init.GPIO_OType = GPIO_OType_PP;
  gpio_init.GPIO_Speed = GPIO_Speed_100MHz;
  gpio_init.GPIO_PuPd = GPIO_PuPd_NOPULL;
  gpio_init.GPIO_Pin = GPIO_Pin_12 | GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_15; // VU Meter IN
  GPIO_Init(GPIOD, &gpio_init);
  gpio_init.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7 | GPIO_Pin_8 | GPIO_Pin_9; // VU Meter OUT 
  GPIO_Init(GPIOC, &gpio_init);

  /*
  GPIO_PinAFConfig(GPIOD, GPIO_PinSource12, GPIO_AF_TIM4);
  GPIO_PinAFConfig(GPIOD, GPIO_PinSource13, GPIO_AF_TIM4);
  GPIO_PinAFConfig(GPIOD, GPIO_PinSource14, GPIO_AF_TIM4);
  GPIO_PinAFConfig(GPIOD, GPIO_PinSource15, GPIO_AF_TIM4);
  GPIO_PinAFConfig(GPIOC, GPIO_PinSource6, GPIO_AF_TIM3);
  GPIO_PinAFConfig(GPIOC, GPIO_PinSource7, GPIO_AF_TIM3);
  GPIO_PinAFConfig(GPIOC, GPIO_PinSource8, GPIO_AF_TIM3);
  GPIO_PinAFConfig(GPIOC, GPIO_PinSource9, GPIO_AF_TIM3);

  // Configure Timers
  TIM_TimeBaseInitTypeDef TIM_BaseStruct;
  TIM_BaseStruct.TIM_Prescaler = 0;
  TIM_BaseStruct.TIM_CounterMode = TIM_CounterMode_Up;
  TIM_BaseStruct.TIM_Period = 839;
  TIM_BaseStruct.TIM_ClockDivision = TIM_CKD_DIV1;
  TIM_BaseStruct.TIM_RepetitionCounter = 0;
  TIM_TimeBaseInit(TIM3, &TIM_BaseStruct);
  TIM_TimeBaseInit(TIM4, &TIM_BaseStruct);

  TIM_Cmd(TIM3, ENABLE);
  TIM_Cmd(TIM4, ENABLE);
  
  // Initialize OC (PWM) Output Pins
  TIM_OCInitTypeDef TIM_OCStruct;
  TIM_OCStruct.TIM_OCMode = TIM_OCMode_PWM2;
  TIM_OCStruct.TIM_OutputState = TIM_OutputState_Enable;
  TIM_OCStruct.TIM_OCPolarity = TIM_OCPolarity_Low;

  // Pulse is 0-839 for brightness. Yeah, stupid range, I know. But it's 1kHz pwm
  TIM_OCStruct.TIM_Pulse = 150;
  // TIM4
  TIM_OC1Init(TIM4, &TIM_OCStruct);
  TIM_OC1PreloadConfig(TIM4, TIM_OCPreload_Enable);
  TIM_OC2Init(TIM4, &TIM_OCStruct);
  TIM_OC2PreloadConfig(TIM4, TIM_OCPreload_Enable);
  TIM_OC3Init(TIM4, &TIM_OCStruct);
  TIM_OC3PreloadConfig(TIM4, TIM_OCPreload_Enable);
  TIM_OC4Init(TIM4, &TIM_OCStruct);
  TIM_OC4PreloadConfig(TIM4, TIM_OCPreload_Enable);
  // TIM3
  TIM_OC1Init(TIM3, &TIM_OCStruct);
  TIM_OC1PreloadConfig(TIM3, TIM_OCPreload_Enable);
  TIM_OC2Init(TIM3, &TIM_OCStruct);
  TIM_OC2PreloadConfig(TIM3, TIM_OCPreload_Enable);
  TIM_OC3Init(TIM3, &TIM_OCStruct);
  TIM_OC3PreloadConfig(TIM3, TIM_OCPreload_Enable);
  TIM_OC4Init(TIM3, &TIM_OCStruct);
  TIM_OC4PreloadConfig(TIM3, TIM_OCPreload_Enable);
  */

  Clear();
  pwm_counter_ = 0;
}

void Leds::Write() {
  // Pack data.
  /* 
  // Software PWM
  pwm_counter_ += 32;
  uint16_t data = 0;
  for (uint16_t i = 0; i < 16; ++i) {
    data <<= 1;
    if (led_[i] && led_[i] >= pwm_counter_) {
      data |= 0x1;
    }
  }
  */
  // Pack data.
  uint16_t static_outs[16];
  pwm_counter_ += 32;
  for (uint8_t i = 0; i < 16; i++)  {
  /*
    if (i < 8) {
        static_outs[i] = led_[i] * 3;
    } else {
    }
    */
    //static_outs[i] = led_[i] > 0 ? 1 : 0;
    if (led_[i] && led_[i] >= pwm_counter_) {
        static_outs[i] = 1;
    } else {
        static_outs[i] = 0;
    }
  }
  GPIO_WriteBit(GPIOD, kPinsTime[0], static_cast<BitAction>(static_outs[15]));
  GPIO_WriteBit(GPIOD, kPinsTime[1], static_cast<BitAction>(static_outs[14]));
  GPIO_WriteBit(GPIOD, kPinsTime[2], static_cast<BitAction>(static_outs[13]));
  GPIO_WriteBit(GPIOD, kPinsTime[3], static_cast<BitAction>(static_outs[12]));
  GPIO_WriteBit(GPIOD, kPinsBank[0], static_cast<BitAction>(static_outs[11]));
  GPIO_WriteBit(GPIOD, kPinsBank[1], static_cast<BitAction>(static_outs[10]));
  GPIO_WriteBit(GPIOD, kPinsBank[2], static_cast<BitAction>(static_outs[9]));
  GPIO_WriteBit(GPIOD, kPinsBank[3], static_cast<BitAction>(static_outs[8]));
  GPIO_WriteBit(GPIOD, kPinFreezeLed, static_cast<BitAction>(freeze_led_));

  GPIO_WriteBit(GPIOD, kPinsVUIn[0], static_cast<BitAction>(static_outs[7]));
  GPIO_WriteBit(GPIOD, kPinsVUIn[1], static_cast<BitAction>(static_outs[6]));
  GPIO_WriteBit(GPIOD, kPinsVUIn[2], static_cast<BitAction>(static_outs[5]));
  GPIO_WriteBit(GPIOD, kPinsVUIn[3], static_cast<BitAction>(static_outs[4]));
  GPIO_WriteBit(GPIOC, kPinsVUOut[0], static_cast<BitAction>(static_outs[3]));
  GPIO_WriteBit(GPIOC, kPinsVUOut[1], static_cast<BitAction>(static_outs[2]));
  GPIO_WriteBit(GPIOC, kPinsVUOut[2], static_cast<BitAction>(static_outs[1]));
  GPIO_WriteBit(GPIOC, kPinsVUOut[3], static_cast<BitAction>(static_outs[0]));
  /*
  TIM_SetCompare1(TIM3, static_outs[4]);
  TIM_SetCompare2(TIM3, static_outs[5]);
  TIM_SetCompare3(TIM3, static_outs[6]);
  TIM_SetCompare4(TIM3, static_outs[7]);
  TIM_SetCompare1(TIM4, static_outs[3]);
  TIM_SetCompare2(TIM4, static_outs[2]);
  TIM_SetCompare3(TIM4, static_outs[1]);
  TIM_SetCompare4(TIM4, static_outs[0]);
  */

  // Set PWM Outputs

  // Shift out.
  /*
  GPIO_WriteBit(GPIOC, kPinEnable, Bit_RESET);
  for (uint16_t i = 0; i < 16; ++i) {
    GPIO_WriteBit(GPIOC, kPinClk, Bit_RESET);
    if (data & 0x8000) {
      GPIO_WriteBit(GPIOC, kPinData, Bit_SET);
    } else {
      GPIO_WriteBit(GPIOC, kPinData, Bit_RESET);
    }
    data <<= 1;
    GPIO_WriteBit(GPIOC, kPinClk, Bit_SET);
  }
  GPIO_WriteBit(GPIOC, kPinEnable, Bit_SET);
  GPIO_WriteBit(GPIOC, kPinFreezeLed, static_cast<BitAction>(freeze_led_));
  */
}

void Leds::Clear() {
  fill(&led_[0], &led_[15], 0);
  //fill(&green_[0], &green_[4], 0);
  freeze_led_ = false;
}

void Leds::Test(bool state) {
    // BLINK ALL LEDs
    GPIO_WriteBit(GPIOC, GPIO_Pin_6, static_cast<BitAction>(state));

    GPIO_WriteBit(GPIOD, GPIO_Pin_0, static_cast<BitAction>(state));
}

}  // namespace clouds
