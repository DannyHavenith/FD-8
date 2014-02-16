/*
 * fd8.cpp
 *
 *  Created on: Aug 11, 2013
 *      Author: danny
 */
#include <avr/io.h>
#include <util/delay.h>
#include "avr_utilities/devices/bitbanged_spi.h"

#define NO_INLINE __attribute__ ((noinline))

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
	/// maximum value received from adc. This is used to scale the output.
	/// Note that this variable is signed. Which is no problem since ADC values are 10 bits only, so they'll always remain positive.
	int16_t max_measured_adc = 1 << 9;

	/// minimum value received from adc. This is used to scale the output.
	/// Note that this variable is signed. Which is no problem since ADC values are 10 bits only, so they'll always remain positive.
	int16_t min_measured_adc = 1 << 9;

	int32_t translation_scale = 1;
	int16_t translation_offset = 0;

	typedef bitbanged_spi<spi_pins> spi;

	/// Write a value (0-255) to the digital potentiometer by spi.
	void NO_INLINE write_pot( uint8_t value)
	{
		reset( select_potmeter);
		// send command (xx01xx11, 01 = write, 11 = "both potmeters"
		spi::transmit_receive( (uint8_t)0b00010011);
		spi::transmit_receive( value);
		set( select_potmeter);
	}


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

	/// Wait until the ADC has finished its measurement
	void wait_for_adc_result()
	{
		// wait--spinlock--for the ADSC bit to become zero again.
		while( (ADCSRA & _BV(ADSC)));
	}

	uint16_t NO_INLINE read_adc()
	{
		// read ADCL and ADCH in the right order.
		uint16_t result= ADCL;
		result |= (static_cast<uint16_t>( ADCH) << 8);
		return result;
	}

	/// From the maximum and minimum measured ADC values determine:
	/// * the direction of the pedal (whether the voltage goes up or down when the pedal is depressed).
	/// * the range of the pedals movement (in terms of ADC measured voltage).
	void NO_INLINE rescale_range()
	{
		static const int16_t mid_point = 1<<9; // the middle of the ADC range.

		// calculate the median point of the range
		if ( (max_measured_adc + min_measured_adc)/2 < mid_point)
		{
			// if the median is below the midpoint, we assume that the voltage goes down as the pedal goes down.
			translation_offset = min_measured_adc;
			translation_scale = max_measured_adc - min_measured_adc + 1; // scale is positive
		}
		else
		{
			// the median is above the midpoint, voltage goes up as pedal goes down.
			translation_offset = max_measured_adc;
			translation_scale = min_measured_adc - max_measured_adc + 1; // scale is negative.
		}
	}

	void note_max_min( int16_t value)
	{
		if (value < min_measured_adc)
		{
			min_measured_adc = value;
			rescale_range();
		}
		if (value > max_measured_adc)
		{
			max_measured_adc = value;
			rescale_range();
		}
	}


	/// scale the 10-bit raw ADC value down to an 8-bit value.
	/// This takes into account the observed maximum and minimum values of the ADC, to scale the
	/// observed range to the full potmeter range.
	uint8_t scale_down( uint16_t raw_value)
	{
		int32_t accumulator = (static_cast<int16_t>(raw_value)-translation_offset) * 256;
		accumulator /= translation_scale;
		return accumulator;
	}

	uint8_t NO_INLINE read_pedal()
	{
		start_adc();
		wait_for_adc_result();
		uint16_t adcvalue = read_adc();
		note_max_min( adcvalue);
		return scale_down( adcvalue);
	}

	void NO_INLINE delay()
	{
		_delay_ms(5000);
	}
} // unnamed namespace

int main()
{

	set( select_potmeter);
	spi::init();
	init_adc( 2);
	make_output( select_potmeter);

	// initialize the adc range by sampling the ADC values and
	// determining an initial scale.
	max_measured_adc = min_measured_adc = read_adc();
	for (uint8_t count = 10; count; --count)
	{
		note_max_min( read_adc());
	}
	rescale_range();

	for(;;)
	{
		uint8_t val = read_pedal();
		if (val > 128) reset( led);
		else set(led);
		write_pot( val);
	}
}
