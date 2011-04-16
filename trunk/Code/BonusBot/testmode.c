#include "testmode.h"

#include "ADC.h"
#include "bonusbot.h"
#include "LCD.h"
#include "motors.h"
#include "serial.h"
#include "servos.h"
#include "util.h"
#include "utility.h"


//! Test Mode pages.
enum {
	TEST_Switches,
	TEST_QrdReadings,
	TEST_DriveMotors,
	TEST_Launcher,
	NUM_Tests
};

void testModeInit()
{

}

//! Runs the Test Mode functionality.
void testModeExec()
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
			case TEST_Switches:
				printString_P(PSTR("SW:FL   FR"));
				break;
			case TEST_QrdReadings:
				break;
			case TEST_DriveMotors:
				printString_P(PSTR("Test All Motors"));
				break;
			case TEST_Launcher:
				printString_P(PSTR("Test Launcher"));
				break;
			default:
				printString_P(PSTR("invalid testpage"));
				break;
		}

		//loop to keep updating the values on the LCD
		while (1)
		{
			switch (page)
			{
				case TEST_Switches:
					lcdCursor(1, 3);
					printChar(digitalInput(SWITCH_LEFT) + '0');
					lcdCursor(1, 6);
					printChar(digitalInput(SWITCH_RIGHT) + '0');
					break;
				case TEST_QrdReadings:
					upperLine();
					printChar('F');
					printChar('L');
					print_u16(qrdFrontLeftReading);
					printChar(' ');
					printChar('F');
					printChar('R');
					print_u16(qrdFrontRightReading);
					lowerLine();
					printChar('B');
					printChar('L');
					print_u16(qrdBackLeftReading);
					printChar(' ');
					printChar('B');
					printChar('R');
					print_u16(qrdBackRightReading);
					break;
				case TEST_DriveMotors:
					servo(SERVO_FRONT_RIGHT, DRIVE_SLOW_SPEED);
					servo(SERVO_FRONT_LEFT,  DRIVE_SLOW_SPEED);
					servo(SERVO_BACK_RIGHT,  DRIVE_SLOW_SPEED);
					servo(SERVO_BACK_LEFT,   DRIVE_SLOW_SPEED);
					break;
				case TEST_Launcher:
					motor0(LAUNCHER_LAUNCH_SPEED);
					motor1(LAUNCHER_LAUNCH_SPEED);
				default:
					printString_P(PSTR("invalid testpage"));
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
			else if (digitalInput(SWITCH_SCROLL) == 0)
			{
				page++;
				if (page >= NUM_Tests)
				{
					page = 0;
				}
				//wait for switch to be released
				while (digitalInput(SWITCH_SCROLL) == 0)
					;
				//break out of the inner update loop
				break;
			}
		}
		//stop/reset any motors/servos that might have been changed
		haltRobot();
	}
}
