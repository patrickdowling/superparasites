// Copyright 2019 Patrick Dowling
//
// Author: Patrick Dowling (pld@gurkenkiste.com)
// Based on code existing in supercell/dsp/pvoc/...
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

#ifndef CLOUDS_DSP_PVOC_MODIFIER_H_
#define CLOUDS_DSP_PVOC_MODIFIER_H_

#include "stmlib/stmlib.h"

#include "supercell/dsp/pvoc/stft.h"

namespace clouds {

struct Parameters;

class Modifier {
public:
  Modifier() { }
  virtual ~Modifier() { }

  virtual uint32_t num_textures() const = 0;
  virtual uint32_t texture_size(size_t texture_size) const = 0;

  virtual void Init(float* buffer, int32_t fft_size, int32_t num_textures,
                    float sample_rate_hz, FFT* fft) = 0;

  virtual void Process(const Parameters& parameters, float* fft_out, float* ifft_in, bool trigger) = 0;
};

} // clouds

#endif // CLOUDS_DSP_PVOC_MODIFIER_H_
