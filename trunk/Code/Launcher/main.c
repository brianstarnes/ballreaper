#include "ADC.h"
#include <util/atomic.h>
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
#include "utility.h"
#include <avr/pgmspace.h>

//Defines
//! The number of seconds per competition round.
#define COMPETITION_DURATION_SECS (3 * 60)

//Local variables
static volatile u16 innerEncoderTicks = 0;
static volatile u16 totalInnerEncoderTicks = 0;
static volatile u16 wallEncoderTicks = 0;
static volatile u16 totalWallEncoderTicks = 0;
static volatile u08 innerEncoderState;
static volatile u08 wallEncoderState;
static volatile u16 innerEncoderReading;
static volatile u16 wallEncoderReading;
static volatile u16 batteryReading;
static volatile s16 error;
static volatile s16 totalError;
static volatile bool pause = FALSE;


//Local prototypes
static void mainMenu();
static void runCompetition();
static void victoryDance();
static void testMode();

static void printVoltage(u16 milliVolts);

static void pidDrive(s08 wallSpeed, s08 innerSpeed);
static void turnLeft();
static void stop();

static void launcherSpeed(u08 speed);
static void scraperDown();
static void scraperUp();
static void feederOn();
static void feederOff();


//static void downLink();

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

	//Start taking ADC readings
	ADCSRA |= _BV(ADSC);

	//print firmware version and wait for button press
	printString_P(PSTR("ballReaper v" LAUNCHER_FIRMWARE_VERSION));
	buttonWait();

	while (1)
	{
		mainMenu();
	}
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
	u08 previousChoice = 255;

	clearScreen();
	printString_P(PSTR("Main Menu"));

	//configure scroll inputs
	//digitalDirection(SWITCH_SCROLL_UP, Direction_INPUT_PULLUP);
	//digitalDirection(SWITCH_SCROLL_DOWN, Direction_INPUT_PULLUP);

	//loop until user makes a selection
	do
	{
		if (digitalInput(SWITCH_SCROLL_UP) == 0)
		{
			choice--;
			if (choice >= NUM_Options)
			{
				choice = NUM_Options - 1;
			}
			//wait for switch to be released
			while (digitalInput(SWITCH_SCROLL_UP) == 0)
				;
		}
		else if (digitalInput(SWITCH_SCROLL_DOWN) == 0)
		{
			choice++;
			if (choice >= NUM_Options)
			{
				choice = 0;
			}
			//wait for switch to be released
			while (digitalInput(SWITCH_SCROLL_DOWN) == 0)
				;
		}

		//redraw menu only when choice changes
		if (choice != previousChoice)
		{
			previousChoice = choice;
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
		delayMs(5);
	} while (getButton1() == 0);
	//debounce button
	buttonWait();

	clearScreen();

	//run the chosen mode
	switch (choice)
	{
		case Option_RunCompetition:
			runCompetition();
			break;
		case Option_TestMode:
			testMode();
			break;
		case Option_RunRemoteSystem:
			runRemoteSystem();
			break;
		default:
			SOFTWARE_FAULT(PSTR("invalid choice"), choice, 0);
			break;
	}
}


