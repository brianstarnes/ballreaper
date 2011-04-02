#include "rtc.h"

//There is an external 32.768kHz watch crystal attached to the Xiphos board,
//which can be used as the clock source for timer2 to allow keeping of real time.
//This means 32768 ticks of the crystal oscillator happen per second.
//When using a prescaler of 128, 256 post-prescaler ticks happen per second. (32768/128 = 256)

//! The number of seconds that have elapsed since the RTC was started.
volatile u08 secCount;

//! Initializes the RTC timer/counter, but does not start it.
void rtcInit()
{
	//Make the 32.768kHz crystal oscillator the clock source for timer2's prescaler.
	//We do this instruction first because changing AS2 can corrupt the contents of
	//registers TCNT2, OCR2A, OCR2B, TCCR2A, and TCCR2B.
	ASSR = _BV(AS2);

	//Enable timer2 overflow interrupt.
	TIMSK2 = _BV(TOIE2);
}

//! Resets the Real Time Clock to zero seconds and starts running it again.
void rtcRestart()
{
	//Reset fractional seconds to 0.
	TCNT2 = 0;

	//Reset secCount.
	secCount = 0;

	//Apply clock source to timer2.
	rtcResume();

	//Wait for TCNT2 to finish updating, to prevent user from writing again too soon
	//and corrupting the register.
	loop_until_bit_is_clear(ASSR, TCN2UB);
}

/*! Freezes running the Real Time Clock.
 *  Note: this technique is not sub-second accurate, because the prescaler continues to run.
 */
void rtcPause()
{
	//Stop timer2 by removing its clock source.
	TCCR2B = 0;

	//Wait for TCCR2B to finish updating, to prevent user from writing again too soon
	//and corrupting the register.
	loop_until_bit_is_clear(ASSR, TCR2BUB);
}

/*! Resumes running the Real Time Clock.
 *  Note: this technique is not sub-second accurate, because the prescaler continues to run.
 */
void rtcResume()
{
	//Set timer2 prescaler to 128, so the 8-bit timer2 will overflow exactly once per second.
	TCCR2B = _BV(CS22) | _BV(CS20);

	//Wait for TCCR2B to finish updating, to prevent user from writing again too soon
	//and corrupting the register.
	loop_until_bit_is_clear(ASSR, TCR2BUB);
}

//! Fires when timer2 overflows, which means that one second has elapsed.
ISR(TIMER2_OVF_vect)
{
	//Increment the elapsed seconds.
	secCount++;
}
