//Copyright (C) 2009-2011  Patrick J. McCarty.
//Licensed under X11 License. See LICENSE.txt for details.

/*! @file
    Various utility functions for the PolyBot Board.
 */

#include "ADC.h"
#include "globals.h"
#include "LCD.h"
#include "motors.h"
#include "servos.h"
#include "utility.h"
#include <util/atomic.h>
#include <util/delay.h>
#include <avr/wdt.h>

/*
Key of pin operations:
DDR:           0=input, 1=output
input PORT:    0=disable pullup, 1=enable pullup
output PORT:   0=drive low, 1=drive high
write to PIN:  1=toggle value of PORT
read from PIN: value on the pin
*/

//! Initializes all enabled board features. Must be called in your program before using other library functions.
inline void initialize()
{
	//configure switch SW1 as an input
	cbi(DDRD, DDD7);

	//configure LED as an output
	sbi(DDRA, DDA4);

	//configure the motor/servo/LCD/digitalInput data bus as an output (as expected by the servo/motor ISRs and LCD)
	makeBusOutput();

	#if USE_LCD == 1
		//initialize LCD
		lcdInit();
	#endif

	#if USE_I2C == 1
		//configure I2C clock rate
		i2cInit();
	#endif

	#if NUM_MOTORS > 0
		//initialize motors
		motorInit();
	#endif

	#if NUM_SERVOS > 0
		//initialize servos
		servoInit();
	#endif

	#if USE_ADC == 1
		//initialize ADC
		adcInit();
	#endif

	#if USE_RELAY == 1
		//configure relay pin as output
		#if defined (__AVR_ATmega32__)
			sbi(DDRB, DDB3);
		#elif defined (__AVR_ATmega644__) || defined(__AVR_ATmega644P__)
			sbi(DDRB, DDB4);
		#endif
	#endif
}

//! Provides a busy wait loop for an approximate number of milliseconds.
void delayMs(u16 num)
{
	u16 i;
	for (i = 0; i < num; i++)
	{
		#if (F_CPU == 16000000UL)
			_delay_loop_2(4000);
		#elif (F_CPU == 20000000UL)
			_delay_loop_2(5000);
		#else
			#error delayMs() is not implemented for your F_CPU speed.
		#endif
	}
}

//! Provides a busy wait loop for an approximate number of microseconds.
void delayUs(u16 num)
{
	u16 i;
	for (i = 0; i < num; i++)
	{
		#if (F_CPU == 16000000UL)
			_delay_loop_2(4);
		#elif (F_CPU == 20000000UL)
			_delay_loop_2(5);
		#else
			#error delayUs() is not implemented for your F_CPU speed.
		#endif
	}
}

/*! Reads the status of the SW1 button.
    @return 0 when the button is not pressed and 1 when the button is pressed.
    @see Use buttonWait() if you want to wait for a button press and release.
 */
u08 getButton1()
{
	return gbi(PIND, PD7) == 0;
}

/*! Waits for a complete SW1 button press and release, with button debouncing.
    @see Use getButton1() for simply checking the button state without waiting or debouncing.
 */
void buttonWait()
{
	//wait for button to be pushed down
	while (!getButton1());
	//delay 30 ms for button debouncing
	delayMs(30);
	//wait for button to be released, if it is still down
	while (getButton1());
	//delay 30 ms for button debouncing
	delayMs(30);
}

/*! Waits for a complete SW1 button press and release, with button debouncing, and
 *  indicates whether the button was held down for more than 300 ms or quickly tapped.
    @see Use buttonWait() if you don't care how long the button was pressed for.
    @see Use getButton1() for simply checking the button state without waiting or debouncing.
    @return TRUE if button was held at least 300ms, otherwise FALSE.
 */
bool buttonHeld()
{
	//wait for button to be pushed down
	while (!getButton1());
	//delay 30 ms for button debouncing
	delayMs(30);
	//count down 270ms waiting for button to be released, if it is still down
    u08 cycles = 135;
    while (getButton1() && cycles > 0)
    {
    	delayMs(2);
        cycles--;
    }
    //wait for button to be released, if it is still down
	while (getButton1());
	//delay 30 ms for button debouncing
	delayMs(30);
    return cycles == 0;
}