void calibrate(int ticksPerSec, s08* wallMotorSpeed, s08* innerMotorSpeed)
{
	*wallMotorSpeed = 50;
	*innerMotorSpeed = 50;
	int cmdStep = 25;

	while (cmdStep > 0) {

		// command motor speeds
		wallMotor(*wallMotorSpeed);
		innerMotor(*innerMotorSpeed);

		// reset encoder ticks and wait for encoder ticks to accumulate
		totalInnerEncoderTicks = 0;
		totalWallEncoderTicks = 0;
		delayMs(1000);

		int innerTicksPerSec = totalInnerEncoderTicks;
		int wallTicksPerSec = totalInnerEncoderTicks;

		// adjust inner motor speed
		if (innerTicksPerSec > ticksPerSec)
			*innerMotorSpeed -= cmdStep;
		else if (innerTicksPerSec < ticksPerSec)
			*innerMotorSpeed += cmdStep;

		// adjust wall motor speed
		if (wallTicksPerSec > ticksPerSec)
			*wallMotorSpeed -= cmdStep;
		else if (wallTicksPerSec < ticksPerSec)
			*wallMotorSpeed += cmdStep;

		cmdStep = cmdStep/2;
	}
	stop();
}
/************************************************************************************************/
//! Runs the competition code.
static void runCompetition()
{
	u08 rearSideWallHit = 0;
	u08 frontSideWallHit = 0;
	u08 refills = 0;

	configPacketProcessor(&validateLauncherPacket, &execLauncherPacket, LAST_UplinkPacketType - 1);

	clearScreen();
	printString_P(PSTR("Press Button"));

	buttonWait();
	rtcRestart();

	ATOMIC_BLOCK(ATOMIC_FORCEON)
	{
	    innerEncoderTicks = 0;
		wallEncoderTicks = 0;
	}

	clearScreen();
	lowerLine();
	printString_P(PSTR("Drive forward"));

	//drive until either side wall switches hit
	while(!rearSideWallHit && !frontSideWallHit)
	{
		pidDrive(FAST_SPEED_WALL_WHEEL, FAST_SPEED_INNER_WHEEL);

		delayMs(1);
		rearSideWallHit =  REAR_SIDE_WALL_HIT;
		frontSideWallHit = FRONT_SIDE_WALL_HIT;
	}

	// Hit wall so turn left
	clearScreen();
	printString_P(PSTR("Turn Left"));

	innerEncoderTicks = 0;
    wallEncoderTicks = 0;

    turnLeft();

	// Turn left 90 degrees
	while(innerEncoderTicks < 26 || wallEncoderTicks < 26);

	innerEncoderTicks = 0;
    wallEncoderTicks = 0;

    pidDrive(TURN_SPEED_WALL_WHEEL, TURN_SPEED_INNER_WHEEL);
	delayMs(100);

	// make sure that the robot turned the full 90 degrees and isn't stuck on the wall with the wheels not moving
	while (BACK_RIGHT_HIT        ||
		   BACK_LEFT_HIT         ||
		   innerEncoderTicks < 5 ||
		   wallEncoderTicks < 5)
	{
    	clearScreen();
        printString_P(PSTR("Adjusting Left"));

		turnLeft();
		while(innerEncoderTicks < 7 && wallEncoderTicks < 7);
		pidDrive(TURN_SPEED_WALL_WHEEL, TURN_SPEED_INNER_WHEEL);
		delayMs(100);
	}

	stop();
	launcherSpeed(180);
	feederOn();

	// Start Ball Reaping! We only get 2 refills so make them count!
    while (refills < 2)
	{
    	static int i, j;

    	i = j = 0;
		stop();
		delayMs(500);
    	scraperDown();

    	// Start driving forward to pick balls up
    	clearScreen();
        printString_P(PSTR("Drive forward"));
		while(!FRONT_HIT)
		{
			if (!REAR_SIDE_WALL_HIT && !FRONT_SIDE_WALL_HIT)
			{
				//lost the wall
				j++;
				if (j > 5)
					j = 5;
				pidDrive(SLOW_SPEED_WALL_WHEEL, SLOW_SPEED_INNER_WHEEL + j);
			}
			else if (!REAR_SIDE_WALL_HIT)
			{
				i++;
				if (j > 0)
					j--;
				if (i > 5)
					i = 5;
				pidDrive(SLOW_SPEED_WALL_WHEEL + i, SLOW_SPEED_INNER_WHEEL);
			}
			else if (!FRONT_SIDE_WALL_HIT)
			{
				j++;
				if (i > 0)
					i--;
				if (j > 5)
					j = 5;
				pidDrive(SLOW_SPEED_WALL_WHEEL, SLOW_SPEED_INNER_WHEEL + j);
			}
			else
			{
				pidDrive(SLOW_SPEED_WALL_WHEEL, SLOW_SPEED_INNER_WHEEL);
				i = j = 0;
			}
		}

		stop();
		delayMs(500);
		scraperUp();

		i = j = 0;

    	// Start driving backwards to get a refill
    	clearScreen();
        printString_P(PSTR("Drive backwards"));
		while(!BACK_RIGHT_HIT && !BACK_LEFT_HIT)
		{
			if (!REAR_SIDE_WALL_HIT && !FRONT_SIDE_WALL_HIT)
			{
				//lost the wall
				i++;
				if (j > 0)
					j--;
				if (i > 5)
					i = 5;
				pidDrive(-SLOW_SPEED_WALL_WHEEL, -SLOW_SPEED_INNER_WHEEL - i);
			}
			else if (!REAR_SIDE_WALL_HIT)
			{
				i++;
				if (j > 0)
					j--;
				if (i > 5)
					i = 5;
				pidDrive(-SLOW_SPEED_WALL_WHEEL, -SLOW_SPEED_INNER_WHEEL - i);
			}
			else if (!FRONT_SIDE_WALL_HIT)
			{
				j++;
				if (i > 0)
					i--;
				if (j > 5)
					j = 5;
				pidDrive(-SLOW_SPEED_WALL_WHEEL - j, -SLOW_SPEED_INNER_WHEEL);
			}
			else
			{
				pidDrive(-SLOW_SPEED_WALL_WHEEL, -SLOW_SPEED_INNER_WHEEL);
				i = j = 0;
			}
		}

		refills++;
	}
    haltRobot();

	// Wait for end of competition
	clearScreen();
	printString_P(PSTR("Remaining"));
	u08 priorSeconds = 255;
	while (secCount < COMPETITION_DURATION_SECS)
	{
		// only print when the time has changed
		if (secCount != priorSeconds)
		{
			priorSeconds = secCount;
			lcdCursor(0, 11);
			u08 secsRemaining = COMPETITION_DURATION_SECS - secCount;
			// print minutes
			printChar((secsRemaining / 60) + '0');
			printChar(':');
			// print seconds (tens digit)
			printChar(((secsRemaining % 60) / 10) + '0');
			// print seconds (ones digit)
			printChar(((secsRemaining % 60) % 10) + '0');
			printChar('s');
		}
	}

	victoryDance();
} // End competition
/************************************************************************************************/

