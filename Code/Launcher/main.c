#include "ADC.h"
#include <util/atomic.h>
#include "debug.h"
#include "launcherPackets.h"
#include "LCD.h"
#include "main.h"
#include "motors.h"
#include "packetprotocol.h"
#include "remoteControl.h"
#include "serial.h"
#include "servos.h"
#include "utility.h"
#include <avr/pgmspace.h>

static volatile u16 innerEncoderTicks = 0;
static volatile u16 totalInnerEncoderTicks = 0;
static volatile u16 wallEncoderTicks = 0;
static volatile u16 totalWallEncoderTicks = 0;
static volatile u08 innerEncoderState;
static volatile u08 wallEncoderState;
static volatile u16 innerEncoderReading;
static volatile u16 wallEncoderReading;
static volatile s16 error;
static volatile s16 totalError;
static volatile bool pause = FALSE;

//local prototypes
static void mainMenu();
static void runCompetition();
static void testMode();

static void printVoltage(u16 milliVolts);

static void pidDrive(s08 wallSpeed, s08 innerSpeed);
static void turnLeft();
static void stop();

static void launcherSpeed(u08 speed);
static void scraperDown();
static void scraperUp();

//static void downLink();

//! Initializes PolyBotLibrary and timers, sends bootup packet, prints version.
int main()
{
	//query what kind of reset caused this bootup
	u08 resetCause = MCUSR;
	//reset the MCU Status Register
	MCUSR = 0;

	initialize();
	uart0Init();
	initPacketDriver();
	sendBootNotification(resetCause);

	digitalPullups(0xFF);
	digitalDirections(0);
	adcInit();
	sbi(PORTF, PF0);
	sbi(PORTF, PF1);

	//enable timer0 (set prescaler to /64)
	TCCR0B |= _BV(CS02);// | _BV(CS00);
	//enable interrupt for timer0 output compare unit A
	TIMSK0 |= _BV(OCIE0A);

	//wait for button to be released, if it is already down from skipping bootloader
	while (getButton1());
	//print firmware version and wait for button press
	printString_P(PSTR("ballReaper v" LAUNCHER_FIRMWARE_VERSION));
	buttonWait();

	while (1)
	{

		runCompetition();

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
				choice = NUM_Options - 1;
			//wait for switch to be released
			while (digitalInput(SWITCH_SCROLL_UP) == 0)
				;
		}
		else if (digitalInput(SWITCH_SCROLL_DOWN) == 0)
		{
			choice++;
			if (choice >= NUM_Options)
				choice = 0;
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
				case Option_PolyBotRemote:
					printString_P(PSTR("3 PolyBot Remote"));
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
		case Option_PolyBotRemote:
			polyBotRemote();
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

	ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
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

	// make sure that the robot turned the full 90 degrees and itsn't stuck on the wall with the wheels not moving
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

	// Done, declare domination!
	stop();
	clearScreen();
	printString_P(PSTR("  Dominated!!"));
	lowerLine();
	printString_P(PSTR("ReapedYourBalls"));
    while(1);

} // End competition
/************************************************************************************************/

void pauseCompetition()
{
	pause = TRUE;
}

void resumeCompetition()
{
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
	servo(SERVO_SCRAPER, SCRAPER_DOWN);;
}

static void scraperUp()
{
	servo(SERVO_SCRAPER, SCRAPER_UP);
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
			case TEST_BatteryVoltages:
			case TEST_BatteryReadings:
				printString_P(PSTR(" Logic   Motor"));
				break;
			case TEST_Switches:
				printString_P(PSTR("SW:BL BR SB SF F"));
				break;
			case TEST_EncoderTicks:
				printString_P(PSTR("W EncoderTicks I"));
				break;
			case TEST_EncoderReadings:
				//encoder readings
				printString_P(PSTR("W EncoderInput I"));
				break;
			case TEST_DriveMotors:
				//Drive motor test
				//printString_P(PSTR("Drive Motor Test"));
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
					//drive motor tests
					pidDrive(25, 25);
					/*
					lowerLine();
					printString_P(PSTR("Inner Motor"));
					innerMotor(30);
					delayMs(750);
					innerMotor(-30);
					delayMs(750);
					innerMotor(0);
					delayMs(1000);
					lowerLine();
					printString_P(PSTR("Wall Motor "));
					wallMotor(30);
					delayMs(750);
					wallMotor(-30);
					delayMs(750);
					wallMotor(0);
					*/
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
	innerEncoderReading = analog10(ANALOG_WHEEL_ENCODER_INNER);
	wallEncoderReading = analog10(ANALOG_WHEEL_ENCODER_WALL);

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

void haltRobot()
{
	//stop drive motors first
	innerMotor(0);
	wallMotor(0);

	//raise scraper
	servo(SERVO_SCRAPER, SCRAPER_UP);

	//stop feeder
	servoOff(SERVO_FEEDER);

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
