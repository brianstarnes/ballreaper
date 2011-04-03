#include "compRight.h"

#include "ADC.h"
#include "debug.h"
#include "launcherPackets.h"
#include "LCD.h"
#include "main.h"
#include "motors.h"
#include "packetprotocol.h"
#include "rtc.h"
#include "serial.h"
#include "servos.h"
#include "util.h"
#include "utility.h"

static void hugWallForwards();
static void hugWallBackwards();
static void compStart();
static void compTurnLeft();
static void compCollectFwd();
static void compCollectBack();
static void compDone();
static void victoryDance();


enum {
	COMP_DRIVE_FORWARD,
	COMP_TURN_LEFT,
	COMP_COLLECT_DRV_FWD,
	COMP_COLLECT_DRV_BACK,
	COMP_DONE
};

static int compState;
static u08 refills = 0;
static int i, j;

void compRightInit()
{
	rtcRestart();

	configPacketProcessor(&validateLauncherPacket, &execLauncherPacket, LAST_UplinkPacketType - 1);

	compStart();
	compState = COMP_DRIVE_FORWARD;
}

void compRightExec()
{
	switch (compState) {
		case COMP_DRIVE_FORWARD:
			//drive until either side wall switches hit
			if (REAR_SIDE_WALL_HIT || FRONT_SIDE_WALL_HIT)
			{
				compTurnLeft();
				pidStop = TRUE;
				compState = COMP_TURN_LEFT;
			}
			break;

		case COMP_TURN_LEFT:
			// Turn left until you hit the 90 switch
			if (PIVOT_HIT)
			{
				stop();
				launcherSpeed(180);
				feederOn();
				// Start Ball Reaping! We only get 2 refills so make them count!
				compCollectFwd();
				compState = COMP_COLLECT_DRV_FWD;
			}
			break;

		case COMP_COLLECT_DRV_FWD:
			if (FRONT_HIT)
			{
				compCollectBack();
				compState = COMP_COLLECT_DRV_BACK;
			}
			else {
				hugWallForwards();
			}
			break;

		case COMP_COLLECT_DRV_BACK:
			if (BACK_RIGHT_HIT || BACK_LEFT_HIT)
			{
				refills++;
				if (refills == 2)
				{
					compDone();
					compState = COMP_DONE;
				}
				else
				{
					compCollectFwd();
					compState = COMP_COLLECT_DRV_FWD;
				}
			}
			else
			{
				hugWallBackwards();
			}
			break;

		case COMP_DONE:
			buttonWait();
			break;
	}
} // End competition

static void hugWallForwards()
{
	if (!REAR_SIDE_WALL_HIT && !FRONT_SIDE_WALL_HIT)
	{
		//lost the wall
		if (j < 5)
			j++;
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

static void hugWallBackwards()
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

static void compStart()
{
	resetEncoders();

	clearScreen();
	lowerLine();
	printString_P(PSTR("Drive forward"));

	pidDrive(FAST_SPEED_WALL_WHEEL, FAST_SPEED_INNER_WHEEL);
}

static void compTurnLeft()
{
	// Hit wall so turn left
	clearScreen();
	printString_P(PSTR("Turn Left"));

    turnLeft();
}

static void compCollectFwd()
{
	i = j = 0;

	stop();
	delayMs(500);
	scraperDown();

	// Start driving forward to pick balls up
	clearScreen();
    printString_P(PSTR("Drive forward"));
}

static void compCollectBack()
{
	i = j = 0;

	stop();
	delayMs(500);
	scraperUp();

	// Start driving backwards to get a refill
	clearScreen();
    printString_P(PSTR("Drive backwards"));
}

static void compDone()
{
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
}

//! Do da Dance!
static void victoryDance()
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
