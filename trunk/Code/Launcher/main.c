#include "ADC.h"
#include "compLeft.h"
#include "compRight.h"
#include "debug.h"
#include "launcherPackets.h"
#include "LCD.h"
#include "main.h"
#include "motors.h"
#include "packetprotocol.h"
#include "remoteControl.h"
#include "rtc.h"
#include "serial.h"
#include "servos.h"
#include "testmode.h"
#include "util.h"
#include "utility.h"
#include <avr/pgmspace.h>

//Local variables
volatile u16 innerEncoderTicks = 0;
volatile u16 totalInnerEncoderTicks = 0;
volatile u16 wallEncoderTicks = 0;
volatile u16 totalWallEncoderTicks = 0;
volatile u08 innerEncoderState;
volatile u08 wallEncoderState;
volatile u16 innerEncoderReading;
volatile u16 wallEncoderReading;
volatile u16 batteryReading;
volatile s16 error;
volatile s16 totalError;
volatile bool pause = FALSE;

//Local prototypes
static void mainMenu();

//! Initializes XiphosLibrary, pullups, and timers, sends bootup packet, prints version.
int main()
{
	//query what kind of reset caused this bootup
	u08 resetCause = MCUSR;
	//reset the MCU Status Register
	MCUSR = 0;

	//Initialize XiphosLibrary
	initialize();

	rtcInit();

	//Enable ADC interrupt
	ADCSRA |= _BV(ADIE);

	//set ADC right shifting (for 10-bit ADC reading), and select first ADC pin to read
	ADMUX = _BV(REFS0) | ANALOG_WHEEL_ENCODER_INNER;

	//enable interrupts
	sei();

	//Initialize UART and packet system then send boot packet
	uart0Init();
	initPacketDriver();
	sendBootNotification(resetCause);

	//configure all digital pins as inputs
	digitalDirections(0);
	//enable pullup resistors for all 10 digital inputs
	digitalPullups(0x3FF);
	//enable pullup resistors for all 8 analog inputs
	analogPullups(0xFF);

	//print firmware version and wait for button press
	printString_P(PSTR("ballReaper v" LAUNCHER_FIRMWARE_VERSION));
	delayMs(1500);

	//Start taking ADC readings
	ADCSRA |= _BV(ADSC);

	//Make sure launcher is off
	launcherSpeed(128);

	mainMenu();
}

//! Main Menu options.
enum {
	Option_RunCompetition,
	Option_TestMode,
	Option_RunRemoteSystem,
	NUM_Options
};

/*! Displays the main menu and runs the option that the user selects with the
 *  scroll switches.
 */
static void mainMenu()
{
	u08 choice = 0;
	u08 prevChoice = 255;
	void (*pProgInit)(void) = 0;
	void (*pProgExec)(void) = 0;

	clearScreen();
	printString_P(PSTR("Main Menu"));

	//loop until user makes a selection
	do
	{
		if (digitalInput(SWITCH_SCROLL) == 0)
		{
			if (++choice >= NUM_Options)
			{
				choice = 0;
			}
			//wait for switch to be released
			while (digitalInput(SWITCH_SCROLL) == 0)
				;
		}

		//redraw menu only when choice changes
		if (choice != prevChoice)
		{
			prevChoice = choice;

			lowerLine();
			switch (choice)
			{
				case Option_RunCompetition:
					printString_P(PSTR("1 RunCompetition"));
					break;
				case Option_TestMode:
					printString_P(PSTR("2 Test Mode     "));
					break;
				case Option_RunRemoteSystem:
					printString_P(PSTR("3 Remote System "));
					break;
				default:
					printString_P(PSTR("invalid choice"));
					SOFTWARE_FAULT(PSTR("invalid choice"), choice, 0);
					break;
			}
		}
	} while (getButton1() == 0);
	//debounce button
	buttonWait();

	clearScreen();

	//run the chosen mode
	switch (choice)
	{
		case Option_RunCompetition:
			if (LEFT_ROBOT_ID)
			{
				pProgInit = compLeftInit;
				pProgExec = compLeftExec;
			}
			else
			{
				pProgInit = compRightInit;
				pProgExec = compRightExec;
			}
			break;
		case Option_TestMode:
			pProgInit = testModeInit;
			pProgExec = testModeExec;
			break;
		case Option_RunRemoteSystem:
			pProgInit = remoteSystemInit;
			pProgExec = remoteSystemExec;
			break;
		default:
			SOFTWARE_FAULT(PSTR("invalid choice"), choice, 0);
			break;
	}

	pProgInit();

	while (1)
	{
		pProgExec();

		pidExec();

		delayMs(20);
	}
}

void pauseCompetition()
{
	rtcPause();
	pause = TRUE;
}

void resumeCompetition()
{
	rtcResume();
	pause = FALSE;
}

ISR(ADC_vect)
{
	// lower 8 bits of result must be read first
	const u08 lowByte = ADCL;
	// combine the high and low byte to get a 16-bit result.
	u16 reading = ((u16)ADCH << 8) | lowByte;

	bool encoderUpdated = FALSE;

	// determine which input was read
	switch (ADMUX & 0x3F)
	{
		case ANALOG_WHEEL_ENCODER_INNER:
			innerEncoderReading = reading;
			encoderUpdated = TRUE;
			// set ADC right shifting (for 10-bit ADC reading), and select next ADC pin to read
			ADMUX = _BV(REFS0) | ANALOG_WHEEL_ENCODER_WALL;
			break;
		case ANALOG_WHEEL_ENCODER_WALL:
			wallEncoderReading = reading;
			encoderUpdated = TRUE;
			// set ADC right shifting (for 10-bit ADC reading), and select next ADC pin to read
			ADMUX = _BV(REFS0) | ANALOG_BATTERY_VOLTAGE;
			break;
		case ANALOG_BATTERY_VOLTAGE:
			batteryReading = reading;
			// set ADC right shifting (for 10-bit ADC reading), and select next ADC pin to read
			ADMUX = _BV(REFS0) | ANALOG_WHEEL_ENCODER_INNER;
			break;
		default:
			// shouldn't ever reach this point. Just reset to a valid analog input.
			// set ADC right shifting (for 10-bit ADC reading), and select next ADC pin to read
			ADMUX = _BV(REFS0) | ANALOG_WHEEL_ENCODER_INNER;
			break;
	}

	// Start the next reading
	ADCSRA |= _BV(ADSC);

	// for encoder readings, run the tick counting logic
	if (encoderUpdated)
	{
		if (innerEncoderReading >= ENCODER_THRESHOLD_INNER_HIGH && innerEncoderState != 1)
		{
			innerEncoderState = 1;
			innerEncoderTicks++;
			totalInnerEncoderTicks++;
		}
		else if (innerEncoderReading <= ENCODER_THRESHOLD_INNER_LOW && innerEncoderState != 0)
		{
			innerEncoderState = 0;
			innerEncoderTicks++;
			totalInnerEncoderTicks++;
		}

		if (wallEncoderReading >= ENCODER_THRESHOLD_WALL_HIGH && wallEncoderState != 1)
		{
			wallEncoderState = 1;
			wallEncoderTicks++;
			totalWallEncoderTicks++;
		}
		else if (wallEncoderReading <= ENCODER_THRESHOLD_WALL_LOW && wallEncoderState != 0)
		{
			wallEncoderState = 0;
			wallEncoderTicks++;
			totalWallEncoderTicks++;
		}

		error = (totalInnerEncoderTicks - totalWallEncoderTicks);
		totalError += error;
	}
}
