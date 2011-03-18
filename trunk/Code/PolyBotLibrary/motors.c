//Copyright (C) 2009-2011  Patrick J. McCarty.
//Licensed under X11 License. See LICENSE.txt for details.

/*! @file
    Implements support for controlling four brushed DC motors using two quad half H-bridge chips.
 */
#include "globals.h"
#include "motors.h"
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

//Validate the number of enabled motors
#if NUM_MOTORS < 0 || NUM_MOTORS > 4
	#error "NUM_MOTORS must be set to a value of 0 to 4 in the project's Makefile"
#endif

/*! Initialize the enabled motor channels.
    Normally called only by the initialize() function in utility.c.
 */
void motorInit()
{
	//Initialize all motors to off.
	for (u08 i = 0; i < NUM_MOTORS; i++)
	{
		motor(i, 0);
	}

	//configure 74HC374AN (D Flip-Flop) clock pin used for motors as an output
	sbi(DDRD, DDD5);

	//enable timer (set prescaler to /8)
	TCCR1B |= _BV(CS11);

	//enable output compare OC1B interrupt
#if defined (__AVR_ATmega32__)
	TIMSK |= _BV(OCIE1B);
#elif defined (__AVR_ATmega644__) || defined(__AVR_ATmega644P__)
	TIMSK1 |= _BV(OCIE1B);
#endif

	//enable interrupts
	sei();
}

//! The frequency in Hertz to run the motor PWM at.
#define PWM_FREQUENCY 500
//! The number of timer ticks in one PWM period, using a timer prescaler of 8.
#define PWM_PERIOD_CYCLES (F_CPU / (PWM_FREQUENCY * 8))
//! The number of different motor power levels. This prevent motor interrupts from firing too close together.
#define MOTOR_POWER_STEPS 20

//! Stores the current motor power settings: 0 to 100.
u08 motorPower[NUM_MOTORS];
//! The lower 4 bits tell which motor enable pins are currently high.
volatile u08 motor_high = 0;

u16 motor_next[NUM_MOTORS];
volatile u08 num_motor_high;
volatile u16 motor_begin_period;
volatile u16 on_time;

//! Stores the state of the D flip-flop that drives the H-bridge chips.
volatile u08 motorOutput = 0;
// how the bits in motorOutput are used:
// 7 6 5 4 3 2 1 0
// 7=motor 3 direction
// 6=motor 3 enable
// 5=motor 2 direction
// 4=motor 2 enable
// 3=motor 1 direction
// 2=motor 1 enable
// 1=motor 0 direction
// 0=motor 0 enable

/*! Sets the power and direction of a motor using an 8-bit signed integer.
    @param num Selects the motor (0 to NUM_MOTORS-1).
    @param powerAndDirection Sets the motor power and direction (-100 to 100).
 */
// The motor number should be between 0 and 3. 0 is the leftmost motor.
// The direction is a number between -100 and 100
void motor(u08 num, s08 powerAndDirection)
{
	//validate the num parameter
	if (num >= NUM_MOTORS)
	{
		return;
	}

	//limit the range of the powerAndDirection parameter
	if (powerAndDirection > 100)
	{
		powerAndDirection = 100;
	}
	else if (powerAndDirection < -100)
	{
		powerAndDirection = -100;
	}

	motorPower[num] = ((powerAndDirection < 0) ? -powerAndDirection : powerAndDirection) / (100 / MOTOR_POWER_STEPS);
	motor_next[num] = motorPower[num] * (PWM_PERIOD_CYCLES / MOTOR_POWER_STEPS);

	if (powerAndDirection >= 0)
	{
		//set the motor's direction bit to go forward
		sbi(motorOutput, num * 2 + 1);
	}
	else
	{
		//clear the motor's direction bit to reverse
		cbi(motorOutput, num * 2 + 1);
	}
	num_motor_high = 0;
}

ISR(TIMER1_COMPB_vect)
{
	u08 i;
	u16 temp_time = 0xFFFF;
	u16 temp_diff;
	u08 num_low = 0;

	if (num_motor_high == 0)
	{
		//If all of the PWM periods for the motors are over, then go through
		//each of the active motors and set the enabled outputs to be high.
		for (i = 0; i < NUM_MOTORS; i++)
		{
			if (motorPower[i] == 0)
			{
				//If the motor power is 0, then set the output pin to low all of the time.
				cbi(motorOutput, 2 * i);
				num_low++;
			}
			else if (motorPower[i] != MOTOR_POWER_STEPS)
			{
				//if the motor is set to less than full power, set its output pin to high,
				//flag it as high, increment high counter, and find its next interrupt time
				sbi(motorOutput, 2 * i);
				sbi(motor_high, i);
				num_motor_high++;

				//find next time to get interrupt
				if (motor_next[i] < temp_time)
				{
					temp_time = motor_next[i];
				}
			}
			else
			{
				//If the motor power is 100, then set the output pin to high all of the time.
				sbi(motorOutput, 2 * i);
			}
		}

		if (num_low == NUM_MOTORS)
		{
			temp_time = PWM_PERIOD_CYCLES / 2;
		}

		on_time = temp_time;
		motor_begin_period = OCR1B;
		OCR1B += temp_time;
	}
	else
	{
		//Whenever an interrupt occurs and it is not the start of a new
		//PWM period, then check each motor output pin to see if it should
		//be set low. For each motor output pin that is set low, the variable
		//num_motor_high is decremented.

		for (i = 0; i < NUM_MOTORS; i++)
		{
			//Whenever a motor output is high, check if the current time
			//is beyond the high time for the motor. If so, turn off the
			//motor.
			if (bit_is_set(motor_high, i) && (motor_next[i] <= on_time))
			{
				cbi(motor_high,i);
				cbi(motorOutput, 2 * i); //set this motor output to low
				num_motor_high--;
			}

			if (bit_is_set(motor_high, i) && (motor_next[i] < temp_time))
			{
				temp_time = motor_next[i];
			}
		}

		if (num_motor_high == 0)
		{
			OCR1B = motor_begin_period + PWM_PERIOD_CYCLES;
		}
		else
		{
			temp_diff = temp_time - on_time;
			on_time = temp_time;
			OCR1B += temp_diff;
		}
	}

	//put the new motor control signals on the data bus
	writeBus(motorOutput);
	//drive the octal D flip-flop's clock pin high to change it's values
	sbi(PORTD, PD5);
	//brief delay to ensure the values fully propagate to the D flip-flop
	_delay_loop_1(1);
	//drive the octal D flip-flop's clock pin low to latch the new values
	cbi(PORTD, PD5);
}
