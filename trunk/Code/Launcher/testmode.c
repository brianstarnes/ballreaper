#include "testmode.h"

#include "ADC.h"
#include "debug.h"
#include "launcherPackets.h"
#include "LCD.h"
#include "main.h"
#include "motors.h"
#include "serial.h"
#include "servos.h"
#include "util.h"
#include "utility.h"

static void printVoltage(u16 milliVolts);

//! Test Mode pages.
enum {
	TEST_BatteryVoltage,
	TEST_Switches,
	TEST_EncoderTicks,
	TEST_EncoderReadings,
	TEST_DriveMotors,
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

	resetEncoders();

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
				if (robotID == LEFT_ROBOT)
					printString_P(PSTR("SW:B SB SF F"));
				else
				  printString_P(PSTR(" P BL BR SB SF F"));
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
					lcdCursor(1, 9);
					print_u16(batteryReading);
					break;
				case TEST_Switches:
					if (robotID == LEFT_ROBOT)
					{
						lcdCursor(1, 3);
						printChar(digitalInput(LSWITCH_BACK) + '0');
						lcdCursor(1, 6);
						printChar(digitalInput(LSWITCH_SIDE_WALL_REAR) + '0');
						lcdCursor(1, 9);
						printChar(digitalInput(LSWITCH_SIDE_WALL_FRONT) + '0');
						lcdCursor(1, 12);
						printChar(digitalInput(LSWITCH_FRONT) + '0');
					}
					else
					{
						lcdCursor(1, 1);
						printChar(digitalInput(SWITCH_PIVOT) + '0');
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
					}
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