//! Turns the LED on
void ledOn()
{
	sbi(PORTA, PA4);
}

//! Turns the LED off
void ledOff()
{
	cbi(PORTA, PA4);
}

//! Toggles the LED on/off
void ledToggle()
{
	tbi(PORTA, PA4);
}

//! Turns the relay transistor on
void relayOn()
{
	#if defined (__AVR_ATmega32__)
		sbi(PORTB, PB3);
	#elif defined (__AVR_ATmega644__) || defined(__AVR_ATmega644P__)
		sbi(PORTB, PB4);
	#endif
}

//! Turns the relay transistor off
void relayOff()
{
	#if defined (__AVR_ATmega32__)
		cbi(PORTB, PB3);
	#elif defined (__AVR_ATmega644__) || defined(__AVR_ATmega644P__)
		cbi(PORTB, PB4);
	#endif
}

//Returns the value of a digital input.
//num argument can be 0 to 7.
u08 digitalInput(u08 num)
{
	#if defined (__AVR_ATmega32__)
		u08 value;

		//Disable interrupts in this block to prevent motor/servo ISRs from
		//messing with the data bus while we are trying to use it.
		//ATOMIC_RESTORESTATE is used so that this function can be called from
		//user code/ISRs without unexpected side effects.
		ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
		{
			//configure the data bus pins as inputs
			makeBusInput();
			//enable the 74HC244N octal buffer so the digital inputs flow onto the data bus
			cbi(PORTD, PD4);
			//allow some time for the signals to propagate to the microcontroller
			_delay_loop_1(1);
			//read the input values from the data bus
			value = PINC;
			//disable the 74HC244N octal buffer to tri-state the data bus
			sbi(PORTD, PD4);
			//restore the data bus pins to outputs as expected by the servo/motor ISRs and LCD
			makeBusOuput();
		}

		return ((value & _BV(num)) != 0);

	#elif defined (__AVR_ATmega644__) || defined(__AVR_ATmega644P__)
		u08 portC;
		u08 portB; //PB2 & PB3 are part of the shared bus

		//Disable interrupts in this block to prevent motor/servo ISRs from
		//messing with the data bus while we are trying to use it.
		//ATOMIC_RESTORESTATE is used so that this function can be called from
		//user code/ISRs without unexpected side effects.
		ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
		{
			//configure the data bus pins as inputs
			makeBusInput();
			//enable the 74HC244N octal buffer so the digital inputs flow onto the data bus
			cbi(PORTD, PD4);
			//allow some time for the signals to propagate to the microcontroller
			_delay_loop_1(1);
			//read the input values from the data bus
			portC = PINC;
			portB = PINB;
			//disable the 74HC244N octal buffer to tri-state the data bus
			sbi(PORTD, PD4);
			//restore the data bus pins to outputs as expected by the servo and motor ISRs
			makeBusOutput();
		}

		if (num < 2)
			return (((portB>>2) & _BV(num)) != 0);
		else
			return ((portC & _BV(num)) != 0);
	#endif
}

/*
//The u08 number returned from digitalInputs is the the result of the 8 inputs being treated as a binary number.
//This number will make more sense if displayed in hex by the printHex_u16 function.
u08 digitalInputs()
{
	//TODO
}*/

/*! Performs a software reset by setting the shortest watchdog timeout possible and infinite looping.
    This is quite similar to pushing the reset button. The chip will reset and run the bootloader again.
    Therefore if the USB cable is providing power, the bootloader will do a 10 second countdown.
 */
inline void softReset()
{
	wdt_enable(WDTO_15MS);
	while (1)
	{
	}
}

/*! Since the watchdog timer remains enabled after a resetting, this function is added
    to a section of code that runs very early during startup to disable the watchdog.
    It should not be called from user code.
 */
void wdt_init() __attribute__((naked)) __attribute__((section(".init3")));
void wdt_init()
{
	MCUSR = 0;
	wdt_disable();
	return;
}
