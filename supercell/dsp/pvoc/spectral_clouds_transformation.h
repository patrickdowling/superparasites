// Copyright 2014 Julius Kammerl.
//
// Author: Julius Kammerl (julius.kammerl@gmail.com)
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
// Generates clouds like frequency spectrums.

#ifndef CLOUDS_DSP_PVOC_SPECTRAL_CLOUDS_TRANSFORMATION_H_
#define CLOUDS_DSP_PVOC_SPECTRAL_CLOUDS_TRANSFORMATION_H_

#include "stmlib/stmlib.h"

#include "supercell/dsp/pvoc/stft.h"
#include "supercell/dsp/pvoc/modifier.h"
#include "supercell/resources.h"

namespace clouds {

struct Parameters;

class SpectralCloudsTransformation : public Modifier {
public:
	SpectralCloudsTransformation() {
	}
	~SpectralCloudsTransformation() {
	}

	static const int32_t kMaxNumTextures = 7;

	virtual uint32_t num_textures() const {
		return kMaxNumTextures;
	}
	virtual uint32_t texture_size(size_t fft_size) const {
		return (fft_size >> 1);
	}

	virtual void Init(float* buffer, int32_t fft_size, int32_t num_textures,
										float sample_rate_hz, FFT* fft);

	virtual void Process(const Parameters& parameters, float* fft_out, float* ifft_in, bool trigger);

private:
	void Reset(int32_t num_textures);
	void RectangularToPolar(float* fft_data);
	void PolarToRectangular(float* mags, float* fft_data);


	inline void fast_p2r(float magnitude, uint16_t angle, float* re,
			float* im) {
		angle >>= 6;
		*re = magnitude * lut_sin[angle + 256];
		*im = magnitude * lut_sin[angle];
	}

	FFT* fft_;

	int32_t size_;

	float* buffers_[kMaxNumTextures];

	uint16_t* phases_;
	uint16_t* previous_mag_;
	uint16_t* band_gain_target_;
	uint16_t* band_gain_state_;

	float current_num_freq_bands_parameter_;

	DISALLOW_COPY_AND_ASSIGN (SpectralCloudsTransformation);
};

}  // namespace clouds

#endif  // CLOUDS_DSP_PVOC_SPECTRAL_CLOUDS_TRANSFORMATION_H_
