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
// Transformations applied to a single STFT slice.

#include "supercell/dsp/pvoc/spectral_clouds_transformation.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <numeric>

#include "stmlib/dsp/atan.h"
#include "stmlib/dsp/units.h"
#include "stmlib/utils/random.h"

#include "supercell/dsp/frame.h"
#include "supercell/dsp/parameters.h"

namespace clouds {

using namespace std;
using namespace stmlib;

static const size_t kMaxFilterBankBands = 128;
static const size_t kMaxRandTriggerValue = 30;

void SpectralCloudsTransformation::Init(float* buffer, int32_t fft_size,
		int32_t num_textures, float sample_rate_hz, FFT* fft) {
	fft_ = fft;
	size_ = fft_size / 2;

	for (int32_t i = 0; i < num_textures; ++i) {
		buffers_[i] = &buffer[i * size_];
	}
	phases_ = static_cast<uint16_t*>((void*) (buffers_[num_textures - 1]));
	previous_mag_ = phases_ + size_;

	band_gain_target_ = static_cast<uint16_t*>((void*) (buffers_[num_textures
			- 2]));
	band_gain_state_ = band_gain_target_ + size_;

	current_num_freq_bands_parameter_ = 1.0f;

	Reset(num_textures);
}

void SpectralCloudsTransformation::Reset(int32_t num_textures) {
	for (int32_t i = 0; i < num_textures; ++i) {
		fill(&buffers_[i][0], &buffers_[i][size_], 0.0f);
	}
}

void SpectralCloudsTransformation::Process(const Parameters& parameters,
		float* fft_out, float* ifft_in, bool trigger) {
	fft_out[0] = 0.0f;
	fft_out[size_] = 0.0f;

	float density_threshold = 1.0f - parameters.position;
	CONSTRAIN(density_threshold, 0.0f, 1.0f);
	float num_freq_bands_parameter = parameters.size;
	CONSTRAIN(num_freq_bands_parameter, 0.0f, 1.0f);
	float parameter_low_pass_parameter = parameters.density; //parameters.kammerl.pitch;
	CONSTRAIN(parameter_low_pass_parameter, 0.0f, 1.0f);

	float rand_trigger_parameter = parameters.kammerl.clock_divider;
	CONSTRAIN(rand_trigger_parameter, 0.0f, 1.0f);
	float phase_randomization_parameter = parameters.texture;
	CONSTRAIN(phase_randomization_parameter, 0.0f, 1.0f);

	const bool freeze = parameters.freeze;

	bool rand_trigger =
			rand_trigger_parameter < 0.1 ?
					false :
					(stmlib::Random::GetWord()
							% static_cast<uint32_t>((1.0f
									- rand_trigger_parameter)
									* kMaxRandTriggerValue + 1) == 0);

	const float parameter_low_pass =
			parameter_low_pass_parameter < 0.1 ?
					0.0f : 0.9f + parameter_low_pass_parameter * 0.10f;

	if (trigger || rand_trigger) {
		for (size_t i = 0; i < kMaxFilterBankBands; ++i) {
			band_gain_target_[i] =
					static_cast<uint16_t>(stmlib::Random::GetWord() & 0xFFFFU);
		}
	}

	for (size_t i = 0; i < kMaxFilterBankBands; ++i) {
		band_gain_state_[i] = parameter_low_pass * band_gain_state_[i]
				+ (1.0f - parameter_low_pass) * band_gain_target_[i];
	}
	current_num_freq_bands_parameter_ = parameter_low_pass
			* current_num_freq_bands_parameter_
			+ (1.0f - parameter_low_pass) * num_freq_bands_parameter;

	float* magnitudes = &fft_out[0];
	if (!freeze) {
		RectangularToPolar(fft_out);
		float max_magnitude = std::max(1e-5f,
				*std::max_element(magnitudes, magnitudes + size_));
		// store float scalar at the beginning.
		*reinterpret_cast<float*>(previous_mag_) = max_magnitude;
		for (int i = 1; i < size_ - 1; ++i) {
			previous_mag_[i + 1] = std::min(
					magnitudes[i] / max_magnitude * 0xFFFFU,
					static_cast<float>(0xFFFFU));
		}
	} else {
		const float max_previous_mag = *reinterpret_cast<float*>(previous_mag_);
		for (int i = 1; i < size_ - 1; ++i) {
			magnitudes[i] = static_cast<float>(previous_mag_[i + 1])
					/ static_cast<float>(0xFFFFU) * max_previous_mag;
		}
	}

	int32_t amount = static_cast<int32_t>(phase_randomization_parameter
			* 32768.0f);
	for (int32_t i = 1; i < size_ - 1; ++i) {
		phases_[i] += static_cast<int32_t>(stmlib::Random::GetSample()) * amount
				>> 14;
	}
	size_t band_idx = 0;
	const float base = Interpolate(lut_freq_log, current_num_freq_bands_parameter_, LUT_FREQ_LOG_SIZE-1);
	float interval = base;
	float remainder = interval;

	float band_mag = static_cast<float>(band_gain_state_[band_idx])
			/ static_cast<float>(0xFFFFU);

	for (int i = 1; i < size_ - 1; ++i) {
		float band_gain = (density_threshold > band_mag) ? 0.0f : band_mag;
		static float kSqrt2 = 1.4142f;
		band_gain *= kSqrt2;
		magnitudes[i] *= band_gain;
		remainder -= 1.0f;
		if (remainder <= 0) {
			++band_idx;
			band_idx = band_idx % kMaxFilterBankBands;
			interval *= base;
			remainder = interval;
			band_mag = static_cast<float>(band_gain_state_[band_idx])
					/ static_cast<float>(0xFFFFU);
		}
	}

	PolarToRectangular(fft_out, ifft_in);

	ifft_in[0] = 0.0f;
	ifft_in[size_] = 0.0f;
}

void SpectralCloudsTransformation::RectangularToPolar(float* fft_data) {
	float* real = &fft_data[0];
	float* imag = &fft_data[size_];
	float* magnitude = &fft_data[0];
	for (int32_t i = 1; i < size_; ++i) {
		phases_[i] = fast_atan2r(imag[i], real[i], &magnitude[i]);
	}
}

void SpectralCloudsTransformation::PolarToRectangular(float* mags,
		float* fft_out) {
	float* real = &fft_out[0];
	float* imag = &fft_out[size_];
	for (int32_t i = 1; i < size_; ++i) {
		fast_p2r(mags[i], phases_[i], &real[i], &imag[i]);
	}
}

}  // namespace clouds
