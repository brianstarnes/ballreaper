#include "launcherMotors.h"
//TODO: remove LCD.h
#include "LCD.h"

/*! @file Drives via a transistor an external Power MOSFET that serves as a
 *  single-direction motor speed controller.

*/

//! Initializes the MOSFET motor controller.
void mosfetInit()
{
	// Set up Timer0 in 8-bit fast PWM mode with inverted polarity.
	// We need to invert the output because the transistor is also inverting.
	TCCR0A = _BV(COM0A1) | _BV(COM0A0) | _BV(COM0B1) | _BV(COM0B0) | _BV(WGM01) | _BV(WGM00);
	//Set prescaler to I/O clock / 8. Resulting PWM frequency is F_CPU/8/256 = 7812.5 Hz.
	TCCR0B = _BV(CS01);

	// Turn off the motors.
	mosfet0Power(0);
	mosfet1Power(0);

	// Set the MOSFET0 PWM pin as an output.
	DDRB |= _BV(DDB7);

	// Set the MOSFET1 PWM pin as an output.
	DDRG |= _BV(DDG5);
}

//! Sets the duty cycle of the PWM signal on PB7 driving the transistor/MOSFET.
void mosfet0Power(u08 duty)
{
	OCR0A = duty;
	clearScreen();
	printString_P(PSTR("mosfet0Power "));
	print_u08(duty);
}

//! Sets the duty cycle of the PWM signal on PG5 driving the transistor/MOSFET.
void mosfet1Power(u08 duty)
{
	OCR0B = duty;
	clearScreen();
	printString_P(PSTR("mosfet1Power "));
	print_u08(duty);
}
