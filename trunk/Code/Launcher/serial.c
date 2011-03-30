#include "serial.h"
#include <inttypes.h>
#include <string.h>

//BAUDn sets the baud rate.
//Valid options are: 2400, 4800, 9600, 14400, 19200, 28800, 38400, 57600, 76800, 115200, 230400, 250000

//BAUD_TOLn defines the acceptable baud rate error tolerance in percent. Default is 2%.
//setbaud.h will #define USE_2X as 1 if the desired baud rate tolerance could only be achieved by setting the U2X bit in the UART configuration.

//settings for UART0
#define BAUD0     57600
#define BAUD_TOL0 2

//settings for UART1
#define BAUD1     57600
#define BAUD_TOL1 2

/*
From ATmega documentation:
The USART has to be initialized before any communication can take place. The initialization process
normally consists of setting the baud rate, the frame format, and enabling the
Transmitter and/or the Receiver depending on the usage. For interrupt driven USART operation, the
Global Interrupt Flag should be cleared (and interrupts globally disabled) when doing the
initialization.
*/
void uart0Init()
{
	// Disable interrupts globally, per ATmega documentation recommendation
	cli();
	// Copy the values defined above to the names used by util/setbaud.h
	#define BAUD BAUD0
	#define BAUD_TOL BAUD_TOL0

	// Converts the user-friendly baud rate defined above to prescaler bits to set in UBRR
	#include <util/setbaud.h>
	// Copy the values defined by setbaud.h to the appropriate UART config registers
	#if defined (UBRRH)
		UBRRH = UBRRH_VALUE;
		UBRRL = UBRRL_VALUE;
		#if USE_2X
		UCSRA |= (1<<U2X);
		#else
		UCSRA &= ~(1<<U2X);
		#endif
	#elif defined (UBRR0H)
		UBRR0H = UBRRH_VALUE;
		UBRR0L = UBRRL_VALUE;
		#if USE_2X
		UCSR0A |= (1<<U2X0);
		#else
		UCSR0A &= ~(1<<U2X0);
		#endif
	#else
		#error Failed to detect which serial registers your chip uses.
	#endif

	// Set USART Control and Status Register B to configure enabled functionality
	// Enable receiver and transmitter, and Receive Complete Interrupt
	#if defined (UCSRB)
		UCSRB = (1<<RXEN)|(1<<TXEN)|(1<<RXCIE);
	#elif defined (UCSR0B)
		UCSR0B = (1<<RXEN0)|(1<<TXEN0)|(1<<RXCIE0);
	#else
		#error Failed to detect which serial registers your chip uses.
	#endif
	// Enable Receiver=(1<<RXEN)
	// Enable Transmitter=(1<<TXEN)
	// Enable RX Complete Interrupt=(1<<RXCIE), fires USART_RXC_vect/USART0_RX_vect ISR when RXC bit in UCSRA is set
	// Enable TX Complete Interrupt=(1<<TXCIE), fires USART_TXC_vect/USART0_TX_vect ISR when TXC bit in UCSRA is set
	// Enable USART Data Register Empty Interrupt=(1<<UDRIE), fires USART_UDRE_vect/USART0_UDRE_vect ISR when UDRE bit in UCSRA is set

	// Set USART Control and Status Register C to configure frame format, etc
	// asynchronous, 8 data bits, no parity, 1 stop bit, UCPOL must be 0 in async mode
	#if defined (UCSRC)
		UCSRC = (1<<URSEL)|(0<<UMSEL)|(3<<UCSZ0)|(0<<UPM0)|(0<<USBS)|(0<<UCPOL);
	#elif defined (UCSR0C)
		UCSR0C = (0<<UMSEL00)|(3<<UCSZ00)|(0<<UPM00)|(0<<USBS0)|(0<<UCPOL0);
	#else
		#error Failed to detect which serial registers your chip uses.
	#endif
	//(1<<URSEL) is a register selecter only needed for atmega32. URSEL must be one when writing to UCSRC.
	// asynchronous=(0<<UMSEL00), synchronous=(1<<UMSEL00)
	// 5 databits=(0<<UCSZ00), 6 databits=(1<<UCSZ00), 7 databits=(2<<UCSZ00), 8 databits=(3<<UCSZ00), a 9 databit mode is also supported but uses additional registers
	// no parity=(0<<UPM00), even parity=(2<<UPM00), odd parity=(3<<UPM00) - parity mismatches set UPE bit in UCSRA
	// 1 stop bit=(0<<USBS0), 2 stop bits=(1<<USBS0) - this bit is only used for transmitter, ignored by receiver
	// clock polarity (for synchronous mode only, set to 0 for async mode):
	//     (0<<UCPOL) = transmit on rising XCK (synchronous clock) edge and receive on falling XCK edge
	//     (1<<UCPOL) = transmit on falling XCK (synchronous clock) edge and receive on rising XCK edge

	// Enable interrupts so that the USART_RXC_vect/USART0_RX_vect ISR will be called when data is received
	sei();
}

