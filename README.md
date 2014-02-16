FD-8 replacement.
================

This is the code to a replacement device for the internals of a Roland FD-8 highhat pedal. The hardware is described at http://rurandom.org/justintime/index.php?title=DigitalFD8

This code reads values from one of the ADC channels, which is assumed to be connected to a Hall effect sensor. The pedal should have a magnet attached that is brought close to 
the sensor when the pedal is pushed down. Typically, such a sensor will output either values from 1/2 Vcc down to Vss or values from 1/2 Vcc upto Vcc, dependent on the orientation 
of the magnet that is attached to the pedal. This code will monitor the values that arrive, and from that tries to determine what the orientation of the magnet is, and what the 
range of values is that the ADC reports. 

The range of values is then mapped onto the range 0-255, which is subsequently send to a digital potentiometer through SPI. The potentiometer emulates the behavior of the film 
variable resistor that is normally in the FD-8 pedal.

This code is intended to run on an Attiny13, on any clock frequency that is appropriate for this MCU.