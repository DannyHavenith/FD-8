#include "adc.h"

#ifndef UNIT_TEST

#include <avr/io.h>

uint16_t NO_INLINE adc::read()
{
    start();
    wait_for_result();
    return read_register();
}

void adc::init(uint8_t channel)
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

void adc::start()
{
    // set ADSC in ADCSRA
    ADCSRA |= _BV( ADSC);
}

void adc::wait_for_result()
{
    // wait--spinlock--for the ADSC bit to become zero again.
    while( (ADCSRA & _BV(ADSC)));
}

uint16_t NO_INLINE adc::read_register()
{
    // read ADCL and ADCH in the right order.
    uint16_t result= ADCL;
    result |= (static_cast<uint16_t>( ADCH) << 8);
    return result;
}

#else

uint16_t NO_INLINE adc::read()
{
    return 0;
}

void adc::init(uint8_t channel)
{
}


#endif