#ifndef MAPPER_H_
#define MAPPER_H_

#include "adc.h"
#include <stdint.h>

struct PedalMapperListener {
	virtual void onRawAdcValue(uint16_t adcValue) = 0;
	virtual void onCalibrationSet(int16_t minRawValue, int16_t maxRawValue, int32_t translationScale, int16_t translationOffset) = 0;
	virtual void onMapped(int32_t value) = 0;
};

struct NullListener : PedalMapperListener {
	virtual void onRawAdcValue(uint16_t) {};
	virtual void onCalibrationSet(int16_t, int16_t, int32_t, int16_t) {};
	virtual void onMapped(int32_t value) {};
};
extern NullListener nullListener;

class PedalMapper {
public:
	PedalMapper(PedalMapperListener &listener = nullListener) : listener(listener) { }

	/// Read a few values from the ADC and determine first values for min and max ADC values
	/// to start off the auto calibration.
	void init_pedal_calibration(adc &adc);

	/// Read the ADC value, but use auto-calibration data to scale the value into
	/// a value in the range 0-255, where 255 represents the highest position of the pedal ever encountered
	/// and 0 means the lowest position ever encountered.
	uint8_t read_scaled_pedal(adc &adc);

private:
	PedalMapperListener &listener;

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
    void rescale_range();

	/// Take a raw value and adapt the max or min-value accordingly.
	/// If the min- or max value is adapted then the calibration range will be adapted as well.
	void note_max_min( int16_t raw_value);

    /// scale the 10-bit raw ADC value down to an 8-bit value.
	/// This takes into account the observed maximum and minimum values of the ADC, to scale the
	/// observed range to the full potmeter range.
	uint8_t scale_down( uint16_t raw_value);

};

#endif // MAPPER_H_