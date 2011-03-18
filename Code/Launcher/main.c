#include "ADC.h"
#include "debug.h"
#include "globals.h"
#include "LCD.h"
#include "main.h"
#include "motors.h"
#include "packetprotocol.h"
#include "remoteprotocol.h"
#include "serial.h"
#include "servos.h"
#include "utility.h"
#include <avr/pgmspace.h>

static volatile u16 leftEncoderTicks = 0;
static volatile u16 rightEncoderTicks = 0;
static volatile u08 leftEncoderState;
static volatile u08 rightEncoderState;
static volatile u16 leftEncoderReading;
static volatile u16 rightEncoderReading;

//local prototypes
void mainMenu();
void runCompetition();
void testMode();
void polyBotRemote();

void printVoltage(u16 milliVolts);
u16 readLogicBattery();
u16 readMotorBattery();

void launcherSpeed(u08 speed);
void strafeRight();
void driveForward();
void driveBackward();
void turnLeft();
void turnRight();
void stop();
void scraperDown();
void scraperUp();

void printEncoderTicks();
void haltRobot();
void downLink();

//! Initializes PolyBotLibrary and timers, sends bootup packet, prints version.
int main()
{
	initialize();
	uart0Init();
	initPacketDriver();
	sendBootNotification();

	//enable timer0 (set prescaler to /256)
	TCCR0B |= _BV(CS02);
	//enable interrupt for timer0 output compare unit A
	TIMSK0 |= _BV(OCIE0A);

	//wait for button to be released, if it is already down from skipping bootloader
	while (getButton1());
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
	Option_PolyBotRemote,
	NUM_Options
};

//! Displays the main menu and runs the option that the user selects with the knob.
void mainMenu()
{
	u08 choice;
	u08 previousChoice = 255;

	clearScreen();
	printString_P(PSTR("Main Menu"));

	//loop until user makes a selection
	do
	{
		u16 input = knob10();
		choice = input * NUM_Options / NUM_ADC10_VALUES;

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
				case Option_PolyBotRemote:
					printString_P(PSTR("3 PolyBot Remote"));
					break;
				default:
					printString_P(PSTR("invalid choice"));
					SOFTWARE_FAULT(PSTR("invalid choice"), choice, input);
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
		case Option_PolyBotRemote:
			polyBotRemote();
			break;
		default:
			SOFTWARE_FAULT(PSTR("invalid choice"), choice, 0);
			break;
	}
}
/************************************************************************************************/
//! Runs the competition code.
void runCompetition()
{
	u08 rearSideWallHit = 0;
	u08 frontSideWallHit = 0;
	u08 refills = 0;

	clearScreen();
	lowerLine();
	printString_P(PSTR("Drive forward"));

	//drive until at least the side wall switches hit
	while(!rearSideWallHit && !frontSideWallHit)
	{
		driveForward();

		delayMs(1);
		rearSideWallHit =  REAR_SIDE_WALL_HIT;
		frontSideWallHit = FRONT_SIDE_WALL_HIT;
	}

	// Hit wall so turn left
	clearScreen();
	printString_P(PSTR("Turn Left"));

	leftEncoderTicks = 0;
    rightEncoderTicks = 0;

    turnLeft();

	// Turn left 90 degrees
	while(leftEncoderTicks < 25 && rightEncoderTicks < 25);
	stop();

	launcherSpeed(180);

	// Start Ball Reaping!
    while (refills < 2)
	{
		stop();
    	scraperDown();

    	// Start driving forward to pick balls up
    	clearScreen();
        printString_P(PSTR("Drive forward"));
		driveForward();
		while(!FRONT_HIT);

		stop();
		scraperUp();

    	// Start driving backwards to get a refill
    	clearScreen();
        printString_P(PSTR("Drive backwards"));
		driveBackward();
		while(!BACK_RIGHT_HIT || !BACK_LEFT_HIT);
		refills++;
	}

	// Done, declare domination!
	stop();
	clearScreen();
	printString_P(PSTR("  Dominated!!"));
	lowerLine();
	printString_P(PSTR("ReapedYourBalls"));
    while(1);

} // End competition
/************************************************************************************************/

