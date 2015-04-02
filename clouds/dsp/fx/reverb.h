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
// Reverb.

#ifndef CLOUDS_DSP_FX_REVERB_H_
#define CLOUDS_DSP_FX_REVERB_H_

#include "stmlib/stmlib.h"

#include "clouds/dsp/fx/fx_engine.h"

namespace clouds {

class Reverb {
 public:
  Reverb() { }
  ~Reverb() { }
  
  void Init(uint16_t* buffer) {
    engine_.Init(buffer);
    engine_.SetLFOFrequency(LFO_1, 0.5f / 32000.0f);
    engine_.SetLFOFrequency(LFO_2, 0.3f / 32000.0f);
    lp_ = 0.7f;
    hp_ = 0.0f;
    diffusion_ = 0.625f;
    size_ = 1.0f;
    modulation_ = 0.0f;
    smoothed_size_ = size_;
  }
  
  void Process(FloatFrame* in_out, size_t size) {
    // This is the Griesinger topology described in the Dattorro paper
    // (4 AP diffusers on the input, then a loop of 2x 2AP+1Delay).
    // Modulation is applied in the loop of the first diffuser AP for additional
    // smearing; and to the two long delays for a slow shimmer/chorus effect.
    typedef E::Reserve<113,
      E::Reserve<162,
      E::Reserve<241,
      E::Reserve<399,
      E::Reserve<1653,
      E::Reserve<2038,
      E::Reserve<3411,
      E::Reserve<1913,
      E::Reserve<1663,
      E::Reserve<4782> > > > > > > > > > Memory;
    E::DelayLine<Memory, 0> ap1;
    E::DelayLine<Memory, 1> ap2;
    E::DelayLine<Memory, 2> ap3;
    E::DelayLine<Memory, 3> ap4;
    E::DelayLine<Memory, 4> dap1a;
    E::DelayLine<Memory, 5> dap1b;
    E::DelayLine<Memory, 6> del1;
    E::DelayLine<Memory, 7> dap2a;
    E::DelayLine<Memory, 8> dap2b;
    E::DelayLine<Memory, 9> del2;
    E::Context c;

    const float kap = diffusion_;
    const float klp = lp_;
    const float khp = hp_;
    const float krt = reverb_time_;
    const float amount = amount_;
    const float gain = input_gain_;
    const float modulation = modulation_;

    float lp_1 = lp_decay_1_;
    float lp_2 = lp_decay_2_;
    float hp_1 = hp_decay_1_;
    float hp_2 = hp_decay_2_;

    while (size--) {
      float wet;
      float apout = 0.0f;

      // Smooth the size to avoid glitch
      smoothed_size_ = smoothed_size_ + 0.0005f * (size_ - smoothed_size_);

      engine_.Start(&c);
      
      // Smear AP1 inside the loop.
      c.Interpolate(ap1, 10.0f * smoothed_size_, LFO_1, 60.0f * modulation, 1.0f);
      c.Write(ap1, 100 * smoothed_size_, 0.0f);
      
      c.Read(in_out->l + in_out->r, gain);

      // Diffuse through 4 allpasses.
      c.InterpolateFrom(ap1, smoothed_size_, kap);
      c.WriteAllPass(ap1, -kap);
      c.InterpolateFrom(ap2, smoothed_size_, kap);
      c.WriteAllPass(ap2, -kap);
      c.InterpolateFrom(ap3, smoothed_size_, kap);
      c.WriteAllPass(ap3, -kap);
      c.InterpolateFrom(ap4, smoothed_size_, kap);
      c.WriteAllPass(ap4, -kap);
      c.Write(apout);
      
      // Main reverb loop.
      c.Load(apout);
      c.Interpolate(del2, 4683.0f * smoothed_size_, LFO_2, 100.0f * modulation, krt);
      c.Lp(lp_1, klp);
      c.Hp(hp_1, khp);
      c.InterpolateFrom(dap1a, smoothed_size_, -kap);
      c.WriteAllPass(dap1a, kap);
      c.InterpolateFrom(dap1b, smoothed_size_, kap);
      c.WriteAllPass(dap1b, -kap);
      c.Write(del1, 2.0f);
      c.Write(wet, 0.0f);

      in_out->l += (wet - in_out->l) * amount;

      c.Load(apout);
      c.InterpolateFrom(del1, smoothed_size_, krt);
      c.Lp(lp_2, klp);
      c.Hp(hp_2, khp);
      c.InterpolateFrom(dap2a, smoothed_size_, kap);
      c.WriteAllPass(dap2a, -kap);
      c.InterpolateFrom(dap2b, smoothed_size_, -kap);
      c.WriteAllPass(dap2b, kap);
      c.Write(del2, 2.0f);
      c.Write(wet, 0.0f);

      in_out->r += (wet - in_out->r) * amount;
      
      ++in_out;
    }
    
    lp_decay_1_ = lp_1;
    lp_decay_2_ = lp_2;
    hp_decay_1_ = hp_1;
    hp_decay_2_ = hp_2;
  }
  
  inline void set_amount(float amount) {
    amount_ = amount;
  }
  
  inline void set_input_gain(float input_gain) {
    input_gain_ = input_gain;
  }

  inline void set_time(float reverb_time) {
    reverb_time_ = reverb_time;
  }
  
  inline void set_diffusion(float diffusion) {
    diffusion_ = diffusion;
  }
  
  inline void set_lp(float lp) {
    lp_ = lp;
  }

  inline void set_hp(float hp) {
    hp_ = hp;
  }

  inline void set_size(float size) {
    size_ = size;
  }

  inline void set_modulation(float modulation) {
    modulation_ = modulation;
  }
  
 private:
  typedef FxEngine<16384, FORMAT_12_BIT> E;
  E engine_;
  
  float amount_;
  float input_gain_;
  float reverb_time_;
  float diffusion_;
  float lp_;
  float hp_;
  float size_;

  float lp_decay_1_;
  float lp_decay_2_;
  float hp_decay_1_;
  float hp_decay_2_;
  float smoothed_size_;
  float modulation_;

  DISALLOW_COPY_AND_ASSIGN(Reverb);
};

}  // namespace clouds

#endif  // CLOUDS_DSP_FX_REVERB_H_
