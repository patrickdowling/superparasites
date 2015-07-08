#include "clouds/resources.h"
#include "stmlib/utils/random.h"
#include "stmlib/utils/dsp.h"

using namespace clouds;
using namespace stmlib;

/* Smoothed random generator, outputs values between -1 and 1 */
class RandomOscillator
{
public:

  inline float getFloat()
  {
    return Random::GetFloat() * 2.0f - 1.0f;
  }

  void Init() {
    value_ = getFloat();
    next_value_ = getFloat();
  }

  inline void set_period(float period) {
    phase_increment_ = 1.0f / period;
  }

  float Next() {
    phase_ += phase_increment_;
    if (phase_ > 1.0f) {
      phase_ = 0.0f;
      value_ = next_value_;
      next_value_ = getFloat();
    }
    float sin = Interpolate(lut_window, phase_, LUT_WINDOW_SIZE-1);
    return value_ + (next_value_ - value_) * sin;
  }

private:
  float phase_;
  float phase_increment_;
  float value_;
  float next_value_;
};
