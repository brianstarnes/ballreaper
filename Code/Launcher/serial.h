#ifndef SERIAL_H
#define SERIAL_H

#include "globals.h"

void uart0Init();

void uart0Transmit(u08 data);

void uart1Transmit(u08 data);

u08 uart0Receive();

void usart0Flush();

#if defined (UBRR1H)
	void uart1Init();
#endif

#endif
