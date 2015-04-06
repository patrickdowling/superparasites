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
    smooth_mod_amount_ = mod_amount_ = 0.0f;
    smooth_mod_rate_ = mod_rate_ = 0.0f;
    smooth_size_ = size_ = 0.5f;
    smooth_input_gain_ = input_gain_ = 1.0f;
    smooth_time_ = reverb_time_ = 0.5f;
    smooth_lp_ = lp_ = 1.0f;
    smooth_hp_ = hp_= 0.0f;
    phase_ = 0.0f;
    ratio_ = 0.0f;
    pitch_shift_amount_ = smooth_pitch_shift_amount_ = 1.0f;
    envelope_1_ = envelope_2_ = 0.0f;
    level_ = 0.0f;
    for (int i=0; i<9; i++)
      lfo_[i].Init();
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
    const float amount = amount_;

    float lp_1 = lp_decay_1_;
    float lp_2 = lp_decay_2_;
    float hp_1 = hp_decay_1_;
    float hp_2 = hp_decay_2_;

    /* Set frequency of LFOs */
    float period = 1.0f / (fabs(smooth_mod_rate_) + 0.001f) * 32000.0f;
    for (int i=0; i<9; i++)
      lfo_[i].set_period((uint32_t)period);

    while (size--) {
      float wet;
      float apout = 0.0f;

      engine_.Start(&c);

      // Smooth parameters to avoid delay glitches
      ONE_POLE(smooth_size_, size_, 0.005f);
      ONE_POLE(smooth_mod_amount_, mod_amount_, 0.005f);
      ONE_POLE(smooth_mod_rate_, mod_rate_, 0.005f);
      ONE_POLE(smooth_input_gain_, input_gain_, 0.05f);
      ONE_POLE(smooth_time_, reverb_time_, 0.005f);
      ONE_POLE(smooth_lp_, lp_, 0.05f);
      ONE_POLE(smooth_hp_, hp_, 0.05f);
        /* disables pitch shifter when pitch is unchanged, to avoid
         * weird chorus effects */
      const float pitch_shift_amount =
        ratio_ <= 1.0001 && ratio_ >= 0.999 ? 0.0 : pitch_shift_amount_;
      ONE_POLE(smooth_pitch_shift_amount_, pitch_shift_amount, 0.05f);

      float ps_size = 128.0f + (3410.0f - 128.0f) *
        smooth_size_ * smooth_size_ * smooth_size_;
      phase_ += (1.0f - ratio_) / ps_size;
      if (phase_ >= 1.0f) {
        phase_ -= 1.0f;
      }
      if (phase_ <= 0.0f) {
        phase_ += 1.0f;
      }
      float tri = 2.0f * (phase_ >= 0.5f ? 1.0f - phase_ : phase_);
      float phase = phase_ * ps_size;
      float half = phase + ps_size * 0.5f;
      if (half >= ps_size) {
        half -= ps_size;
      }

#define INTERPOLATE_LFO(del, lfo, gain)                                 \
    {                                                                   \
            float lfo_val = smooth_mod_amount_ <= 0.001 ? 0 :           \
                lfo.Next() * smooth_mod_amount_;                        \
            float offset = (del.length - 1) * smooth_size_ + lfo_val;   \
            CONSTRAIN(offset, 0, del.length - 1);                       \
            c.Interpolate(del, offset, gain);                           \
    }

#define INTERPOLATE(del, gain)                                          \
    {                                                                   \
        c.Interpolate(del, (del.length - 1) * smooth_size_, gain);                       \
    }

      // Smear AP1 inside the loop.
      INTERPOLATE_LFO(ap1, lfo_[0], 1.0f);
      c.Write(ap1, 100 * smooth_size_, 0.0f);
      
      c.Read(in_out->l + in_out->r, smooth_input_gain_);

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
      INTERPOLATE_LFO(del2, lfo_[5], smooth_time_);
      c.Lp(lp_1, smooth_lp_);
      c.Hp(hp_1, smooth_hp_);
      c.SoftLimit();
      INTERPOLATE_LFO(dap1a, lfo_[6], -kap);
      c.WriteAllPass(dap1a, kap);
      INTERPOLATE(dap1b, kap);
      c.WriteAllPass(dap1b, -kap);
      c.Write(del1, 1.0f);
      c.Write(wet, 0.0f);

      in_out->l += (wet - in_out->l) * amount;

      c.Load(apout);
      INTERPOLATE_LFO(del1, lfo_[7], smooth_time_ * (1.0f - smooth_pitch_shift_amount_));
      /* blend in the pitch shifted feedback */
      c.Interpolate(del1, phase, tri * smooth_time_ * smooth_pitch_shift_amount_);
      c.Interpolate(del1, half, (1.0f - tri) * smooth_time_ * smooth_pitch_shift_amount_);
      c.Lp(lp_2, smooth_lp_);
      c.Hp(hp_2, smooth_hp_);
      c.SoftLimit();
      INTERPOLATE_LFO(dap2a, lfo_[8], kap);
      c.WriteAllPass(dap2a, -kap);
      INTERPOLATE(dap2b, -kap);
      c.WriteAllPass(dap2b, kap);
      c.Write(del2, 1.0f);
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

  inline void set_mod_amount(float mod_amount) {
    mod_amount_ = mod_amount;
  }

  inline void set_mod_rate(float mod_rate) {
    mod_rate_ = mod_rate;
  }

  inline void set_ratio(float ratio) {
    ratio_ = ratio;
  }

  inline void set_pitch_shift_amount(float pitch_shift) {
    pitch_shift_amount_ = pitch_shift;
  }

 private:
  typedef FxEngine<16384, FORMAT_12_BIT> E;
  E engine_;
  
  float amount_;
  float input_gain_;
  float smooth_input_gain_;
  float reverb_time_;
  float smooth_time_;
  float diffusion_;
  float lp_;
  float hp_;
  float smooth_lp_;
  float smooth_hp_;
  float size_;
  float smooth_size_;
  float mod_amount_;
  float smooth_mod_amount_;
  float mod_rate_;
  float smooth_mod_rate_;
  float pitch_shift_amount_;
  float smooth_pitch_shift_amount_;

  float lp_decay_1_;
  float lp_decay_2_;
  float hp_decay_1_;
  float hp_decay_2_;

  float phase_;
  float ratio_;
  float envelope_1_;
  float envelope_2_;
  float level_;

  RandomOscillator lfo_[9];

  DISALLOW_COPY_AND_ASSIGN(Reverb);
};

}  // namespace clouds

#endif  // CLOUDS_DSP_FX_REVERB_H_
