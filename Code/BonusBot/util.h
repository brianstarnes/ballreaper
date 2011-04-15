#ifndef UTIL_H_
#define UTIL_H_

extern u08 pidStop;

void pidDrive(s08 wallSpeed, s08 innerSpeed);
void pidExec();
void driveForward(s08 wallSpeed, s08 innerSpeed);
void turnLeft();
void turnRight();
void stop();
void scraperDown();
void scraperUp();
void feederOn();
void feederOff();
void haltRobot();
void resetEncoders();
void calibrate(int ticksPerSec, s08* wallMotorSpeed, s08* innerMotorSpeed);
void compCollectFwd();
void compCollectBack();
void compDone();
void hugWallForwards();
void hugWallBackwards();
void victoryDance();
u16 convertToBatteryVoltage(u16 reading);

void launcherSpeed(u08 speed);
void launcherExec();

#endif
