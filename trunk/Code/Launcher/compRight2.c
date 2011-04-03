#include "compRight2.h"

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
static void compTurnRight();
static void compDriveTowardWall();
static void compTurnTowardWall();
static void compTouchFrontToWall();
static void compTouchBackToWall();
static void compCollectFwd();
static void compCollectBack();
static void compDone();
static void victoryDance();


enum {
	CR2_START,
	CR2_TURN_RIGHT,
	CR2_DRIVE_TOWARD_WALL,
	CR2_TURN_TOWARD_WALL,
	CR2_TOUCH_FRONT_TO_WALL,
	CR2_TOUCH_BACK_TO_WALL,
	CR2_COLLECT_DRV_FWD,
	CR2_COLLECT_DRV_BACK,
	CR2_DONE
};

static int compState;
static u08 refills = 0;
static int i, j;

void compRight2Init()
{
	rtcRestart();

	configPacketProcessor(&validateLauncherPacket, &execLauncherPacket, LAST_UplinkPacketType - 1);

	compStart();
	compState = CR2_START;
}

void compRight2Exec()
{
	switch (compState) {

	case CR2_START:
		//drive until either side wall switches hit
		if (innerEncoderTicks >= 26 && wallEncoderTicks >= 26)
		{
			compTurnRight();
		    compState = CR2_TURN_RIGHT;
		}
		else
		{
			pidDrive(FAST_SPEED_WALL_WHEEL, FAST_SPEED_INNER_WHEEL);
		}
		break;

	case CR2_TURN_RIGHT:
		// Turn left 90 degrees
		if (innerEncoderTicks >= 20 && wallEncoderTicks >= 20)
		{
			compDriveTowardWall();
			compState = CR2_DRIVE_TOWARD_WALL;
		}
		else
		{
			pidDrive(-TURN_SPEED_WALL_WHEEL, TURN_SPEED_INNER_WHEEL);
		}
		break;

	case CR2_DRIVE_TOWARD_WALL:
		// go ~ 4ft, then slow down and start alignment with wall. 259 ticks is ~ 4 ft
		if (innerEncoderTicks >= 259 && wallEncoderTicks >= 259)
		{
			compTurnTowardWall();
			compState = CR2_TURN_TOWARD_WALL;
		}
		else
		{
			pidDrive(FAST_SPEED_WALL_WHEEL, FAST_SPEED_INNER_WHEEL);
		}
		break;

	case CR2_TURN_TOWARD_WALL:
		if (wallEncoderTicks >= 20)
		{
			compTouchFrontToWall();
			compState = CR2_TOUCH_FRONT_TO_WALL;
		}
		else
		{
			pidDrive(FAST_SPEED_WALL_WHEEL, -FAST_SPEED_INNER_WHEEL);
		}
		break;

	case CR2_TOUCH_FRONT_TO_WALL:
		if (FRONT_SIDE_WALL_HIT)
		{
			compTouchBackToWall();
			compState = CR2_TOUCH_BACK_TO_WALL;
		}
		else
		{
			pidDrive(FAST_SPEED_WALL_WHEEL, FAST_SPEED_INNER_WHEEL);
		}
		break;

	case CR2_TOUCH_BACK_TO_WALL:
		if (REAR_SIDE_WALL_HIT)
		{
			compState = CR2_COLLECT_DRV_BACK;
		}
		else
		{
			pidDrive(FAST_SPEED_WALL_WHEEL, FAST_SPEED_INNER_WHEEL);
		}
		break;

	case CR2_COLLECT_DRV_FWD:
		if (FRONT_HIT)
		{
			compCollectBack();
	        compState = CR2_COLLECT_DRV_BACK;
		}
		else
		{
			hugWallForwards();
		}
		break;

	case CR2_COLLECT_DRV_BACK:
		if (BACK_RIGHT_HIT || BACK_LEFT_HIT)
		{
			refills++;
			if (refills == 2)
			{
				compDone();
				compState = CR2_DONE;
			}
			else
			{
				compCollectFwd();
				compState = CR2_COLLECT_DRV_FWD;
			}
		}
		else
		{
			hugWallBackwards();
		}
		break;

	case CR2_DONE:
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
	clearScreen();
	lowerLine();
	printString_P(PSTR("Drive forward"));

	stop();
	delayMs(500);

	resetEncoders();
}

static void compTurnRight()
{
	// Hit wall so turn left
	clearScreen();
	lowerLine();
	printString_P(PSTR("Turn Right"));

	stop();
	delayMs(500);

	resetEncoders();
}

static void compDriveTowardWall()
{
	clearScreen();
	lowerLine();
	printString_P(PSTR("Drive to wall"));

	stop();
	delayMs(500);

	resetEncoders();
}

static void compTurnTowardWall()
{
	clearScreen();
	lowerLine();
	printString_P(PSTR("Turn to wall"));

	stop();
	delayMs(500);

	resetEncoders();

    turnLeft();
}

static void compTouchFrontToWall()
{
	clearScreen();
	lowerLine();
	printString_P(PSTR("Touch front"));

	stop();
	delayMs(500);
}

static void compTouchBackToWall()
{
	clearScreen();
	lowerLine();
	printString_P(PSTR("Touch back"));
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
