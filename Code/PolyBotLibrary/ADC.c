//Copyright (C) 2009-2011  Darron Baida and Patrick J. McCarty.
//Licensed under X11 License. See LICENSE.txt for details.

/*! @file
    Implements support for the Analog to Digital Converter (ADC).
    Provides functions for reading the 8 single-ended analog inputs with both 8-bit and 10-bit resolution.
    Functions are implemented synchronously, so the code will block while waiting for the conversion to complete.
    With a prescaler of 128, each conversion takes:
    62.5 ns/cpucycle * 128 cpucycle/aclock * 13 aclock/conversion = 104 us/conversion.
 */

#include "ADC.h"
#include <util/delay.h>

/*! Initialize ADC.
    Normally called only by the initialize() function in utility.c.
 */
void adcInit()
{
	//Use AREF as the ADC voltage reference, with internal Vref turned off. Right shifting.
	ADMUX = 0;

	//Enable ADC and set prescaler to /128 to yield 125kHz (PolyBot 1.0) or 156.25kHz (PolyBot 1.1) ADC clock.
	ADCSRA |= _BV(ADEN) | _BV(ADPS2) | _BV(ADPS1)| _BV(ADPS0);

	//Configure PA5, PA6, and PA7 as outputs to control the 4051N analog mux.
	DDRA |= 0xE0;
}

/*! Returns an 8-bit resolution reading of the specified analog input.
    @param num The analog input to sample (0 to 7).
    @return The 8-bit reading or 0xBD if an invalid input number was passed.
 */
u08 analog(const u08 num)
{
	//validate num parameter and return early if num is out of range
	if (num > 9)
	{
		return 0xBD;
	}
	//if the input is less than 7, then it is an input connected to the 4051N analog mux.
	else if (num < 7)
	{
		//clear the three 4051N analog mux selection bits (7:5)
		PORTA &= 0x1F;
		//set the three 4051N analog mux selection bits for the specified input
		PORTA |= ((num + 1) << 5);
		//Set left shifting and select input channel 0
		ADMUX = _BV(ADLAR);
		//wait for the voltage to stabilize
		_delay_loop_1(100);
	}
	else
	{
		//Set left shifting and select the input channel
		ADMUX = _BV(ADLAR) | (num - 6);
	}

	//start conversion
	ADCSRA |= _BV(ADSC);
	//wait for conversion to complete
	loop_until_bit_is_clear(ADCSRA, ADSC);

	//return 8-bit result
	return ADCH;
}

/*! Returns a 10-bit resolution reading of the specified analog input.
    @param num The analog input to sample (0 to 7).
    @return The 10-bit reading or 0xBAD if an invalid input number was passed.
 */
u16 analog10(const u08 num)
{
	//validate num parameter and return early if num is out of range
	if (num > 9)
	{
		return 0x0BAD;
	}
	//if the input is less than 7, then it is an input connected to the 4051N analog mux.
	else if (num < 7)
	{
		//clear the three 4051N analog mux selection bits (7:5)
		PORTA &= 0x1F;
		//set the three 4051N analog mux selection bits for the specified input
		PORTA |= ((num + 1) << 5);
		//Set right shifting and select input channel 0
		ADMUX = 0;
		//wait for the voltage to stabilize
		_delay_loop_1(100);
	}
	else
	{
		//Set right shifting and select the input channel
		ADMUX = num - 6;
	}

	//start conversion
	ADCSRA |= _BV(ADSC);
	//wait for conversion to complete
	loop_until_bit_is_clear(ADCSRA, ADSC);

	//lower 8 bits of result must be read first
	const u08 temp = ADCL;
	//combine the high and low bits and return the result as a 16-bit number.
	return ((u16)ADCH << 8) | temp;
}

/*! Returns an 8-bit resolution reading of the knob (potentiometer).
    @return The 8-bit reading of the knob.
 */
u08 knob()
{
	//clear the three 4051N analog mux selection bits (7:5) to select the knob
	PORTA &= 0x1F;
	//Set left shifting and select input channel 0
	ADMUX = _BV(ADLAR);
	//wait for the voltage to stabilize
	_delay_loop_1(100);
	//start conversion
	ADCSRA |= _BV(ADSC);
	//wait for conversion to complete
	loop_until_bit_is_clear(ADCSRA, ADSC);

	return ADCH;
}


/*! Returns a 10-bit resolution reading of the knob (potentiometer).
    @return The 10-bit reading of the knob.
 */
u16 knob10()
{
	//clear the three 4051N analog mux selection bits (7:5)
	PORTA &= 0x1F;
	//Set right shifting and select input channel 0
	ADMUX = 0;
	//wait for the voltage to stabilize
	_delay_loop_1(100);
	//start conversion
	ADCSRA |= _BV(ADSC);
	//wait for conversion to complete
	loop_until_bit_is_clear(ADCSRA, ADSC);

	//lower 8 bits of result must be read first
	const u08 temp = ADCL;
	//combine the high and low bits and return the result as a 16-bit number.
	return ((u16)ADCH << 8) | temp;
}
