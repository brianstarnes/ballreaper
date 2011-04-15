#include "ADC.h"
#include "debug.h"
#include "launcherPackets.h"
#include "LCD.h"
#include "main.h"
#include "motors.h"
#include "rtc.h"
#include "serial.h"
#include "servos.h"
#include "util.h"
#include "utility.h"

#include <math.h>
#include <util/atomic.h>

static s08 wallSpeed = 0;
static s08 innerSpeed = 0;
static int i, j;
u08 pidStop = TRUE;

static s16 curLauncherSpeed = 128;
static s16 requestedLauncherSpeed = 128;

#define LAUNCHER_SPEED_STEP 1

#define LIMIT(v, min, max) (((v) < (min)) ? (min) : (((v) > (max)) ? (max) : (v)))
#define ABS(v) (((v) < 0) ? -(v) : (v))

void driveForward(s08 w, s08 i)
{
	wallMotor(LIMIT(w, -100, 100));
	innerMotor(LIMIT(i, -100, 100));
}

void pidDrive(s08 w, s08 i)
{
	wallSpeed = w;
	innerSpeed = i;

	pidStop = FALSE;
}

void pidExec()
{
	s08 wallMotorSpeed;
	s08 innerMotorSpeed;
	float Kp = 0.15;
	//float Ki = 0.15;

	//Do not drive on PID
	if (pidStop)
		 return;

	// If wall motor is counting ticks faster error will be negative, error is calculated in the interrupt.
	if (wallSpeed > 0)
		wallMotorSpeed  = (s08)(wallSpeed  + (Kp * error)); //+ (Ki * totalError));
	else
		wallMotorSpeed  = (s08)(wallSpeed  - (Kp * error)); //+ (Ki * totalError));

	if (innerSpeed > 0)
		innerMotorSpeed = (s08)(innerSpeed - (Kp * error)); //- (Ki * totalError));
	else
		innerMotorSpeed = (s08)(innerSpeed + (Kp * error)); //- (Ki * totalError));

	wallMotor(LIMIT(wallMotorSpeed, -100, 100));
	innerMotor(LIMIT(innerMotorSpeed, -100, 100));
}

void compCollectFwd()
{
	i = j = 0;

	stop();
	scraperDown();
	feederOn();
	launcherSpeed(LAUNCHER_SPEED_FAR);

	clearScreen();
    printString_P(PSTR("Drive forward"));

	resetEncoders();
}

void compCollectBack()
{
	i = j = 0;

	stop();
	scraperUp();
	feederOff();
	launcherSpeed(LAUNCHER_SPEED_STOPPED);
	delayMs(500);

	// Start driving backwards to get a refill
	clearScreen();
    printString_P(PSTR("Drive backwards"));
}

void hugWallForwards()
{
	lowerLine();
	if (!REAR_SIDE_WALL_HIT && !FRONT_SIDE_WALL_HIT)
	{
		printString_P(PSTR("Lost"));
		//lost the wall, turn back into the wall
		pidStop = TRUE;
		driveForward(SLOW_SPEED_WALL_WHEEL - 7, SLOW_SPEED_INNER_WHEEL + 10);
	}
	else if (!FRONT_SIDE_WALL_HIT)
	{
		printString_P(PSTR("into"));
		//Turn into the wall
		pidStop = TRUE;
		driveForward(SLOW_SPEED_WALL_WHEEL - 7, SLOW_SPEED_INNER_WHEEL + 10);
	}
/*
	else if (!REAR_SIDE_WALL_HIT)
	{
		printString_P(PSTR("away"));
		//Turn away from the wall
		pidStop = TRUE;
		driveForward(SLOW_SPEED_WALL_WHEEL + 20, SLOW_SPEED_INNER_WHEEL);
	}
*/
	else
	{
		printString_P(PSTR("strt"));
		//Drive straight
		pidDrive(SLOW_SPEED_WALL_WHEEL, SLOW_SPEED_INNER_WHEEL + 3);
	}
}

