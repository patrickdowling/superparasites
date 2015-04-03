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
#include "clouds/dsp/random_oscillator.h"

using namespace stmlib;

namespace clouds {

class Reverb {
 public:
  Reverb() { }
  ~Reverb() { }


  void Init(uint16_t* buffer) {
    engine_.Init(buffer);
    lp_ = 0.7f;
    hp_ = 0.0f;
    diffusion_ = 0.625f;
    size_ = 1.0f;
    mod_amount_ = 0.0f;
    mod_rate_ = 0.0f;
    smoothed_size_ = size_;
    smoothed_mod_amount_ = mod_amount_;
    for (int i=0; i<12; i++)
        lfo_[i].Init(0);
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
    const float mod_amount_max = 600.0f;

    float lp_1 = lp_decay_1_;
    float lp_2 = lp_decay_2_;
    float hp_1 = hp_decay_1_;
    float hp_2 = hp_decay_2_;

    for (int i=0; i<12; i++) {
        lfo_[i].set_period(1 / mod_rate_ * 32000);
    }

#define INTERPOLATE_LFO(del, lfo, gain)                                     \
    {                                                                       \
        float lfo_val = lfo.Next() * mod_amount_max * smoothed_mod_amount_; \
        float offset = (del.length - 1) * smoothed_size_ + lfo_val;     \
        CONSTRAIN(offset, 0, del.length - 1);                           \
        c.Interpolate(del, offset, gain);                               \
    }

#define INTERPOLATE(del, gain)                                          \
    {                                                                   \
        c.Interpolate(del, del.length - 1, gain);                       \
    }

    while (size--) {
      float wet;
      float apout = 0.0f;

      // Smooth size and mod_amount to avoid delay glitches
      smoothed_size_ = smoothed_size_ + 0.0005f * (size_ - smoothed_size_);
      smoothed_mod_amount_ = smoothed_mod_amount_ + 0.0005f * (mod_amount_ - smoothed_mod_amount_);

      engine_.Start(&c);
      
      // Smear AP1 inside the loop.
      INTERPOLATE_LFO(ap1, lfo_[0], 1.0f);
      c.Write(ap1, 100 * smoothed_size_, 0.0f);
      
      c.Read(in_out->l + in_out->r, gain);

      // Diffuse through 4 allpasses.
      INTERPOLATE_LFO(ap1, lfo_[1], kap);
      c.WriteAllPass(ap1, -kap);
      INTERPOLATE_LFO(ap2, lfo_[2], kap);
      c.WriteAllPass(ap2, -kap);
      INTERPOLATE_LFO(ap3, lfo_[3], kap);
      c.WriteAllPass(ap3, -kap);
      INTERPOLATE_LFO(ap4, lfo_[4], kap);
      c.WriteAllPass(ap4, -kap);
      c.Write(apout);
      
      // Main reverb loop.
      c.Load(apout);
      INTERPOLATE_LFO(del2, lfo_[5], krt);
      c.Lp(lp_1, klp);
      c.Hp(hp_1, khp);
      INTERPOLATE_LFO(dap1a, lfo_[6], -kap);
      c.WriteAllPass(dap1a, kap);
      INTERPOLATE(dap1b, kap);
      c.WriteAllPass(dap1b, -kap);
      c.Write(del1, 2.0f);
      c.Write(wet, 0.0f);

      in_out->l += wet - in_out->l * amount;

      c.Load(apout);
      INTERPOLATE_LFO(del1, lfo_[8], krt);
      c.Lp(lp_2, klp);
      c.Hp(hp_2, khp);
      INTERPOLATE_LFO(dap2a, lfo_[9], kap);
      c.WriteAllPass(dap2a, -kap);
      INTERPOLATE(dap2b, -kap);
      c.WriteAllPass(dap2b, kap);
      c.Write(del2, 2.0f);
      c.Write(wet, 0.0f);

      in_out->r += wet - in_out->r * amount;

      /* if(in_out->r > 1.0f || in_out->r < 1.0f || in_out->l > 1.0f || in_out->l < 1.0f) */
      /*     printf("clip!\n"); */

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

  inline void set_mod_amount(float mod_amount) {
    mod_amount_ = mod_amount;
  }

  inline void set_mod_rate(float mod_rate) {
    mod_rate_ = mod_rate;
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
  float smoothed_size_;
  float mod_amount_;
  float smoothed_mod_amount_;
  float mod_rate_;

  float lp_decay_1_;
  float lp_decay_2_;
  float hp_decay_1_;
  float hp_decay_2_;

  RandomOscillator lfo_[12];

  DISALLOW_COPY_AND_ASSIGN(Reverb);
};

}  // namespace clouds

#endif  // CLOUDS_DSP_FX_REVERB_H_
