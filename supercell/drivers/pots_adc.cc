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
// Driver for ADC2 - used for scanning pots.

#include "supercell/drivers/pots_adc.h"

#include <stm32f4xx_conf.h>

namespace clouds {

/* static */
uint8_t PotsAdc::addresses_[ADC_CHANNEL_POTENTIOMETER_LAST] = {
  
  4, //position pot
  6, //pitch pot
  7, //density pot
  5, //texture pot
  3, //balance pot
  0, //spread pot
  1, //feedback pot
  2, //reverb pot
};
  /*28,  // POT_EXCITER_ENVELOPE_SHAPE,
  30,  // POT_EXCITER_BOW_LEVEL,
  5,  // POT_EXCITER_BOW_TIMBRE,
  1,  // POT_EXCITER_BOW_TIMBRE_ATTENUVERTER,
  29,  // POT_EXCITER_BLOW_LEVEL,
  31,  // POT_EXCITER_BLOW_META,
  2,  // POT_EXCITER_BLOW_META_ATTENUVERTER,
  7,  // POT_EXCITER_BLOW_TIMBRE,
  11,  // POT_EXCITER_BLOW_TIMBRE_ATTENUVERTER,
  22,  // POT_EXCITER_STRIKE_LEVEL,
  20,  // POT_EXCITER_STRIKE_META,
  8,  // POT_EXCITER_STRIKE_META_ATTENUVERTER,
  6,  // POT_EXCITER_STRIKE_TIMBRE,
  9,  // POT_EXCITER_STRIKE_TIMBRE_ATTENUVERTER,
  23,  // POT_RESONATOR_COARSE,
  26,  // POT_RESONATOR_FINE,
  25,  // POT_RESONATOR_FM_ATTENUVERTER,
  21,  // POT_RESONATOR_GEOMETRY,
  19,  // POT_RESONATOR_GEOMETRY_ATTENUVERTER,
  24,  // POT_RESONATOR_BRIGHTNESS,
  17,  // POT_RESONATOR_RIGHTNESS_ATTENUVERTER,
  4,  // POT_RESONATOR_DAMPING,
  10,  // POT_RESONATOR_DAMPING_ATTENUVERTER,
  0,  // POT_RESONATOR_POSITION,
  16,  // POT_RESONATOR_POSITION_ATTENUVERTER,
  27,  // POT_SPACE,
  18,  // POT_SPACE_ATTENUVERTER
};*/

void PotsAdc::Init() {
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB | RCC_AHB1Periph_GPIOE, ENABLE);
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC2, ENABLE);
  
  ADC_CommonInitTypeDef adc_common_init;
  ADC_InitTypeDef adc_init;
  GPIO_InitTypeDef gpio_init;
  
  // Configure the ADC pin.
  gpio_init.GPIO_Pin = GPIO_Pin_1;
  gpio_init.GPIO_PuPd = GPIO_PuPd_NOPULL;
  gpio_init.GPIO_Mode = GPIO_Mode_AN;
  GPIO_Init(GPIOB, &gpio_init);
  
  // Configure the mux chip selector and inhibit all chips.
  gpio_init.GPIO_Pin = GPIO_Pin_2;
  gpio_init.GPIO_Mode = GPIO_Mode_OUT;
  gpio_init.GPIO_OType = GPIO_OType_PP;
  gpio_init.GPIO_Speed = GPIO_Speed_2MHz;
  gpio_init.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_Init(GPIOE, &gpio_init);
  GPIO_SetBits(GPIOE, GPIO_Pin_2);

  // Configure the address lines.
  gpio_init.GPIO_Pin = GPIO_Pin_3 | GPIO_Pin_4 | GPIO_Pin_5;
  gpio_init.GPIO_Mode = GPIO_Mode_OUT;
  gpio_init.GPIO_OType = GPIO_OType_PP;
  gpio_init.GPIO_Speed = GPIO_Speed_2MHz;
  gpio_init.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_Init(GPIOE, &gpio_init);
  GPIO_ResetBits(GPIOE, GPIO_Pin_3 | GPIO_Pin_4 | GPIO_Pin_5);
  
  adc_common_init.ADC_Mode = ADC_Mode_Independent;
  adc_common_init.ADC_Prescaler = ADC_Prescaler_Div8;
  adc_common_init.ADC_DMAAccessMode = ADC_DMAAccessMode_Disabled;
  adc_common_init.ADC_TwoSamplingDelay = ADC_TwoSamplingDelay_5Cycles;
  ADC_CommonInit(&adc_common_init);
  
  adc_init.ADC_Resolution = ADC_Resolution_12b;
  adc_init.ADC_ScanConvMode = DISABLE;
  adc_init.ADC_ContinuousConvMode = DISABLE;
  adc_init.ADC_ExternalTrigConv = ADC_ExternalTrigConv_T1_CC1;
  adc_init.ADC_ExternalTrigConvEdge = ADC_ExternalTrigConvEdge_None;
  adc_init.ADC_DataAlign = ADC_DataAlign_Left;
  adc_init.ADC_NbrOfConversion = 1;
  ADC_Init(ADC2, &adc_init);
  
  ADC_RegularChannelConfig(ADC2, ADC_Channel_9, 1, ADC_SampleTime_480Cycles);
  
  ADC_Cmd(ADC2, ENABLE);
  
  index_ = ADC_CHANNEL_POTENTIOMETER_LAST - 1;
  last_read_ = 0;
  state_ = false;
  Scan();
}

void PotsAdc::DeInit() {
  ADC_DeInit();
}

void PotsAdc::Scan() {
  if (state_) {
    // Read the value from the previous conversion.
    values_[index_] = ADC2->DR;
    last_read_ = index_;
    ++index_;
    if (index_ >= ADC_CHANNEL_POTENTIOMETER_LAST) {
      index_ = 0;
    }
  
    uint8_t address = addresses_[index_];
  
    // Inhibit all muxes.
    //GPIO_SetBits(GPIOB, GPIO_Pin_1);
    GPIO_SetBits(GPIOE, GPIO_Pin_2);
  
    // Write the mux address.
    GPIO_WriteBit(GPIOE, GPIO_Pin_3, static_cast<BitAction>(address & 1));
    GPIO_WriteBit(GPIOE, GPIO_Pin_4, static_cast<BitAction>(address & 2));
    GPIO_WriteBit(GPIOE, GPIO_Pin_5, static_cast<BitAction>(address & 4));
  
    // Activate the right mux.
    //GPIO_ResetBits(GPIOB, GPIO_Pin_1);
    GPIO_ResetBits(GPIOE, GPIO_Pin_2);
  } else {
    ADC_SoftwareStartConv(ADC2);
  }
  state_ = !state_;
}

}  // namespace elements
