#include <stdint.h>
#include <avr/io.h>

#define NO_INLINE __attribute__ ((noinline))

class adc
{
public:
    /// Read a raw ADC value.
    static uint16_t NO_INLINE read()
    {
        start();
        wait_for_result();
        return read_register();
    }

    /// Set up the ADC configuration registers to start sampling from the given channel.
    void init(uint8_t channel)
    {
        // given the clock frequency, determine which power of 2 we need to bring it down to
        // 200kHz.
        static const uint8_t div =  divider< F_CPU/1000, 200>::value;

        //set ADMUX, channel, REFS0 = Vcc (0) ADLAR = right adjust (0)
        ADMUX = (channel & 0x03) << MUX0;

        // set ADC Enable, ADATE none (0), clock div factor
        ADCSRA = _BV( ADEN) | (div & 0x03);

        ADCSRB = 0;

    }

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
    static void start()
    {
        // set ADSC in ADCSRA
        ADCSRA |= _BV( ADSC);
    }

    /// Wait until the ADC has finished its measurement
    static void wait_for_result()
    {
        // wait--spinlock--for the ADSC bit to become zero again.
        while( (ADCSRA & _BV(ADSC)));
    }

    /// read the ADC register.
    static uint16_t NO_INLINE read_register()
    {
        // read ADCL and ADCH in the right order.
        uint16_t result= ADCL;
        result |= (static_cast<uint16_t>( ADCH) << 8);
        return result;
    }

};
