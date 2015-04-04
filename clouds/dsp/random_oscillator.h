#include "clouds/resources.h"
#include "stmlib/utils/random.h"
#include "stmlib/utils/dsp.h"

using namespace clouds;
using namespace stmlib;

/* Smoothed random generator, outputs values between -1 and 1 */
class RandomOscillator
{
public:

  void Init() {
    value_ = Random::GetSample();
    next_value_ = Random::GetSample();
  }

  inline void set_period(uint32_t period) {
    phase_increment_ = UINT32_MAX / period;
  }

  float Next() {
    phase_ += phase_increment_;
    if (phase_ < phase_increment_) {
      value_ = next_value_;
      next_value_ = Random::GetSample();
    }
    int16_t raised_cosine = Interpolate824(clouds::lut_raised_cosine, phase_) >> 1;
    return (value_ + ((next_value_ - value_) * raised_cosine >> 15)) / 32767.0f;
  }

private:
  uint32_t phase_;
  uint32_t phase_increment_;
  int32_t value_;
  int32_t next_value_;

};