//Not all AVR chips have a second UART
#if defined (UBRR1H)
void uart1Init()
{
	// Disable interrupts globally, per ATmega documentation recommendation
	cli();
	// undefine the previous names to avoid compiler warnings
	#undef BAUD
	#undef BAUD_TOL
	// Copy the values defined above to the names used by util/setbaud.h
	#define BAUD BAUD1
	#define BAUD_TOL BAUD_TOL1

	// Converts the user-friendly baud rate defined above to prescaler bits to set in UBRR
	#include <util/setbaud.h>
	// Copy the values defined by setbaud.h to the appropriate UART config registers
	UBRR1H = UBRRH_VALUE;
	UBRR1L = UBRRL_VALUE;
	#if USE_2X
	UCSR1A |= (1<<U2X1);
	#else
	UCSR1A &= ~(1<<U2X1);
	#endif

	// Set USART Control and Status Register B to configure enabled functionality
	// Enable receiver and transmitter, and Receive Complete Interrupt
	UCSR1B = (1<<RXEN1)|(1<<TXEN1)|(1<<RXCIE1);
	// Enable Receiver=(1<<RXEN)
	// Enable Transmitter=(1<<TXEN)
	// Enable RX Complete Interrupt=(1<<RXCIE), fires USART_RXC_vect/USART0_RX_vect ISR when RXC bit in UCSRA is set
	// Enable TX Complete Interrupt=(1<<TXCIE), fires USART_TXC_vect/USART0_TX_vect ISR when TXC bit in UCSRA is set
	// Enable USART Data Register Empty Interrupt=(1<<UDRIE), fires USART_UDRE_vect/USART0_UDRE_vect ISR when UDRE bit in UCSRA is set

	// Set USART Control and Status Register C to configure frame format, etc
	// asynchronous, 8 data bits, no parity, 1 stop bit, UCPOL must be 0 in async mode
	UCSR1C = (0<<UMSEL10)|(3<<UCSZ10)|(0<<UPM10)|(0<<USBS1)|(0<<UCPOL1);
	// asynchronous=(0<<UMSEL10), synchronous=(1<<UMSEL10)
	// 5 databits=(0<<UCSZ00), 6 databits=(1<<UCSZ00), 7 databits=(2<<UCSZ00), 8 databits=(3<<UCSZ00), a 9 databit mode is also supported but uses additional registers
	// no parity=(0<<UPM00), even parity=(2<<UPM00), odd parity=(3<<UPM00) - parity mismatches set UPE bit in UCSRA
	// 1 stop bit=(0<<USBS0), 2 stop bits=(1<<USBS0) - this bit is only used for transmitter, ignored by receiver
	// clock polarity (for synchronous mode only, set to 0 for async mode):
	//     (0<<UCPOL) = transmit on rising XCK (synchronous clock) edge and receive on falling XCK edge
	//     (1<<UCPOL) = transmit on falling XCK (synchronous clock) edge and receive on rising XCK edge

	// Enable interrupts so that the USART_RXC_vect/USART0_RX_vect ISR will be called when data is received
	sei();
}
#endif

// Simple UART transmit function from ATmega datasheet
// More sophisticated way is to use the TX Complete interrupt - USART_TXC_vect/USART0_TX_vect
void uart0Transmit(u08 data)
{
	#if defined (UCSRA)
		// Wait for empty transmit buffer
		while(!(UCSRA & (1<<UDRE)));
		// put data into buffer, which sends the data
		UDR = data;
	#elif defined (UCSR0A)
		// Wait for empty transmit buffer
		while(!(UCSR0A & (1<<UDRE0)));
		// put data into buffer, which sends the data
		UDR0 = data;
	#else
		#error Failed to detect which serial registers your chip uses.
	#endif
}

// simple UART receive function from ATmega datasheet
// More sophisticated way is to use the RX Complete interrupt - USART_RXC_vect/USART0_RX_vect
u08 uart0Receive()
{
	#if defined (UCSRA)
		// Wait for data to be received
		while(!(UCSRA & (1<<RXC)));
		// Get and return received data from buffer
		return UDR;
	#elif defined (UCSR0A)
		// Wait for data to be received
		while(!(UCSR0A & (1<<RXC0)));
		// Get and return received data from buffer
		return UDR0;
	#else
		#error Failed to detect which serial registers your chip uses.
	#endif
}

// Flushes the entire receive buffer
void usart0Flush()
{
	u08 dummy;
	//gobbles up all the received characters in the buffer until RXC (Receive complete) goes to 0
	#if defined (UCSRA)
		while (UCSRA & (1<<RXC))
			dummy = UDR;
	#elif defined (UCSR0A)
		while (UCSR0A & (1<<RXC0))
			dummy = UDR0;
	#else
		#error Failed to detect which serial registers your chip uses.
	#endif
}
