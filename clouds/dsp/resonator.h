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
// Resonator.

#ifndef CLOUDS_DSP_RESONATOR_H_
#define CLOUDS_DSP_RESONATOR_H_

#include "stmlib/stmlib.h"
#include "stmlib/utils/random.h"
#include "stmlib/dsp/units.h"
#include "clouds/dsp/fx/fx_engine.h"
#include "clouds/resources.h"

using namespace stmlib;

namespace clouds {

const float chords[3][18] =
  {
    { 0.0f, 4.0f/128.0f, 16.0f/128.0f, 4.0f/128.0f, 4.0f/128.0f,
      12.0f, 12.0f, 4.0f, 4.0f,
      3.0f, 3.0f, 2.0f, 4.0f,
      3.0f, 4.0f, 3.0f, 4.0f,
      4.0f },
    { 0.0f, 8.0f/128.0f, 32.0f/128.0f, 7.0f, 12.0f,
      24.0f, 7.0f, 7.0f, 7.0f,
      7.0f, 7.0f, 7.0f, 7.0f,
      7.0f, 7.0f, 7.0f, 7.0f,
      7.0f },
    { 0.0f, 12.0f/128.0f, 48.0f/128.0f, 7.0f + 4.0f/128.0f, 12.0f + 4.0f/128.0f,
      36.0f, 19.0f, 12.0f, 11.0f,
      10.0f, 12.0f, 12.0f, 12.0f,
      14.0f, 14.0f, 16.0f, 16.0f,
      16.0f }
  };

inline float InterpolateSine(const float* table, float index, float size) {
  index *= size;
  MAKE_INTEGRAL_FRACTIONAL(index)
  float a = table[index_integral];
  float b = table[index_integral + 1];
  float c = Interpolate(lut_sin, index_fractional, LUT_SIN_SIZE);
  return a + (b - a) * c;
}

class Resonator {
 public:
  Resonator() { }
  ~Resonator() { }

  void Init(uint16_t* buffer) {
    engine_.Init(buffer);
    feedback_ = 0.0f;
    damp_ = 0.6f;
    pitch_ = 0.0f;
    chord_ = 0.0f;
    spread_amount_ = 0.0f;
    stereo_ = 0.0f;
    burst_time_ = 0.0f;
    burst_damp_ = 1.0f;
    burst_comb_ = 1.0f;
    voice_ = 0.0f;
  }