void launcherSpeed(u08 speed)
{
	servo(SERVO_LEFT_LAUNCHER, speed);
	servo(SERVO_RIGHT_LAUNCHER, speed);
}

void driveForward()
{
	motor(MOTOR_WALL, 30);
	motor(MOTOR_INNER, 30);
}

void driveBackward()
{
	motor(MOTOR_WALL, -30);
	motor(MOTOR_INNER, -30);
}

void turnLeft()
{
	motor(MOTOR_WALL, 40);
	motor(MOTOR_INNER, -40);
}

void turnRight()
{
	motor(MOTOR_WALL, -40);
	motor(MOTOR_INNER, 40);
}

void stop()
{
	motor(MOTOR_WALL, 0);
	motor(MOTOR_INNER, 0);
}

void scraperDown()
{
	servo(SERVO_SCRAPER, 0);
}

void scraperUp()
{
	servo(SERVO_SCRAPER, 200);
}

//! Test Mode pages.
enum {
	TEST_BatteryVoltages,
	TEST_BatteryReadings,
	TEST_Switches,
	TEST_EncoderTicks,
	TEST_EncoderReadings,
	TEST_DriveMotors,
	NUM_Tests
};

//! Runs the Test Mode functionality.
void testMode()
{
	printString_P(PSTR("Test Mode"));

	bool exit = FALSE;
	u08 page = 0;

	//stay in test mode pages until user exits
	while (exit == FALSE)
	{
		clearScreen();
		//print the static top line
		switch (page)
		{
			case TEST_BatteryVoltages:
			case TEST_BatteryReadings:
				printString_P(PSTR(" Logic   Motor"));
				break;
			case TEST_Switches:
				printString_P(PSTR("SW:BL BR SB SF F"));
				break;
			case TEST_EncoderTicks:
				printString_P(PSTR("L EncoderTicks R"));
				break;
			case TEST_EncoderReadings:
				//encoder readings
				printString_P(PSTR("L EncoderInput R"));
				break;
			case TEST_DriveMotors:
				//Drive motor test
				printString_P(PSTR("Drive Motor Test"));
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
				case TEST_BatteryVoltages:
					lowerLine();
					printVoltage(readLogicBattery());
					printChar(' ');
					printVoltage(readMotorBattery());
					break;
				case TEST_BatteryReadings:
					lowerLine();
					print_u16(analog10(ANALOG_LOGIC_BATTERY));
					lcdCursor(1, 8);
					print_u16(analog10(ANALOG_MOTOR_BATTERY));
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
					print_u16(leftEncoderTicks);
					printChar(' ');
					print_u16(rightEncoderTicks);
					break;
				case TEST_EncoderReadings:
					lowerLine();
					print_u16(leftEncoderReading);
					printChar(' ');
					print_u16(rightEncoderReading);
					break;
				case TEST_DriveMotors:
					//drive motor tests
					lowerLine();
					printString_P(PSTR("Inner Motor"));
					motor(MOTOR_INNER, 30);
					delayMs(750);
					motor(MOTOR_INNER, -30);
					delayMs(750);
					motor(MOTOR_INNER, 0);
					delayMs(1000);
					lowerLine();
					printString_P(PSTR("Wall Motor "));
					motor(MOTOR_WALL, 30);
					delayMs(750);
					motor(MOTOR_WALL, -30);
					delayMs(750);
					motor(MOTOR_WALL, 0);
					delayMs(3000);
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
		}
	}
}

void polyBotRemote()
{
	printString_P(PSTR("PolyBot Remote"));
	lowerLine();
	printString_P(PSTR("System v" POLYBOT_REMOTE_VERSION));

	//loop until exit, everything else is handled by interrupts
	remoteExited = FALSE;
	while(remoteExited == FALSE)
		;
}

void printVoltage(u16 milliVolts)
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

