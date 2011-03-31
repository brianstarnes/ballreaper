//Copyright (C) 2009-2011  Patrick J. McCarty.
//Licensed under X11 License. See LICENSE.txt for details.

#ifndef UTILITY_H
#define UTILITY_H

#include "globals.h"

//! Specifies the valid values that can be passed to the digitalDirection() function.
typedef enum
{
	Direction_INPUT,
	Direction_OUTPUT,
	Direction_INPUT_PULLUP
} DigitalDirection;

//Prototypes
void initialize();
void delayMs(u16 num);
void delayUs(u16 num);
u08 getButton1();
void buttonWait();
bool buttonHeld();
void ledOn();
void ledOff();
void ledToggle();
void analogPullups(const u08 pullups);
void digitalDirection(u08 num, const DigitalDirection direction);
void digitalDirections(const u16 directions);
void digitalPullups(const u16 pullups);
u08 digitalInput(const u08 num);
void digitalOutput(const u08 num, const u08 value);
void digitalOutputs(const u16 outputs);
u16 digitalInputs();
void digitalOutputToggle(const u08 num);
void softReset();

#endif
