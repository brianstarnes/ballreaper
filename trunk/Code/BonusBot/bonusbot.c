#include "ADC.h"
#include "bonusbot.h"
#include "competition.h"
#include "debug.h"
#include "launcherPackets.h"
#include "LCD.h"
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
volatile u16 qrdFrontLeftReading;
volatile u16 qrdFrontRightReading;
volatile u16 qrdBackRightReading;
volatile u16 qrdBackLeftReading;
volatile s16 error;
volatile s16 totalError;
volatile bool pause = FALSE;

//Local prototypes
static void mainMenu();

//! Initializes XiphosLibrary, pullups, and timers, sends bootup packet, prints version.
int main()
{
	//Initialize XiphosLibrary
	initialize();

	rtcInit();

	//Enable ADC interrupt
	ADCSRA |= _BV(ADIE);

	//set ADC right shifting (for 10-bit ADC reading), and select first ADC pin to read
	ADMUX = _BV(REFS0) | ANALOG_WHEEL_ENCODER_INNER;

	//enable interrupts
	sei();

	//Initialize UART
	uart0Init();

	//configure digital pins 2-9 as inputs
	DDRA = 0;
	//enable pullup resistors for digital pins 2-9
	digitalPullups(0x3FC);
	//enable pullup resistors for all 8 analog inputs
	analogPullups(0xFF);

	//print firmware version and wait for button press
	printString_P(PSTR("BonusBot v" LAUNCHER_FIRMWARE_VERSION));
	delayMs(1000);

	//Start taking ADC readings
	ADCSRA |= _BV(ADSC);

	//Make sure launcher is off
	launcherSpeed(LAUNCHER_SPEED_STOPPED);

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
			pProgInit = compInit;
			pProgExec = compExec;
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
	}
}

ISR(ADC_vect)
{
	// lower 8 bits of result must be read first
	const u08 lowByte = ADCL;
	// combine the high and low byte to get a 16-bit result.
	u16 reading = ((u16)ADCH << 8) | lowByte;

	// determine which input was read
	switch (ADMUX & 0x3F)
	{
		case ANALOG_FRONT_LEFT:
			qrdFrontLeftReading = reading;
			encoderUpdated = TRUE;
			// set ADC right shifting (for 10-bit ADC reading), and select next ADC pin to read
			ADMUX = _BV(REFS0) | ANALOG_FRONT_RIGHT;
			break;
		case ANALOG_FRONT_RIGHT:
			qrdFrontRightReading = reading;
			encoderUpdated = TRUE;
			// set ADC right shifting (for 10-bit ADC reading), and select next ADC pin to read
			ADMUX = _BV(REFS0) | ANALOG_BACK_RIGHT;
			break;
		case ANALOG_BACK_RIGHT:
			qrdBackRightReading = reading;
			// set ADC right shifting (for 10-bit ADC reading), and select next ADC pin to read
			ADMUX = _BV(REFS0) | ANALOG_BACK_LEFT;
			break;
		case ANALOG_BACK_LEFT:
			qrdBackLeftReading = reading;
			// set ADC right shifting (for 10-bit ADC reading), and select next ADC pin to read
			ADMUX = _BV(REFS0) | ANALOG_FRONT_LEFT;
			break;
		default:
			// shouldn't ever reach this point. Just reset to a valid analog input.
			// set ADC right shifting (for 10-bit ADC reading), and select next ADC pin to read
			ADMUX = _BV(REFS0) | ANALOG_FRONT_LEFT;
			break;
	}

	// Start the next reading
	ADCSRA |= _BV(ADSC);
}
