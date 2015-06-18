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
// Full Reverb (used in the dedicated "Oliverb" playback mode).

#ifndef CLOUDS_DSP_FX_FULL_REVERB_H_
#define CLOUDS_DSP_FX_FULL_REVERB_H_

#include "stmlib/stmlib.h"

#include "clouds/dsp/fx/fx_engine.h"
#include "clouds/dsp/random_oscillator.h"

namespace clouds {

class FullReverb {
 public:
  FullReverb() { }
  ~FullReverb() { }

  void Init(uint16_t* buffer) {
    engine_.Init(buffer);
    diffusion_ = 0.625f;
    size_ = 1.0f;
    mod_amount_ = 0.0f;
    mod_rate_ = 0.0f;
    size_ = 0.5f;
    input_gain_ = 1.0f;
    decay_ = 0.5f;
    lp_ = 1.0f;
    hp_= 0.0f;
    phase_ = 0.0f;
    ratio_ = 0.0f;
    pitch_shift_amount_ = 1.0f;
    level_ = 0.0f;
    for (int i=0; i<9; i++)
      lfo_[i].Init();
  }

  void Process(FloatFrame* in_out, size_t size) {
    // This is the Griesinger topology described in the Dattorro paper
    // (4 AP diffusers on the input, then a loop of 2x 2AP+1Delay).
    // Modulation is applied in the loop of the first diffuser AP for additional
    // smearing; and to the two long delays for a slow shimmer/chorus effect.
    typedef
      E::Reserve<113,
      E::Reserve<162,
      E::Reserve<241,
      E::Reserve<399,
      E::Reserve<79,
      E::Reserve<107,
      E::Reserve<151,
      E::Reserve<257,
      E::Reserve<1653,
      E::Reserve<2038,
      E::Reserve<3411,
      E::Reserve<1913,
      E::Reserve<1663,
      E::Reserve<4177> > > > > > > > > > > > > > Memory;
    E::DelayLine<Memory, 0> ap1l;
    E::DelayLine<Memory, 1> ap2l;
    E::DelayLine<Memory, 2> ap3l;
    E::DelayLine<Memory, 3> ap4l;
    E::DelayLine<Memory, 4> ap1r;
    E::DelayLine<Memory, 5> ap2r;
    E::DelayLine<Memory, 6> ap3r;
    E::DelayLine<Memory, 7> ap4r;
    E::DelayLine<Memory, 8> dap1a;
    E::DelayLine<Memory, 9> dap1b;
    E::DelayLine<Memory, 10> del1;
    E::DelayLine<Memory, 11> dap2a;
    E::DelayLine<Memory, 12> dap2b;
    E::DelayLine<Memory, 13> del2;
    E::Context c;

    const float kap = diffusion_;

    float lp_1 = lp_decay_1_;
    float lp_2 = lp_decay_2_;
    float hp_1 = hp_decay_1_;
    float hp_2 = hp_decay_2_;

    /* Set frequency of LFOs */
    float period = 1.0f / (mod_rate_ + 0.001f) * 32000.0f;
    for (int i=0; i<9; i++)
      lfo_[i].set_period((uint32_t)period);

    while (size--) {
      float apout1 = 0.0f;
      float apout2 = 0.0f;
      engine_.Start(&c);

      // Smooth parameters to avoid delay glitches
      ONE_POLE(smooth_size_, size_, 0.01f);

      float tri;
      float phase;
      float half;

      // compute windowing info for the pitch shifter
      float ps_size = 128.0f + (3410.0f - 128.0f) * smooth_size_;
      phase_ += (1.0f - ratio_) / ps_size;
      if (phase_ >= 1.0f) phase_ -= 1.0f;
      if (phase_ <= 0.0f) phase_ += 1.0f;
      tri = 2.0f * (phase_ >= 0.5f ? 1.0f - phase_ : phase_);
      phase = phase_ * ps_size;
      half = phase + ps_size * 0.5f;
      if (half >= ps_size) half -= ps_size;

#define INTERPOLATE_LFO(del, lfo, gain)                                 \
      {                                                                 \
        float offset = (del.length - 1) * smooth_size_;                 \
        offset += lfo.Next() * mod_amount_;                      \
        CONSTRAIN(offset, 1.0f, del.length - 1);                        \
        c.InterpolateHermite(del, offset, gain);                        \
      }

#define INTERPOLATE(del, gain)                                          \
      {                                                                 \
        float offset = (del.length - 1) * smooth_size_;                 \
        CONSTRAIN(offset, 1.0f, del.length - 1);                        \
        c.InterpolateHermite(del, offset, gain);                        \
      }

      c.Read(in_out->l, input_gain_);
      // Diffuse through 4 allpasses.
      INTERPOLATE_LFO(ap1l, lfo_[1], kap);
      c.WriteAllPass(ap1l, -kap);
      INTERPOLATE_LFO(ap2l, lfo_[2], kap);
      c.WriteAllPass(ap2l, -kap);
      INTERPOLATE_LFO(ap3l, lfo_[3], kap);
      c.WriteAllPass(ap3l, -kap);
      INTERPOLATE_LFO(ap4l, lfo_[4], kap);
      c.WriteAllPass(ap4l, -kap);
      c.Write(apout1, 0.0f);

      c.Read(in_out->r, input_gain_);
      // Diffuse through 4 allpasses.
      INTERPOLATE_LFO(ap1r, lfo_[1], kap);
      c.WriteAllPass(ap1r, -kap);
      INTERPOLATE_LFO(ap2r, lfo_[2], kap);
      c.WriteAllPass(ap2r, -kap);
      INTERPOLATE_LFO(ap3r, lfo_[3], kap);
      c.WriteAllPass(ap3r, -kap);
      INTERPOLATE_LFO(ap4r, lfo_[4], kap);
      c.WriteAllPass(ap4r, -kap);
      c.Write(apout2, 0.0f);

      // Main reverb loop.
      c.Load(apout1);

      INTERPOLATE_LFO(del2, lfo_[5], decay_ * (1.0f - pitch_shift_amount_));
      /* blend in the pitch shifted feedback */
      c.InterpolateHermite(del2, phase, tri * decay_ * pitch_shift_amount_);
      c.InterpolateHermite(del2, half, (1.0f - tri) * decay_ * pitch_shift_amount_);

      c.Lp(lp_1, lp_);
      c.Hp(hp_1, hp_);
      c.SoftLimit();
      INTERPOLATE_LFO(dap1a, lfo_[6], -kap);
      c.WriteAllPass(dap1a, kap);
      INTERPOLATE(dap1b, kap);
      c.WriteAllPass(dap1b, -kap);
      c.Write(del1, 1.0f);
      c.Write(in_out->l, 0.0f);

      c.Load(apout2);

      INTERPOLATE_LFO(del1, lfo_[7], decay_ * (1.0f - pitch_shift_amount_));
      /* blend in the pitch shifted feedback */
      c.InterpolateHermite(del1, phase, tri * decay_ * pitch_shift_amount_);
      c.InterpolateHermite(del1, half, (1.0f - tri) * decay_ * pitch_shift_amount_);
      c.Lp(lp_2, lp_);
      c.Hp(hp_2, hp_);
      c.SoftLimit();
      INTERPOLATE_LFO(dap2a, lfo_[8], kap);
      c.WriteAllPass(dap2a, -kap);
      INTERPOLATE(dap2b, -kap);
      c.WriteAllPass(dap2b, kap);
      c.Write(del2, 1.0f);
      c.Write(in_out->r, 0.0f);

      ++in_out;
    }

    lp_decay_1_ = lp_1;
    lp_decay_2_ = lp_2;
    hp_decay_1_ = hp_1;
    hp_decay_2_ = hp_2;
  }

  inline void set_input_gain(float input_gain) {
    input_gain_ = input_gain;
  }

  inline void set_decay(float decay) {
    decay_ = decay;
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

  float input_gain_;
  float decay_;
  float diffusion_;
  float lp_;
  float hp_;
  float size_, smooth_size_;
  float mod_amount_;
  float mod_rate_;
  float pitch_shift_amount_;

  float lp_decay_1_;
  float lp_decay_2_;
  float hp_decay_1_;
  float hp_decay_2_;

  float phase_;
  float ratio_;
  float level_;

  RandomOscillator lfo_[9];

  DISALLOW_COPY_AND_ASSIGN(FullReverb);
};

}  // namespace clouds

#endif  // CLOUDS_DSP_FX_FULL_REVERB_H_
