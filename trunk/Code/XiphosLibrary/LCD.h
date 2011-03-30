//Copyright (C) 2009-2011  Patrick J. McCarty.
//Licensed under X11 License. See LICENSE.txt for details.

/*! @file
    Defines LCD function prototypes as well as codes for some special LCD characters
	that can be printed to the LCD using printChar(). This is only a selection of some
	potentially useful characters and not a complete listing. Some LCD models may
	print different characters than those defined here.
 */

#ifndef LCD_H
#define LCD_H

#include "globals.h"
#include <avr/pgmspace.h>

//Special characters
#define RIGHT_ARROW  0x7E
#define LEFT_ARROW   0x7F
#define START_FRAME  0xA2
#define END_FRAME    0xA3
#define SEPARATOR    0xA5
#define EMPTY_BOX    0xDB
#define ALPHA        0xE0
#define BETA         0xE2
#define EPSILON      0xE3
#define MU           0xE4
#define SMALL_SIGMA  0xE5
#define RHO          0xE6
#define SQRT_SYMBOL  0xE8
#define CENT_SYMBOL  0xEC
#define THETA        0xF2
#define INFINITY     0xF3
#define OMEGA        0xF4
#define BIG_SIGMA    0xF6
#define PI_SYMBOL    0xF7
#define X_BAR        0xF8
#define DIVISION     0xFD
#define BLACK_SQUARE 0xFF

//Prototypes
void clearScreen();
void lcdOn();
void lcdOff();
void lcdInit();
void printChar(const u08 data);
void printCharN(const u08 data, const u08 times);
void printString(const char *const string);
void printStringN(const char *const string, const u08 maxChars);
void printString_P(PGM_P const string);
void print_u08(const u08 number);
void printPlain_u08(const u08 number);
void print_s08(const s08 number);
void printPlain_s08(const s08 number);
void print_u16(const u16 number);
void printPlain_u16(const u16 number);
void print_s16(const s16 number);
void printPlain_s16(const s16 number);
void printHexDigit(u08 nibble);
void printHex_u08(const u08 number);
void printHex_u16(const u16 number);
void printFloat(const float number);
void upperLine();
void lowerLine();
void lcdCursor(const u08 row, const u08 column);

#endif