//! Do da Dance!
void victoryDance()
{
	// Done, declare domination!
	clearScreen();
	printString_P(PSTR("    PWNED!!!"));
	lowerLine();
	printString_P(PSTR("ReapedYourBalls"));

	// Press button to skip
	for (u08 i = 0; i < 4 && !getButton1(); i++)
	{
		pidDrive(FAST_SPEED_WALL_WHEEL, FAST_SPEED_INNER_WHEEL);
		scraperDown();
		delayMs(500);
		if (getButton1())
		{
			break;
		}
		pidDrive(-SLOW_SPEED_WALL_WHEEL, -SLOW_SPEED_INNER_WHEEL);
		scraperUp();
		delayMs(700);
	}
	haltRobot();
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

static void pidDrive(s08 wallSpeed, s08 innerSpeed)
{
	s08 wallMotorSpeed;
	s08 innerMotorSpeed;
	float Kp = 0.15;
	//float Ki = 0.15;

	// If wall motor is counting ticks faster error will be negative, error is calculated in the interrupt.
	if (wallSpeed > 0)
	{
		wallMotorSpeed  = (s08)(wallSpeed  + (Kp * error)); //+ (Ki * totalError));
		innerMotorSpeed = (s08)(innerSpeed - (Kp * error)); //- (Ki * totalError));

		// limit error from making motor speed go negative and turning the wheel backwards
		if (wallMotorSpeed < 0)
			wallMotorSpeed = 0;

		if (innerMotorSpeed < 0)
			innerMotorSpeed = 0;
	}
	else
	{
		wallMotorSpeed  = (s08)(wallSpeed  - (Kp * error)); //+ (Ki * totalError));
		innerMotorSpeed = (s08)(innerSpeed + (Kp * error)); //- (Ki * totalError));

		// limit error from making motor speed go negative and turning the wheel backwards
		if (wallMotorSpeed > 0)
			wallMotorSpeed = 0;

		if (innerMotorSpeed > 0)
			innerMotorSpeed = 0;
	}

	wallMotor(wallMotorSpeed);
	innerMotor(innerMotorSpeed);

	/*
	upperLine();
	print_s16(error);
	lowerLine();
	print_s16(totalWallEncoderTicks);
	printChar(' ');
	print_s16(totalInnerEncoderTicks);
	*/
}

static void turnLeft()
{
	wallMotor(TURN_SPEED_WALL_WHEEL);
	innerMotor(-TURN_SPEED_INNER_WHEEL);
}

static void stop()
{
	wallMotor(0);
	innerMotor(0);
}

static void launcherSpeed(u08 speed)
{
	servo(SERVO_LEFT_LAUNCHER, speed);
	servo(SERVO_RIGHT_LAUNCHER, speed);
}

static void scraperDown()
{
	servo(SERVO_SCRAPER, SCRAPER_MOSTLY_DOWN);
	delayMs(400);
	servo(SERVO_SCRAPER, SCRAPER_DOWN);
}

static void scraperUp()
{
	servo(SERVO_SCRAPER, SCRAPER_UP);
}

static void feederOn()
{
	servo(SERVO_FEEDER, FEEDER_RUNNING);
}

static void feederOff()
{
	servoOff(SERVO_FEEDER);
}

//! Test Mode pages.
enum {
	TEST_BatteryVoltage,
	TEST_Switches,
	TEST_EncoderTicks,
	TEST_EncoderReadings,
	TEST_DriveMotors,
	NUM_Tests
};

//! Runs the Test Mode functionality.
static void testMode()
{
	printString_P(PSTR("Test Mode"));

	bool exit = FALSE;
	u08 page = 0;

	innerEncoderTicks = 0;
	wallEncoderTicks  = 0;

	//stay in test mode pages until user exits
	while (exit == FALSE)
	{
		clearScreen();
		//print the static top line
		switch (page)
		{
			case TEST_BatteryVoltage:
				printString_P(PSTR("Voltage, Reading"));
				break;
			case TEST_Switches:
				printString_P(PSTR("SW:BL BR SB SF F"));
				break;
			case TEST_EncoderTicks:
				printString_P(PSTR("W EncoderTicks I"));
				break;
			case TEST_EncoderReadings:
				printString_P(PSTR("W EncoderInput I"));
				break;
			case TEST_DriveMotors:
				printString_P(PSTR("W pidDrive I"));
				break;
			default:
				printString_P(PSTR("invalid testpage"));
				SOFTWARE_FAULT(PSTR("invalid testpage"), page, 0);
				break;
		}

		//loop to keep updating the values on the LCD
		while (1)
		{
			switch (page)
			{
				case TEST_BatteryVoltage:
					lowerLine();
					printVoltage(convertToBatteryVoltage(batteryReading));
					lcdCursor(0, 9);
					print_u16(batteryReading);
					break;
				case TEST_Switches:
					lcdCursor(1, 3);
					printChar(digitalInput(SWITCH_BACK_WALL_LEFT) + '0');
					lcdCursor(1, 6);
					printChar(digitalInput(SWITCH_BACK_WALL_RIGHT) + '0');
					lcdCursor(1, 9);
					printChar(digitalInput(SWITCH_SIDE_WALL_REAR) + '0');
					lcdCursor(1, 12);
					printChar(digitalInput(SWITCH_SIDE_WALL_FRONT) + '0');
					lcdCursor(1, 15);
					printChar(digitalInput(SWITCH_FRONT_WALL) + '0');
					break;
				case TEST_EncoderTicks:
					lowerLine();
					print_u16(wallEncoderTicks);
					printChar(' ');
					print_u16(innerEncoderTicks);
					break;
				case TEST_EncoderReadings:
					lowerLine();
					print_u16(wallEncoderReading);
					printChar(' ');
					print_u16(innerEncoderReading);
					break;
				case TEST_DriveMotors:
					pidDrive(SLOW_SPEED_WALL_WHEEL, SLOW_SPEED_INNER_WHEEL);
					lowerLine();
					print_u16(wallEncoderTicks);
					printChar(' ');
					print_u16(innerEncoderTicks);
					break;
				default:
					printString_P(PSTR("invalid testpage"));
					SOFTWARE_FAULT(PSTR("invalid testpage"), page, 0);
					break;
			}

			//poll button state
			if (getButton1())
			{
				//long button hold exits, brief button tap advances the page
				if (buttonHeld())
				{
					exit = TRUE;
				}
				else
				{
					page++;
					if (page >= NUM_Tests)
					{
						page = 0;
					}
				}
				//break out of the inner update loop
				break;
			}
			//poll scroll switches
			else if (digitalInput(SWITCH_SCROLL_UP) == 0)
			{
				page--;
				if (page >= NUM_Tests)
				{
					page = NUM_Tests - 1;
				}
				//wait for switch to be released
				while (digitalInput(SWITCH_SCROLL_UP) == 0)
					;
				//break out of the inner update loop
				break;
			}
			else if (digitalInput(SWITCH_SCROLL_DOWN) == 0)
			{
				page++;
				if (page >= NUM_Tests)
				{
					page = 0;
				}
				//wait for switch to be released
				while (digitalInput(SWITCH_SCROLL_DOWN) == 0)
					;
				//break out of the inner update loop
				break;
			}
		}
		//stop/reset any motors/servos that might have been changed
		haltRobot();
	}
}

static void printVoltage(u16 milliVolts)
{
	//if there is a digit in the ten-thousands place
	if (milliVolts >= 10000)
	{
		//print the ten-thousands digit
		printChar((milliVolts / 10000) + '0');
	}
	else
	{
		//print a space so our total length is always the same
		printChar(' ');
	}

	//print the thousands digit
	printChar(((milliVolts % 10000) / 1000) + '0');

	//print the decimal
	printChar('.');

	//print the hundreds digit
	printChar(((milliVolts % 1000) / 100) + '0');
	//print the tens digit
	printChar(((milliVolts % 100 ) / 10) + '0');
	//print the ones digit
	printChar((milliVolts % 10) + '0');
	//print V for Voltage units
	printChar('V');
}

//! Converts the battery voltage divider reading to a voltage (in milliVolts).
u16 convertToBatteryVoltage(u16 reading)
{
	//multiply by 1000 to convert volts to milliVolts, divide by ADC resolution to get a voltage.
	u32 readingMillivolts = (u32)reading * AREF_VOLTAGE * 1000 / NUM_ADC10_VALUES;
	u16 batteryMillivolts = (u16)(readingMillivolts * (RESISTOR_BATTERY_LOWER + RESISTOR_BATTERY_UPPER) / RESISTOR_BATTERY_LOWER);
	return batteryMillivolts;
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

void haltRobot()
{
	//stop drive motors first
	innerMotor(0);
	wallMotor(0);

	//raise scraper
	scraperUp();

	//stop feeder
	feederOff();

	//stop launchers
	servo(SERVO_LEFT_LAUNCHER, LAUNCHER_SPEED_STOPPED);
	servo(SERVO_RIGHT_LAUNCHER, LAUNCHER_SPEED_STOPPED);

	//power off scraper after it has had time to raise
	delayMs(500);
	servoOff(SERVO_SCRAPER);

	clearScreen();
	printString_P(PSTR("HALTED!"));
}
/*
static void downLink()
{
	//pack all of the digital switches into one byte
	u08 switches = digitalInput(SWITCH_BACK_WALL_LEFT) << 4;
	switches |= digitalInput(SWITCH_BACK_WALL_RIGHT) << 3;
	switches |= digitalInput(SWITCH_SIDE_WALL_REAR) << 2;
	switches |= digitalInput(SWITCH_SIDE_WALL_FRONT) << 1;
	switches |= digitalInput(SWITCH_FRONT_WALL);

	u08 downlinkData[5];
	downlinkData[0] = switches;
	downlinkData[1] = (u08)(innerEncoderTicks >> 8);
	downlinkData[2] = (u08)innerEncoderTicks;
	downlinkData[3] = (u08)(wallEncoderTicks >> 8);;
	downlinkData[4] = (u08)wallEncoderTicks;

	sendPacket(TELEMETRY_DATA, downlinkData, sizeof(downlinkData));
}*/
