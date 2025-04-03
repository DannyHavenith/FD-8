#ifndef MAPPER_H_
#define MAPPER_H_

#include "adc.h"
#include <stdint.h>

struct NullListener {
	void onRawAdcValue(uint16_t adcValue) const {}
	void onCalibrationSet(int16_t minRawValue, int16_t maxRawValue, int32_t translationScale, int16_t translationOffset) const {}
	void onMapped(int32_t value) const {}
};

template <typename Listener = NullListener>
class PedalMapper {
public:
	PedalMapper(Listener listener = NullListener()) : listener(listener) { }

	/// Read a few values from the ADC and determine first values for min and max ADC values
	/// to start off the auto calibration.
	void init_pedal_calibration(adc &adc) {
		for (uint8_t count = 10; count; --count) {
			adc.read();
		}
		max_raw_value = min_raw_value = adc.read();
		for (uint8_t count = 10; count; --count) {
			note_max_min(adc.read());
		}
		rescale_range();
	}

	/// Read the ADC value, but use auto-calibration data to scale the value into
	/// a value in the range 0-255, where 255 represents the highest position of the pedal ever encountered
	/// and 0 means the lowest position ever encountered.
	uint8_t read_scaled_pedal(adc &adc)
	{
		uint16_t adcvalue = adc.read();
		listener.onRawAdcValue(adcvalue);
		note_max_min( adcvalue);
		return scale_down( adcvalue);
	}

private:
	Listener listener;

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
	static const int16_t scale_cutoff = 6;

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
		listener.onCalibrationSet(min_raw_value, max_raw_value, translation_scale, translation_offset);
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
		listener.onMapped(accumulator);
		return accumulator;
	}	
};

#endif // MAPPER_H_