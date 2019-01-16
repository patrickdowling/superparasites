#ifndef CLOUDS_DAC_H
#define CLOUDS_DAC_H

#include "stmlib/stmlib.h"

namespace clouds {

class Dac {
    public:
        Dac() { }
        ~Dac() { }
      
        void Init();
        void DeInit();
        void StartNoise();
        void StopNoise();
        void SetNoiseFreq(float period);

        inline bool is_generating_noise() {
            return generating_noise_;
        }

        inline void set_output(float val) {
            output_ = (uint16_t)(val * 4095.0f);
        }

        inline void set_raw_output(uint16_t val) {
            output_ = val;
        }


    private:
        uint16_t output_;
        bool generating_noise_;

}; // class

} // namespace 


#endif
