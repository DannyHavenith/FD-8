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

// This file contains AVR-specific stuff. Unit tests should not deal with it.
#ifndef UNIT_TEST

#include <util/delay.h>
#include "avr_utilities/devices/bitbanged_spi.h"
#include "avr_utilities/pin_definitions.hpp"

#include "adc.h"
#include "mapper.h"

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
	typedef bitbanged_spi<spi_pins> spi;

	/// Write a value (0-255) to the digital potentiometer by spi.
	void NO_INLINE write_pot( uint8_t value)
	{
		reset( select_potmeter); // select_potmeter is active-low.
		// send command (xx01xx11, 01 = write, 11 = "both potmeters"
		spi::transmit_receive( (uint8_t)0b00010001);
		spi::transmit_receive( value);
		set( select_potmeter);
	}
} // unnamed namespace


int main()
{

	set( select_potmeter);
	spi::init();

	adc adc;
	adc.init( 2);
	make_output( select_potmeter);

	PedalMapper mapper;

	mapper.init_pedal_calibration(adc);
	for(;;)
	{
		_delay_ms( 1);
		uint8_t val = mapper.read_scaled_pedal(adc);
		write_pot( val);
	}
}

#endif // UNIT_TEST