  void Process(FloatFrame* in_out, size_t size) {

    typedef E::Reserve<1500,
      E::Reserve<1500,
      E::Reserve<1500,
      E::Reserve<1500,
      E::Reserve<200,          /* bc */
      E::Reserve<4000,          /* bd */
      E::Reserve<1500,
      E::Reserve<1500,
      E::Reserve<1500,
      E::Reserve<1500 > > > > > > > > > > Memory;
    E::DelayLine<Memory, 0> c1l;
    E::DelayLine<Memory, 1> c2l;
    E::DelayLine<Memory, 2> c3l;
    E::DelayLine<Memory, 3> c4l;
    E::DelayLine<Memory, 4> bc;
    E::DelayLine<Memory, 5> bd;
    E::DelayLine<Memory, 6> c1r;
    E::DelayLine<Memory, 7> c2r;
    E::DelayLine<Memory, 8> c3r;
    E::DelayLine<Memory, 9> c4r;
    E::Context c;

    if (trigger_ && !previous_trigger_) {
      voice_ = 1.0f - voice_;
    }

    if (voice_ < 0.5f)
      pitch_l_ = pitch_;
    else
      pitch_r_ = pitch_;

    float c1l_delay = 32000.0f / 110.0f / SemitonesToRatio(pitch_l_);
    CONSTRAIN(c1l_delay, 0, c1l.length);
    float c2l_pitch = InterpolateSine(chords[0], chord_, 17);
    float c2l_ratio = SemitonesToRatio(c2l_pitch);
    float c2l_delay = c1l_delay / c2l_ratio;
    CONSTRAIN(c2l_delay, 0, c2l.length);
    float c3l_pitch = InterpolateSine(chords[1], chord_, 17);
    float c3l_ratio = SemitonesToRatio(c3l_pitch);
    float c3l_delay = c1l_delay / c3l_ratio;
    CONSTRAIN(c3l_delay, 0, c3l.length);
    float c4l_pitch = InterpolateSine(chords[2], chord_, 17);
    float c4l_ratio = SemitonesToRatio(c4l_pitch);
    float c4l_delay = c1l_delay / c4l_ratio;
    CONSTRAIN(c4l_delay, 0, c4l.length);

    float c1r_delay = 32000.0f / 110.0f / SemitonesToRatio(pitch_r_);
    CONSTRAIN(c1r_delay, 0, c1r.length);
    float c2r_pitch = InterpolateSine(chords[0], chord_, 17);
    float c2r_ratio = SemitonesToRatio(c2r_pitch);
    float c2r_delay = c1r_delay / c2r_ratio;
    CONSTRAIN(c2r_delay, 0, c2r.length);
    float c3r_pitch = InterpolateSine(chords[1], chord_, 17);
    float c3r_ratio = SemitonesToRatio(c3r_pitch);
    float c3r_delay = c1r_delay / c3r_ratio;
    CONSTRAIN(c3r_delay, 0, c3r.length);
    float c4r_pitch = InterpolateSine(chords[2], chord_, 17);
    float c4r_ratio = SemitonesToRatio(c4r_pitch);
    float c4r_delay = c1r_delay / c4r_ratio;
    CONSTRAIN(c4r_delay, 0, c4r.length);

    if (trigger_ && !previous_trigger_) {
      previous_trigger_ = trigger_;
      burst_time_ = c1l_delay > c2l_delay && c1l_delay > c3l_delay ? c1l_delay :
        c2l_delay > c3l_delay ? c2l_delay : c3l_delay;
      burst_time_ *= 2.0f * burst_duration_;

      spread_delay_1_ = Random::GetFloat() * (bd.length - 1);
      spread_delay_2_ = Random::GetFloat() * (bd.length - 1);
      spread_delay_3_ = Random::GetFloat() * (bd.length - 1);
    }

    while (size--) {
      engine_.Start(&c);

      burst_time_--;
      float burst_gain = burst_time_ > 0.0f ? 1.0f : 0.0f;

      // burst noise generation
      c.Read((Random::GetFloat() * 2.0f - 1.0f), burst_gain);
      // goes through comb and lp filters
      float comb_fb = 0.6f - burst_comb_ * 0.4f;
      float comb_del = burst_comb_ * bc.length;
      if (comb_del <= 1.0f) comb_del = 1.0f;
      c.InterpolateHermite(bc, comb_del, comb_fb);
      c.Write(bc, 1.0f);
      c.Lp(burst_lp1, burst_damp_);
      c.Lp(burst_lp2, burst_damp_);

      c.Read(in_out->l + in_out->r, 1.0f);
      c.Write(bd, 0.0f);

#define COMB(pre, del, time, lp, vol)                    \
      c.Read(bd, pre * spread_amount_, vol);   \
      c.InterpolateHermite(del, time, feedback_);        \
      c.Lp(lp, damp_);                                   \
      c.Write(del, 0.0f);

      /* first voice: */
      COMB(0, c1l, c1l_delay, lp1l, 1.0f - voice_);
      COMB(spread_delay_1_, c2l, c2l_delay, lp2l, 1.0f - voice_);
      COMB(spread_delay_2_, c3l, c3l_delay, lp3l, 1.0f - voice_);
      COMB(spread_delay_3_, c4l, c4l_delay, lp4l, 1.0f - voice_);

      /* second voice: */
      COMB(0, c1r, c1r_delay, lp1r, voice_);
      COMB(spread_delay_1_, c2r, c2r_delay, lp2r, voice_);
      COMB(spread_delay_2_, c3r, c3r_delay, lp3r, voice_);
      COMB(spread_delay_3_, c4r, c4r_delay, lp4r, voice_);

      c.Read(c1l, 0.20f * (1.0f - stereo_) * (1.0f - separation_));
      c.Read(c2l, (0.23f + 0.23f * stereo_) * (1.0f - separation_));
      c.Read(c3l, (0.27f * (1.0f - stereo_)) * (1.0f - separation_));
      c.Read(c4l, (0.30f + 0.30f * stereo_) * (1.0f - separation_));
      c.Read(c1r, 0.20f + 0.20f * stereo_);
      c.Read(c2r, 0.23f * (1.0f - stereo_));
      c.Read(c3r, 0.27f + 0.27 * stereo_);
      c.Read(c4r, 0.30f * (1.0f - stereo_));
      c.Write(in_out->l, 0.0f);

      c.Read(c1l, 0.20f + 0.20f * stereo_);
      c.Read(c2l, 0.23f * (1.0f - stereo_));
      c.Read(c3l, 0.27f + 0.27 * stereo_);
      c.Read(c4l, 0.30f * (1.0f - stereo_));
      c.Read(c1r, 0.20f * (1.0f - stereo_) * (1.0f - separation_));
      c.Read(c2r, (0.23f + 0.23f * stereo_) * (1.0f - separation_));
      c.Read(c3r, 0.27f * (1.0f - stereo_) * (1.0f - separation_));
      c.Read(c4r, (0.30f + 0.30f * stereo_) * (1.0f - separation_));
      c.Write(in_out->r, 0.0f);

      ++in_out;
    }
  }

  void set_pitch(float pitch) {
    pitch_ = pitch;
  }

  void set_trigger(bool trigger) {
    previous_trigger_ = trigger_;
    trigger_ = trigger;
  }

  void set_feedback(float feedback) {
    feedback_ = feedback;
  }

  void set_damp(float damp) {
    damp_ = damp;
  }

  void set_burst_damp(float burst_damp) {
    burst_damp_ = burst_damp;
  }

  void set_burst_comb(float burst_comb) {
    burst_comb_ = burst_comb;
  }

  void set_burst_duration(float burst_duration) {
    burst_duration_ = burst_duration;
  }

  void set_chord(float chord) {
    chord_ = chord;
  }

  void set_spread_amount(float spread_amount) {
    spread_amount_ = spread_amount;
  }

  void set_stereo(float stereo) {
    stereo_ = stereo;
  }

  void set_separation(float separation) {
    separation_ = separation;
  }

 private:
  typedef FxEngine<16384, FORMAT_12_BIT> E;
  E engine_;

  float feedback_;
  float damp_;
  float pitch_;
  float chord_;
  float spread_amount_;
  float stereo_;
  float separation_;
  bool trigger_, previous_trigger_;

  float voice_;

  float burst_time_;
  float burst_damp_;
  float burst_comb_;
  float burst_duration_;

  float pitch_l_, pitch_r_;

  float spread_delay_1_, spread_delay_2_, spread_delay_3_;

  float burst_lp1, burst_lp2;
  float lp1l, lp2l, lp3l, lp4l;
  float lp1r, lp2r, lp3r, lp4r;

  DISALLOW_COPY_AND_ASSIGN(Resonator);
};

}  // namespace clouds

#endif  // CLOUDS_DSP_RESONATOR_H_
