#include "supercell/drivers/dac.h"
#include <stm32f4xx_conf.h>
#include <math.h>

namespace clouds {

const float nse_rate_logmax = logf(12.0f);
const float nse_rate_logmin = logf(1170.0f);

void Dac::Init() 
{
    // Enable APB/AHB clocks
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_DAC, ENABLE);
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM6, ENABLE);
    // Configure DAC pin PA4 as AF
    GPIO_InitTypeDef GPIO_InitStruct;
    GPIO_InitStruct.GPIO_Pin = GPIO_Pin_4;
    //GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AN;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_OUT;
    GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_Init(GPIOA, &GPIO_InitStruct);
    // Configure TIM6 as trigger for DAC.
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStruct;
    TIM_TimeBaseStructInit(&TIM_TimeBaseStruct);
    TIM_TimeBaseStruct.TIM_Period=0xffff;
    TIM_TimeBaseStruct.TIM_Prescaler=64;
    TIM_TimeBaseStruct.TIM_ClockDivision=2;
    TIM_TimeBaseStruct.TIM_CounterMode=TIM_CounterMode_Up;
    TIM_TimeBaseInit(TIM6, &TIM_TimeBaseStruct);
    // Enable TRGO and TIM6
    TIM_SelectOutputTrigger(TIM6, TIM_TRGOSource_Update);
    TIM_Cmd(TIM6, ENABLE);
    // Turn on DAC Timer, Enable DAC.
    StartNoise();
}


void Dac::DeInit()
{
    StopNoise();
}

void Dac::StartNoise()
{
    GPIO_InitTypeDef GPIO_InitStruct;
    GPIO_InitStruct.GPIO_Pin = GPIO_Pin_4;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AN;
    GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_Init(GPIOA, &GPIO_InitStruct);
    // Configure DAC Channel
    DAC_InitTypeDef DAC_InitStruct;
    // Enable DAC -- testing with noise first.
    DAC_InitStruct.DAC_Trigger = DAC_Trigger_T6_TRGO;
    DAC_InitStruct.DAC_WaveGeneration = DAC_WaveGeneration_Noise;
    //DAC_InitStruct.DAC_LFSRUnmask_Triangle_Amplitude = DAC_LFSRUnmask_Bits10_0;
    DAC_InitStruct.DAC_LFSRUnmask_TriangleAmplitude = DAC_LFSRUnmask_Bits11_0;
    DAC_InitStruct.DAC_OutputBuffer = DAC_OutputBuffer_Enable;
    DAC_Init(DAC_Channel_1, &DAC_InitStruct);
    DAC_Cmd(DAC_Channel_1, ENABLE);
    DAC_SetChannel1Data(DAC_Align_12b_L, 0x7FF0);
    generating_noise_ = true;
}

void Dac::StopNoise()
{
    DAC_Cmd(DAC_Channel_1, DISABLE);
    generating_noise_ = false;
    GPIO_InitTypeDef GPIO_InitStruct;
    GPIO_InitStruct.GPIO_Pin = GPIO_Pin_4;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_OUT;
    GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_Init(GPIOA, &GPIO_InitStruct);
    GPIO_WriteBit(GPIOA, GPIO_Pin_4, static_cast<BitAction>(0));
}

void Dac::SetNoiseFreq(float period)
{
    //uint32_t ps = static_cast<uint32_t>((1.0f - period) * 192.0f) + 16;
    //uint32_t ps = static_cast<uint32_t>((1.0f - period) * 1170.0f) + 12; // linear scale 1000ms to 10ms
    float temp = expf(period * (nse_rate_logmax - nse_rate_logmin) + nse_rate_logmin);
    uint32_t ps = static_cast<uint32_t>(temp);
    //StopNoise();
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStruct;
    TIM_TimeBaseStructInit(&TIM_TimeBaseStruct);
    TIM_TimeBaseStruct.TIM_Period=0xffff;
    TIM_TimeBaseStruct.TIM_Prescaler=ps;
    TIM_TimeBaseStruct.TIM_ClockDivision=2;
    TIM_TimeBaseStruct.TIM_CounterMode=TIM_CounterMode_Up;
    TIM_TimeBaseInit(TIM6, &TIM_TimeBaseStruct);
    //StartNoise();
}

}
