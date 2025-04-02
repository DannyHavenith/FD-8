#include "adc.h"
#include "mapper.h"


#define NO_INLINE __attribute__ ((noinline))

void PedalMapper::init_pedal_calibration(adc &adc) {
	for (uint8_t count = 10; count; --count) {
		adc.read();
	}
	max_raw_value = min_raw_value = adc.read();
	for (uint8_t count = 10; count; --count) {
		note_max_min(adc.read());
	}
	rescale_range();
}


void NO_INLINE PedalMapper::rescale_range()
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
		translation_scale += translation_scale/scale_cutoff;
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

void PedalMapper::note_max_min( int16_t raw_value)
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

uint8_t PedalMapper::scale_down( uint16_t raw_value)
{
	int32_t accumulator = static_cast<int32_t>(static_cast<int16_t>(raw_value)-translation_offset) * 256;
	accumulator /= translation_scale;

	// the lower part of the range maps onto negative values (see rescale_range() ). All these negative values
	// are clipped to 0.
	if (accumulator < 0) accumulator = 0;
	return accumulator;
}


uint8_t PedalMapper::read_scaled_pedal(adc &adc)
{
	uint16_t adcvalue = adc.read();
	note_max_min( adcvalue);
	return scale_down( adcvalue);
}