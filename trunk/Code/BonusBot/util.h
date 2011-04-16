#ifndef UTIL_H_
#define UTIL_H_

enum direction
{
	REVERSE,
	FORWARD
};

extern u08 pidStop;

void stop();
void haltRobot();
void compDone();
void victoryDance();

void launcherSpeed(u08 speed);
void launcherExec();

void vacuumOn();
void vacuumOff();

void frontLeft(u08 speed, u08 direction);
void frontRight(u08 speed, u08 direction);
void backRight(u08 speed, u08 direction);
void backLeft(u08 speed, u08 direction);
void stopMotors();
void strafeRight(u08 speed);
void strafeLeft(u08 speed);
void driveForward(void);
void driveBackwards(void);

#endif
