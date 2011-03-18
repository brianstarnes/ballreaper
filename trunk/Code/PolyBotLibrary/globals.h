//Copyright (C) 2009-2011  Patrick J. McCarty.
//Licensed under X11 License. See LICENSE.txt for details.

/*! @file
    Global defines.
 */

#ifndef GLOBALS_H
#define GLOBALS_H

#include <avr/io.h>          //I/O definitions (port/pin names, register names, named register bits, etc.)
#include <avr/interrupt.h>   //interrupt support
#include <stdint.h>

//! CPU speed (16MHz) for timer and delay loop calculations.
#if defined (__AVR_ATmega32__)
	// Define CPU speed (16MHz) for delay loop computations
	#define F_CPU 16000000UL
#elif defined (__AVR_ATmega644__) || defined(__AVR_ATmega644P__)
	// Define CPU speed (20MHz) for delay loop computations
	#define F_CPU 20000000UL
#endif

//! Define a Boolean type and boolean values.
typedef enum
{
	FALSE = 0,
	TRUE = 1
} bool;

//Define datatypes aliases as easier shorthand
typedef uint8_t  u08; //!< Unsigned 8-bit integer, range: 0 to +255
typedef int8_t   s08; //!< Signed 8-bit integer, range: -128 to +127
typedef uint16_t u16; //!< Unsigned 16-bit integer, range: 0 to +65,535
typedef int16_t  s16; //!< Signed 16-bit integer, range: -32,768 to +32,767
typedef uint32_t u32; //!< Unsigned 32-bit integer, range: 0 to +4,294,967,295
typedef int32_t  s32; //!< Signed 32-bit integer, range: -2,147,483,648 to +2,147,483,647

//Bit manipulation macros
#define sbi(a, b) ((a) |= 1 << (b))       //!< Sets bit b in variable a.
#define cbi(a, b) ((a) &= ~(1 << (b)))    //!< Clears bit b in variable a.
#define tbi(a, b) ((a) ^= 1 << (b))       //!< Toggles bit b in variable a.
#define gbi(a, b) ((a) & (1 << (b)))      //!< Gets bit b in variable a (masks out everything else).
#define gbis(a, b) (gbi((a), (b)) >> (b)) //!< Gets bit b in variable a and shifts it to the LSB.


//Data bus macros
//PolyBot 1.0 just uses PORTC
//PolyBot 1.1 uses pins: PC7|PC6|PC5|PC4|PC3|PC2|PB3|PB2

//write data to the 8-wide bus
#if defined (__AVR_ATmega32__)
	#define writeBus(X) PORTC = X;
#elif defined (__AVR_ATmega644__) || defined(__AVR_ATmega644P__)
	#define writeBus(X) do { \
		PORTC = ((X) & 0xFC) | (PINC & 0x03); \
		PORTB = (((X) << 2) & 0x0C) | (PINB & 0xF3); \
	} while (0)
#endif

//set all 8 bus pins as outputs
#if defined (__AVR_ATmega32__)
	#define makeBusOutput() DDRC = 0xFF;
#elif defined (__AVR_ATmega644__) || defined(__AVR_ATmega644P__)
	#define makeBusOutput() do { \
		DDRC |= 0xFC; \
		DDRB |= 0x0C; \
	} while (0)
#endif

//set all 8 bus pins as inputs
#if defined (__AVR_ATmega32__)
	#define makeBusInput() DDRC = 0x00;
#elif defined (__AVR_ATmega644__) || defined(__AVR_ATmega644P__)
	#define makeBusInput() do { \
		DDRC &= 0x03; \
		DDRB &= 0xF3; \
	} while(0)
#endif

#endif //ifndef GLOBALS_H
