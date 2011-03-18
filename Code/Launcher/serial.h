#ifndef SERIAL_H
#define SERIAL_H

#include "globals.h"

void uart0Init();

void uart0Transmit(u08 data);

u08 uart0Receive();

void usart0Flush();

//Only the ATmega644P has a second UART
#if defined(__AVR_ATmega644P__)
	void uart1Init();
#endif

#endif
