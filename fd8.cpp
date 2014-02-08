/*
 * fd8.cpp
 *
 *  Created on: Aug 11, 2013
 *      Author: danny
 */
#include <avr/io.h>
#include <util/delay.h>
#include "avr_utilities/devices/bitbanged_spi.h"

/// This struct defines which pins are used by the bit banging spi device
struct spi_pins {
	DEFINE_PIN( mosi, B, 0);
	DEFINE_PIN( miso, B, 1);
	DEFINE_PIN( clk,  B, 2);
};
DEFINE_PIN( select_potmeter, B, 1);
DEFINE_PIN( led, B, 3);

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

template<uint16_t value>
struct dummy {};

namespace
{
	typedef bitbanged_spi<spi_pins> spi;
	void __attribute__ ((noinline)) write_pot( uint8_t value)
	{
		reset( select_potmeter);
		// send command (xx01xx11, 01 = write, 11 = "both potmeters"
		spi::transmit_receive( (uint8_t)0b00010011);
		spi::transmit_receive( value);
		set( select_potmeter);
	}


	// bunch of adc functions. To be added to adc class

	/// Tell the ADC component to start measurement.
	void start_adc()
	{
		// set ADSC in ADCSRA
		ADCSRA |= _BV( ADSC);
	}

	void init_adc(uint8_t channel)
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

	void wait_for_adc_result()
	{
		// wait--spinlock--for the ADSC bit to become zero again.
		while( (ADCSRA & _BV(ADSC)));
	}

	uint16_t read_adc()
	{
		// read ADCL and ADCH in the right order.
		uint16_t result= ADCL;
		result |= (static_cast<uint16_t>( ADCH) << 8);
		return result;
	}

	uint8_t __attribute__ ((noinline)) read_pedal()
	{
		start_adc();
		wait_for_adc_result();

		return read_adc() >> 2;
	}

	void __attribute__ ((noinline)) delay()
	{
		_delay_ms(5000);
	}
} // unnamed namespace

int main()
{

	set( select_potmeter);
	spi::init();
	init_adc( 2);
	make_output( led | select_potmeter);
	for(;;)
	{
		uint8_t val = read_pedal();
		if (val > 128) reset( led);
		else set(led);

		write_pot( val);
	}
}
