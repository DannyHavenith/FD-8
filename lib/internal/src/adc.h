#ifndef ADC_H_
#define ADC_H_

#include <stdint.h>

#define NO_INLINE __attribute__ ((noinline))

class adc
{
public:
    /// Read a raw ADC value.
    uint16_t read();

    /// Set up the ADC configuration registers to start sampling from the given channel.
    void init(uint8_t channel);

private:
    /// template meta function that, given a cpu frequency and a required maximum frequency, finds a divider (as a power of two) that
    /// will result in the highest result frequency that is at most the given result frequency.
    template<uint16_t cpu_khz, uint16_t max_khz, uint8_t proposed = 0, bool result_fits = cpu_khz/(1<<proposed) <= max_khz>
    struct divider{};

    template<uint16_t cpu_khz, uint16_t max_khz, uint8_t proposed>
    struct divider<cpu_khz, max_khz, proposed, true>
    {
        static const uint8_t value = proposed;
    };

    template<uint16_t cpu_khz, uint16_t max_khz, uint8_t proposed>
    struct divider<cpu_khz, max_khz, proposed, false>
    {
        // the proposed divider still resulted in a too high frequency,
        // try a higher divider (power of two).
        static const uint8_t value = divider<cpu_khz, max_khz, (proposed + 1)>::value;
    };

    /// Tell the ADC component to start measurement.
    void start();

    /// Wait until the ADC has finished its measurement
    void wait_for_result();

    /// read the ADC register.
    uint16_t read_register();

};

#endif // ADC_H_
