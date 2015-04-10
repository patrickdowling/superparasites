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
// Single grain synthesis.

#ifndef CLOUDS_DSP_GRAIN_H_
#define CLOUDS_DSP_GRAIN_H_

#include "stmlib/stmlib.h"

#include "stmlib/dsp/dsp.h"

#include "clouds/dsp/audio_buffer.h"

#include "clouds/resources.h"

namespace clouds {

enum GrainQuality {
  GRAIN_QUALITY_LOW,
  GRAIN_QUALITY_MEDIUM,
  GRAIN_QUALITY_HIGH
};

class Grain {
 public:
  Grain() { }
  ~Grain() { }

  void Init() {
    active_ = false;
    envelope_phase_ = 2.0f;
  }

  void Start(
      int32_t pre_delay,
      int32_t buffer_size,
      int32_t start,
      int32_t width,
      int32_t phase_increment,
      float window_shape,
      float gain_l,
      float gain_r,
      GrainQuality recommended_quality) {
    pre_delay_ = pre_delay;
    width_ = width;
    first_sample_ = (start + buffer_size) % buffer_size;
    phase_increment_ = phase_increment;
    phase_ = 0;
    envelope_phase_ = 0.0f;
    envelope_phase_increment_ = 2.0f / static_cast<float>(width);

    if (window_shape >= 0.333f)
      envelope_slope_ = 1.0f;
    else
      envelope_slope_ = 0.333f / (window_shape + 0.0001f);

    if (window_shape < 0.333f)
      envelope_bias_ = - 2.0f * window_shape + 1.0f;
    else if (window_shape < 0.666f)
      envelope_bias_ = 6.0f * window_shape - 2.0f;
    else
      envelope_bias_ = 4.0f - 3.0f * window_shape;
    /* smooth out the response of bias: */
    envelope_bias_ = SoftCurve(envelope_bias_ * 2.0f / 1.971f - 0.015f);

    active_ = true;
    gain_l_ = gain_l;
    gain_r_ = gain_r;
    recommended_quality_ = recommended_quality;
  }
  
  inline void RenderEnvelope(float* destination, size_t size) {
    const float increment = envelope_phase_increment_;
    const float slope = envelope_slope_;
    const float bias = envelope_bias_;

    float phase = envelope_phase_;
    while (size--) {
      float gain = phase <= bias ?
        phase * slope / bias :
        (2.0f - phase) * slope / (2.0f - bias);
      if (gain > 1.0f) gain = 1.0f;
      phase += increment;
      if (phase >= 2.0f) {
        *destination = -1.0f;
        break;
      }
      *destination++ = gain;
    }
    envelope_phase_ = phase;
  }
  
  template<int32_t num_channels, GrainQuality quality, Resolution resolution>
  inline void OverlapAdd(
      const AudioBuffer<resolution>* buffer,
      float* destination,
      float* envelope,
      size_t size) {
    if (!active_) {
      return;
    }
    // Rendering is done on 32-sample long blocks. The pre-delay allows grains
    // to start at arbitrary samples within a block, rather than at block
    // boundaries.
    while (pre_delay_ && size) {
      destination += 2;
      --size;
      --pre_delay_;
    }
    
    // Pre-render the envelope in one pass.
    RenderEnvelope(envelope, size);

    const int32_t phase_increment = phase_increment_;
    const int32_t first_sample = first_sample_;
    const float gain_l = gain_l_;
    const float gain_r = gain_r_;
    int32_t phase = phase_;
    while (size--) {
      int32_t sample_index = first_sample + (phase >> 16);

      float gain = *envelope++;
      if (gain == -1.0f) {
        active_ = false;
        break;
      }

      float l = buffer[0].template Read<InterpolationMethod(quality)>(
          sample_index, phase & 65535) * gain;
      if (num_channels == 1) {
        *destination++ += l * gain_l;
        *destination++ += l * gain_r;
      } else if (num_channels == 2) {
        float r = buffer[1].template Read<InterpolationMethod(quality)>(
            sample_index, phase & 65535) * gain;
        *destination++ += l * gain_l + r * (1.0f - gain_r);
        *destination++ += r * gain_r + l * (1.0f - gain_l);
      }
      phase += phase_increment;
    }
    phase_ = phase;
  }
  
  inline bool active() { return active_; }
  
  inline GrainQuality recommended_quality() const {
    return recommended_quality_;
  }

  inline float SoftCurve (float x) {
  /* (2x + 1) ^ 3 / (9 (4 (x-2) x + 7)) */
    float a = 2 * x + 1;
    float b = 9 * (4 * (x - 2) * x + 7);
    return a * a * a / b;
  }

 private:
  int32_t first_sample_;
  int32_t width_;
  int32_t phase_;
  int32_t phase_increment_;
  int32_t pre_delay_;

  float envelope_slope_;
  float envelope_bias_;         /* asymetry of envelope: -1..1 */
  float envelope_phase_;
  float envelope_phase_increment_;

  float gain_l_;
  float gain_r_;

  bool active_;
  
  GrainQuality recommended_quality_;

  DISALLOW_COPY_AND_ASSIGN(Grain);
};

}  // namespace clouds

#endif  // CLOUDS_DSP_GRAIN_H_
