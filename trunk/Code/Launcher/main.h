#ifndef MAIN_H
#define MAIN_H

#include "globals.h"

/*! Version of the Launcher firmware, part of the response to a ::GET_VERSIONS command.
    Should be incremented when new features or breaking changes are added.
 */
#define LAUNCHER_FIRMWARE_VERSION "0.1"

//! The ADC reference voltage supplied to the microcontroller.
#define AREF_VOLTAGE 5
//! The number of possible 10-bit ADC output values. 10-bit ADC resolution gives 2^10=1024 values.
#define NUM_ADC10_VALUES 1024
//! The maximum ADC output value. ADC has 1024 output values so it ranges from 0 to 1023.
#define ADC_MAX 1023

//! The resistance in Ohms of the logic battery's voltage divider resistor connected to positive.
#define RESISTOR_BATTERY_UPPER 10000
//! The resistance in Ohms of the logic battery's voltage divider resistor connected to ground.
#define RESISTOR_BATTERY_LOWER 10000
//! The resistance in Ohms of the motor battery's voltage divider resistor connected to positive.

//! The number of Lithium polymer cells in the logic battery pack.
#define LOGIC_BATTERY_NUM_CELLS 2
//! The voltage (in milliVolts) at which to alert the user that the logic battery is low.
#define LOGIC_BATTERY_VOLTAGE_WARN 3500 * LOGIC_BATTERY_NUM_CELLS
//! The voltage (in milliVolts) at which to turn off servos and anything else possible on the logic battery.
#define LOGIC_BATTERY_VOLTAGE_CUTOFF 3000 * LOGIC_BATTERY_NUM_CELLS

//! The number of Lithium polymer cells in the motor battery pack.
#define LOGIC_BATTERY_NUM_CELLS 2
//! The voltage (in milliVolts) at which to alert the user that the logic battery is low.
#define LOGIC_BATTERY_VOLTAGE_WARN 3500 * LOGIC_BATTERY_NUM_CELLS
//! The voltage (in milliVolts) at which to turn off servos and anything else possible on the logic battery.
#define LOGIC_BATTERY_VOLTAGE_CUTOFF 3000 * LOGIC_BATTERY_NUM_CELLS

//! The drive motor that is always on the inside of the course.
#define innerMotor(speedAndDirection) motor0(127 + speedAndDirection)
//! The drive motor that always runs along the wall.
#define wallMotor(speedAndDirection) motor1(127 + speedAndDirection)

#define REAR_SIDE_WALL_HIT  !digitalInput(SWITCH_SIDE_WALL_REAR)
#define FRONT_SIDE_WALL_HIT !digitalInput(SWITCH_SIDE_WALL_FRONT)
#define FRONT_HIT           !digitalInput(SWITCH_FRONT_WALL)
#define BACK_RIGHT_HIT      !digitalInput(SWITCH_BACK_WALL_RIGHT)
#define BACK_LEFT_HIT       !digitalInput(SWITCH_BACK_WALL_LEFT)

enum servos
{
	SERVO_SCRAPER, //!< The servo that raises/lowers the scraper arm used to collect balls from a trough.
	SERVO_FEEDER, //!< The servo that pushes balls into the launcher wheels.
	SERVO_LEFT_LAUNCHER, //!< The left launcher wheel motor, controlled via the Sabertooth 2x5.
	SERVO_RIGHT_LAUNCHER //!< The right launcher wheel motor, controlled via the Sabertooth 2x5.
};

enum servoPositions
{
	SCRAPER_DOWN = 4, //!< The final position to lower the scraper arm to to collect balls.
	SCRAPER_MOSTLY_DOWN = 10, //!< The initial position to lower the scraper arm to to avoid whacking the trough.
	SCRAPER_UP = 134, //!< The raised position for the scraper arm.
	FEEDER_STOPPED = 128,
	FEEDER_RUNNING = 140,
	LAUNCHER_SPEED_STOPPED = 128, //!< The center servo setting that the Sabertooth interprets as stopped.
	LAUNCHER_SPEED_NEAR = 188, //!< The minimum speed to spin the launcher wheels at, when closest to the goal.
	LAUNCHER_SPEED_FAR = 228,  //!< The maximum speed to spin the launcher wheels at, when farthest away from the goal.
};

typedef enum
{
	SWITCH_BACK_WALL_LEFT = 0,
	SWITCH_BACK_WALL_RIGHT = 1,
	SWITCH_SIDE_WALL_REAR = 2,
	SWITCH_SIDE_WALL_FRONT = 3,
	SWITCH_FRONT_WALL = 4,
	SWITCH_SCROLL_UP = 8,
	SWITCH_SCROLL_DOWN = 9
} switch_t;

typedef enum
{
	ANALOG_WHEEL_ENCODER_INNER = 0, //!< The left wheel encoder (QRB-1114 reflective sensor).
	ANALOG_WHEEL_ENCODER_WALL = 1, //!< The right wheel encoder (QRB-1114 reflective sensor).
	ANALOG_BATTERY_VOLTAGE = 2, //!< The analog input that reads the motor battery, via a voltage divider.
} analog_t;

//! The number of black and white stripes on the encoder wheel
#define ENCODER_TICKS 46

//! Thresholds used for wheel encoder hysteresis.
enum encoderThresholds
{
	ENCODER_THRESHOLD_INNER_LOW = 100,
	ENCODER_THRESHOLD_INNER_HIGH = 250,
	ENCODER_THRESHOLD_WALL_LOW = 100,
	ENCODER_THRESHOLD_WALL_HIGH = 250
};

typedef enum
{
	MOTOR_WALL = 0, //!< The drive motor that always runs along the wall.
	MOTOR_INNER = 1 //!< The drive motor that is always on the inside of the course.
} motor_t;

//wall motor is stronger/faster
enum motorSpeeds
{
	FAST_SPEED_INNER_WHEEL = 35,
	FAST_SPEED_WALL_WHEEL = 35,
	SLOW_SPEED_INNER_WHEEL = 29,
	SLOW_SPEED_WALL_WHEEL = 29,
	TURN_SPEED_INNER_WHEEL = 50,
	TURN_SPEED_WALL_WHEEL = 50
};

//Prototypes
u16 convertToBatteryVoltage(u16 reading);
void pauseCompetition();
void resumeCompetition();
void haltRobot();

#endif
