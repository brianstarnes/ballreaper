
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

#include <util/atomic.h>

static s08 wallSpeed = 0;
static s08 innerSpeed = 0;
u08 pidStop = TRUE;

void pidDrive(s08 w, s08 i)
{
	wallSpeed = w;
	innerSpeed = i;

	pidStop = FALSE;
}

#define LIMIT(v, min, max) (((v) < (min)) ? (min) : (((v) > (max)) ? (max) : (v)))

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

void launcherSpeed(u08 speed)
{
	servo(SERVO_LEFT_LAUNCHER, speed);
	servo(SERVO_RIGHT_LAUNCHER, speed);
}

void scraperDown()
{
	servo(SERVO_SCRAPER, SCRAPER_MOSTLY_DOWN);
	delayMs(400);
	servo(SERVO_SCRAPER, SCRAPER_DOWN);
}

void scraperUp()
{
	servo(SERVO_SCRAPER, SCRAPER_UP);
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

	clearScreen();
	printString_P(PSTR("HALTED!"));
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