void hugWallBackwards()
{
	lowerLine();
	if (!REAR_SIDE_WALL_HIT && !FRONT_SIDE_WALL_HIT)
	{
		printString_P(PSTR("Lost"));
		//lost the wall, turn back into the wall
		pidStop = TRUE;
		driveForward(-SLOW_SPEED_WALL_WHEEL + 7, -SLOW_SPEED_INNER_WHEEL - 10);
	}
	else if (!REAR_SIDE_WALL_HIT)
	{
		printString_P(PSTR("into"));
		//Turn into the wall
		pidStop = TRUE;
		driveForward(-SLOW_SPEED_WALL_WHEEL + 7, -SLOW_SPEED_INNER_WHEEL - 10);
	}
	else
	{
		printString_P(PSTR("strt"));
		//Drive straight
		pidDrive(-SLOW_SPEED_WALL_WHEEL, -SLOW_SPEED_INNER_WHEEL - 3);
	}
}

void compDone()
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
void victoryDance()
{
	// Done, declare domination!
	clearScreen();
	printString_P(PSTR("    PWNED!!!"));
	lowerLine();
	printString_P(PSTR("ReapedYourBalls"));

	haltRobot();
}

void turnLeft()
{
	wallMotor(TURN_SPEED_WALL_WHEEL);
	innerMotor(-TURN_SPEED_INNER_WHEEL);
}

void turnRight()
{
	wallMotor(-TURN_SPEED_WALL_WHEEL);
	innerMotor(TURN_SPEED_INNER_WHEEL);
}

void stop()
{
	wallMotor(0);
	innerMotor(0);
	brake0(255);
	brake1(255);
}

//! Sets the speed of the launcher motors: 128-255 for forward operation.
void launcherSpeed(u08 speed)
{
	requestedLauncherSpeed = speed;
}

void launcherExec()
{
	u32 curTime = getMsCount();

	if ((curTime - lastUpdateTime) > 250)
	{
		if (ABS(requestedLauncherSpeed - curLauncherSpeed) <= LAUNCHER_SPEED_STEP)
			curLauncherSpeed = requestedLauncherSpeed;
		else if (requestedLauncherSpeed > curLauncherSpeed)
			curLauncherSpeed += LAUNCHER_SPEED_STEP;
		else if (requestedLauncherSpeed < curLauncherSpeed)
			curLauncherSpeed -= LAUNCHER_SPEED_STEP;

		servo(SERVO_LEFT_LAUNCHER, curLauncherSpeed);
		servo(SERVO_RIGHT_LAUNCHER, curLauncherSpeed);
	}
/*
	//channel 1 or shutdown
	uart1Transmit(curLauncherSpeed);
	if (curLauncherSpeed != 0)
	{
		//channel 2
		uart1Transmit(128 + curLauncherSpeed);
	}*/
}

void scraperDown()
{
	if (LEFT_ROBOT_ID)
	{
		servo(SERVO_SCRAPER, LSCRAPER_MOSTLY_DOWN);
		delayMs(400);
		servo(SERVO_SCRAPER, LSCRAPER_DOWN);
	}
	else
	{
		servo(SERVO_SCRAPER, RSCRAPER_MOSTLY_DOWN);
		delayMs(400);
		servo(SERVO_SCRAPER, RSCRAPER_DOWN);
	}
}

void scraperUp()
{
	if (LEFT_ROBOT_ID)
		servo(SERVO_SCRAPER, LSCRAPER_UP);
	else
		servo(SERVO_SCRAPER, RSCRAPER_UP);
}

void feederOn()
{
	servo(SERVO_FEEDER, FEEDER_RUNNING);
}

void feederOff()
{
	servoOff(SERVO_FEEDER);
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
}

void resetEncoders()
{
	ATOMIC_BLOCK(ATOMIC_FORCEON)
	{
		innerEncoderTicks = 0;
		wallEncoderTicks = 0;
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

//! Converts the battery voltage divider reading to a voltage (in milliVolts).
u16 convertToBatteryVoltage(u16 reading)
{
	//multiply by 1000 to convert volts to milliVolts, divide by ADC resolution to get a voltage.
	u32 readingMillivolts = (u32)reading * AREF_VOLTAGE * 1000 / NUM_ADC10_VALUES;
	u16 batteryMillivolts = (u16)(readingMillivolts * (RESISTOR_BATTERY_LOWER + RESISTOR_BATTERY_UPPER) / RESISTOR_BATTERY_LOWER);
	return batteryMillivolts;
}
