//
//  Copyright (C) 2014 Danny Havenith
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//

/**
 * FD-8 replacement.
 *
 * This is the code to a replacement device for the internals of a Roland FD-8 highhat pedal.
 * The hardware is described at http://rurandom.org/justintime/index.php?title=DigitalFD8
 *
 * This code reads values from one of the ADC channels, which is assumed to be connected to
 * a Hall effect sensor. The pedal should have a magnet attached that is brought close to the sensor
 * when the pedal is pushed down.
 * Typically, such a sensor will output either values from 1/2 Vcc down to Vss or
 * values from 1/2 Vcc upto Vcc, dependent on the orientation of the magnet that is attached to the pedal.
 * This code will monitor the values that arrive, and from that tries to determine what the orientation of the
 * magnet is, and what the range of values is that the ADC reports.
 *
 * The range of values is then mapped onto the range 0-255, which is subsequently send to a digital potentiometer
 * through SPI. The potentiometer emulates the behavior of the film variable resistor that is normally in the
 * FD-8 pedal.
 */

#include <avr/io.h>
#include <util/delay.h>
#include "avr_utilities/devices/bitbanged_spi.h"
#include "avr_utilities/pin_definitions.hpp"

#define NO_INLINE __attribute__ ((noinline))

/// This struct defines which pins are used by the bit banging spi device
struct spi_pins {
	DEFINE_PIN( mosi, B, 0);
	DEFINE_PIN( miso, B, 1);
	DEFINE_PIN( clk,  B, 2);
};
DEFINE_PIN( select_potmeter, B, 1);

namespace
{

	// the following globals are not initialized on purpose to save a few bytes of code space.

	/// maximum value received from adc. This is used to scale the output.
	/// Note that this variable is signed. Which is no problem since raw ADC values are 10 bits only, so they'll always remain positive.
	int16_t max_raw_value;

	/// minimum value received from adc. This is used to scale the output.
	/// Note that this variable is signed. Which is no problem since raw ADC values are 10 bits only, so they'll always remain positive.
	int16_t min_raw_value;

	// calibration values.
	int32_t translation_scale; ///<this is the distance between lowest ever and highest ever value measured. Negative if voltage increases with pedal-down.
	int16_t translation_offset; ///< this is either the lowest or highest raw value measured, depending on whether voltage goes up or down if pedal goes down.
	const int16_t scale_cutoff = 6;

	typedef bitbanged_spi<spi_pins> spi;

	/// This is not so much a true class as a set of ADC-related functions. All functions are static, we don't want to burden all these
	/// functions with an extra implicit 'this' argument.
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
		static void init(uint8_t channel)
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

	/// Write a value (0-255) to the digital potentiometer by spi.
	void NO_INLINE write_pot( uint8_t value)
	{
		reset( select_potmeter); // select_potmeter is active-low.
		// send command (xx01xx11, 01 = write, 11 = "both potmeters"
		spi::transmit_receive( (uint8_t)0b00010001);
		spi::transmit_receive( value);
		set( select_potmeter);
	}


	/// From the maximum and minimum measured ADC values determine:
	/// * the direction of the pedal (whether the voltage goes up or down when the pedal is depressed).
	/// * the range of the pedals movement (in terms of ADC measured voltage).
	void NO_INLINE rescale_range()
	{
		static const int16_t mid_point = 1<<9; // the middle of the ADC range.

		// calculate the median point of the range
		if ( (max_raw_value + min_raw_value)/2 < mid_point)
		{
			// if the median is below the midpoint, we assume that the voltage goes down as the pedal goes down.
			translation_offset = min_raw_value;
			translation_scale = max_raw_value - min_raw_value + 1; // scale is positive

			// adapt the range so that the lower 1/8 of the range all registers as '0' (maximum down position)
			translation_offset -= translation_scale/scale_cutoff;
			translation_scale -= translation_scale/scale_cutoff;
		}
		else
		{
			// the median is above the midpoint, voltage goes up as pedal goes down.
			translation_offset = max_raw_value;
			translation_scale = min_raw_value - max_raw_value - 1; // scale is negative.

			// adapt the range so that the lower 1/8 of the range all registers as '0' (maximum down position)
			translation_offset += translation_scale/scale_cutoff;
			translation_scale += translation_scale/scale_cutoff;
		}
	}

	/// Take a raw value and adapt the max or min-value accordingly.
	/// If the min- or max value is adapted then the calibration range will be adapted as well.
	void note_max_min( int16_t raw_value)
	{
		if (raw_value < min_raw_value)
		{
			min_raw_value = raw_value;
			rescale_range();
		}
		if (raw_value > max_raw_value)
		{
			max_raw_value = raw_value;
			rescale_range();
		}
	}

	/// scale the 10-bit raw ADC value down to an 8-bit value.
	/// This takes into account the observed maximum and minimum values of the ADC, to scale the
	/// observed range to the full potmeter range.
	uint8_t scale_down( uint16_t raw_value)
	{
		int32_t accumulator = static_cast<int32_t>(static_cast<int16_t>(raw_value)-translation_offset) * 256;
		accumulator /= translation_scale;

		// the lower part of the range maps onto negative values (see rescale_range() ). All these negative values
		// are clipped to 0.
		if (accumulator < 0) accumulator = 0;
		return accumulator;
	}


	/// Read the ADC value, but use auto-calibration data to scale the value into
	/// a value in the range 0-255, where 255 represents the highest position of the pedal ever encountered
	/// and 0 means the lowest position ever encountered.
	uint8_t  read_scaled_pedal()
	{
		uint16_t adcvalue = adc::read();
		note_max_min( adcvalue);
		return scale_down( adcvalue);
	}

	/// Read a few values from the ADC and determine first values for min and max ADC values
	/// to start off the auto calibration.
	void init_pedal_calibration() {
		for (uint8_t count = 10; count; --count) {
			adc::read();
		}
		max_raw_value = min_raw_value = adc::read();
		for (uint8_t count = 10; count; --count) {
			note_max_min(adc::read());
		}
		rescale_range();
	}

} // unnamed namespace


int main()
{

	set( select_potmeter);
	spi::init();
	adc::init( 2);
	make_output( select_potmeter);

	init_pedal_calibration();
	for(;;)
	{
		_delay_ms( 1);
		uint8_t val = read_scaled_pedal();
		write_pot( val);
	}
}
