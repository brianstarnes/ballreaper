#include "rtc.h"
#include <util/atomic.h>

//There is an external 32.768kHz watch crystal attached to the Xiphos board,
//which can be used as the clock source for timer2 to allow keeping of real time.
//This means 32768 ticks of the crystal oscillator happen per second.
//When using a prescaler of 128, 256 post-prescaler ticks happen per second. (32768/128 = 256)

//! The number of seconds that have elapsed since the RTC was started.
volatile u08 secCount;
volatile u16 tickCount;

//! Initializes the RTC timer/counter, but does not start it.
void rtcInit()
{
	//Normal port operation, normal WGM.
	TCCR5A = 0;

	//16000000 / 8 prescaler / 128 ticks/second desired = 15625 compare value
	OCR5A = 15625;

	//Enable interrupt for timer5 output compare unit A
	TIMSK5 = _BV(OCIE5A);
}

//! Resets the timer to zero seconds and starts running it again.
void rtcRestart()
{
	//Reset fractional seconds to 0.
	TCNT5 = 0;

	//Reset secCount and tickCount
	secCount = 0;
	tickCount = 0;

	//Apply clock source to timer5.
	rtcResume();
}

/*! Freezes running the Real Time Clock.
 *  Note: this technique is not sub-second accurate, because the prescaler continues to run.
 */
void rtcPause()
{
	//Stop timer5 by removing its clock source.
	TCCR5B = 0;
}

/*! Resumes running the Real Time Clock.
 *  Note: this technique is not sub-second accurate, because the prescaler continues to run.
 */
void rtcResume()
{
	//Set timer5 prescaler to /8
	TCCR5B = _BV(CS51);
}

//! Fires when timer5 matches output compare value, which means that 1/128th of a second has elapsed.
ISR(TIMER5_COMPA_vect)
{
	//Update the output compare value
	OCR5A += 15625;

	//Increment the elapsed 1/128th seconds count
	tickCount++;
	//update secCount
	secCount = tickCount >> 7;
}

u32 getMsCount()
{
	u32 temp;
	ATOMIC_BLOCK(ATOMIC_FORCEON)
	{
		temp = tickCount;
	}
	return (u32)(temp * 1000.0 / 128);
}