//! Reads the voltage (in milliVolts) of the battery that powers the logic and servos, via a voltage divider.
u16 readLogicBattery()
{
	u16 readingCounts = analog10(ANALOG_LOGIC_BATTERY);
	//multiply by 1000 to convert volts to milliVolts, divide by ADC resolution to get a voltage.
	u32 readingMillivolts = (u32)readingCounts * AREF_VOLTAGE * 1000 / NUM_ADC10_VALUES;
	u16 batteryMillivolts = (u16)(readingMillivolts * (RESISTOR_LOGIC_LOWER + RESISTOR_LOGIC_UPPER) / RESISTOR_LOGIC_LOWER);
	return batteryMillivolts;
}

//! Reads the voltage (in milliVolts) of the battery that powers the drive and launcher motors, via a voltage divider.
u16 readMotorBattery()
{
	u16 readingCounts = analog10(ANALOG_MOTOR_BATTERY);
	//multiply by 1000 to convert volts to milliVolts, divide by ADC resolution to get a voltage.
	u32 readingMillivolts = (u32)readingCounts * AREF_VOLTAGE * 1000 / NUM_ADC10_VALUES;
	u16 batteryMillivolts = (u16)(readingMillivolts * (RESISTOR_MOTOR_LOWER + RESISTOR_MOTOR_UPPER) / RESISTOR_MOTOR_LOWER);
	return batteryMillivolts;
}

//! Interrupt service routine for counting wheel encoder ticks.
ISR(TIMER0_COMPA_vect)
{
	//read the wheel encoders (QRB-1114 reflective sensors)
	leftEncoderReading = analog10(ANALOG_WHEEL_ENCODER_LEFT);
	rightEncoderReading = analog10(ANALOG_WHEEL_ENCODER_RIGHT);

	if (leftEncoderReading >= ENCODER_THRESHOLD_LEFT_HIGH && leftEncoderState != 1)
	{
		leftEncoderState = 1;
		leftEncoderTicks++;
	}
	else if (leftEncoderReading <= ENCODER_THRESHOLD_LEFT_LOW && leftEncoderState != 0)
	{
		leftEncoderState = 0;
		leftEncoderTicks++;
	}

	if (rightEncoderReading >= ENCODER_THRESHOLD_RIGHT_HIGH && rightEncoderState != 1)
	{
		rightEncoderState = 1;
		rightEncoderTicks++;
	}
	else if (rightEncoderReading <= ENCODER_THRESHOLD_RIGHT_LOW && rightEncoderState != 0)
	{
		rightEncoderState = 0;
		rightEncoderTicks++;
	}
}

//! Prints the left and right wheel encoder ticks.
void printEncoderTicks()
{
	lowerLine();
	print_u08(leftEncoderTicks);
	lcdCursor(1,8);
	print_u08(rightEncoderTicks);
}

void haltRobot()
{
	//stop drive motors first
	motor(MOTOR_INNER, 0);
	motor(MOTOR_WALL, 0);

	//raise scraper and power it off
	servo(SERVO_SCRAPER, SCRAPER_UP);
	servoOff(SERVO_SCRAPER);

	//stop feeder
	servoOff(SERVO_FEEDER);

	//stop launchers
	servo(SERVO_LEFT_LAUNCHER, LAUNCHER_SPEED_STOPPED);
	servo(SERVO_RIGHT_LAUNCHER, LAUNCHER_SPEED_STOPPED);

	clearScreen();
	printString_P("HALTED!");
}

void downLink()
{
	//pack all of the digital switches into one byte
	u08 switches = digitalInput(SWITCH_BACK_WALL_LEFT) << 4;
	switches |= digitalInput(SWITCH_BACK_WALL_RIGHT) << 3;
	switches |= digitalInput(SWITCH_SIDE_WALL_REAR) << 2;
	switches |= digitalInput(SWITCH_SIDE_WALL_FRONT) << 1;
	switches |= digitalInput(SWITCH_FRONT_WALL);

	u08 downlinkData[5];
	downlinkData[0] = switches;
	downlinkData[1] = (u08)(leftEncoderTicks >> 8);
	downlinkData[2] = (u08)leftEncoderTicks;
	downlinkData[3] = (u08)(rightEncoderTicks >> 8);;
	downlinkData[4] = (u08)rightEncoderTicks;

	sendPacket(TELEMETRY_DATA, downlinkData, sizeof(downlinkData));
}
