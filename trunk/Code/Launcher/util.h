#ifndef UTIL_H_
#define UTIL_H_

extern u08 pidStop;

void pidDrive(s08 wallSpeed, s08 innerSpeed);
void pidExec();
void driveForward(s08 wallSpeed, s08 innerSpeed);
void turnLeft();
void turnRight();
void stop();
void launcherSpeed(u08 speed);
void launcherExec();
void scraperDown();
void scraperUp();
void feederOn();
void feederOff();
void haltRobot();
void resetEncoders();
void calibrate(int ticksPerSec, s08* wallMotorSpeed, s08* innerMotorSpeed);
void compCollectFwd();
void compEmptyHopper();
void compCollectBack();
void compDone();
void hugWallForwards();
void hugWallBackwards();
void hugWallStrafe(u08 wallSpeed, u08 innerSpeed);
void victoryDance();
u16 convertToBatteryVoltage(u16 reading);
void filterDigitalInput(u08 *readCount, u08 inputNum);

// globals
extern u08 dBackLeft;
extern u08 dBackRight;
extern u08 dRearSide;
extern u08 dFrontSide;
extern u08 dFront;
extern u08 dSide;

#endif
