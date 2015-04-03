#include "stmlib/utils/random.h"

using namespace stmlib;

/* Smoothed random generator, outputs values between -1 and 1 */
class RandomOscillator
{
public:

  inline void Init(int period) {
    smoothed_ = Random::GetSample();
    val_ = Random::GetSample();
    period_ = period;
  }

  inline void set_period(int period) {
    period_ = period;
  }

  inline float Next() {
    if (period_ != 0) {
        if (phase_ % period_ == 0)
            val_ = Random::GetSample();
        smoothed_ += (val_ - smoothed_) * (1.0f / period_);
    }
    phase_++;
    return smoothed_ / 32768.0f;
  }

private:
  int period_;
  float smoothed_;
  float val_;
  int phase_;

};
