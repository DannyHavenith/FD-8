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
DEFINE_PIN(debug, B, 3);

namespace
{
	typedef bitbanged_spi<spi_pins> spi;

	/// Write a value (0-255) to the digital potentiometer by spi.
	void NO_INLINE write_pot( uint8_t value)
	{
		reset( select_potmeter); // select_potmeter is active-low.
		// send command (xx01xx11, 01 = write, 11 = "both potmeters"
		spi::transmit_receive( (uint8_t)0b00010011);
		spi::transmit_receive( value);
		set( select_potmeter);
	}

	/// Write a generic 16-bit value to spi using the same select line as the digital potentiometer.
	/// To be used for debugging only, i.e. with no digipot connected.
	void dump_to_spi(uint16_t value) {
		reset( select_potmeter);
		spi::transmit_receive( static_cast<uint8_t>(value >> 8));
		spi::transmit_receive( static_cast<uint8_t>(value & 0xff));
		set( select_potmeter);
	}
	
	/// Write a generic 32-bit value to spi using the same select line as the digital potentiometer.
	/// To be used for debugging only, i.e. with no digipot connected.
	void dump_to_spi(uint32_t value) {
		reset( select_potmeter);
		spi::transmit_receive( static_cast<uint8_t>( value >> 24));
		spi::transmit_receive( static_cast<uint8_t>((value >> 16) & 0xff));
		spi::transmit_receive( static_cast<uint8_t>((value >> 8)  & 0xff));
		spi::transmit_receive( static_cast<uint8_t>( value        & 0xff));
		set( select_potmeter);
	}
	
	void dump_to_spi(int16_t value)
	{ 
		dump_to_spi(static_cast<uint16_t>(value));
	}
	
	void dump_to_spi(int32_t value)
	{ 
		dump_to_spi(static_cast<uint32_t>(value));
	}

	/// A PedalMapper Listener-compatible struct that dumps all listenable values to SPI.
	struct SpiPedalDumper {
		void onRawAdcValue(uint16_t adcValue) {
			dump_to_spi(adcValue);
		};
	
		void onCalibrationSet(int16_t minRawValue, int16_t maxRawValue, int32_t translationScale, int16_t translationOffset) {
			dump_to_spi(minRawValue);
			dump_to_spi(maxRawValue);
			dump_to_spi(translationScale);
			dump_to_spi(translationOffset);
		}
	
		void onMapped(int32_t value) {
			dump_to_spi(value);
		}
	};
	
} // unnamed namespace


SpiPedalDumper mapperListener;

int main()
{
	make_output(debug);
	set(debug);

	set( select_potmeter);
	spi::init();

	adc adc;
	adc.init( 2);
	make_output( select_potmeter);

	PedalMapper<> mapper;
	// PedalMapper<SpiPedalDumper> mapper(mapperListener);

	mapper.init_pedal_calibration(adc);
	for(;;)
	{
		_delay_ms( 1);
		reset(debug);
		uint8_t val = mapper.read_scaled_pedal(adc);
		write_pot( val);
		set(debug);
	}
}
#else

// minimal main() required for linking in native environment
// (when unit-testing, PIO uses main() in tests)
int main() { }

#endif // UNIT_TEST
