//Copyright (C) 2009-2011  Patrick J. McCarty.
//Licensed under X11 License. See LICENSE.txt for details.

#ifndef UTILITY_H
#define UTILITY_H

#include "globals.h"

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
void relayOn();
void relayOff();
u08 digitalInput(u08 num);
u08 digitalInputs();
void softReset();

#endif
