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

//! The resistance in Ohms of the battery's voltage divider resistor connected to positive.
#define RESISTOR_BATTERY_UPPER 10000
//! The resistance in Ohms of the battery's voltage divider resistor connected to ground.
#define RESISTOR_BATTERY_LOWER 9980

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

//! The number of seconds per competition round.
#define COMPETITION_DURATION_SECS (3 * 60)

//! The drive motor that is always on the inside of the course.
#define innerMotor(speedAndDirection) motor0(127 + speedAndDirection)
//! The drive motor that always runs along the wall.
#define wallMotor(speedAndDirection) motor1(127 + speedAndDirection)

// Right robot switches
#define REAR_SIDE_WALL_HIT  !digitalInput(SWITCH_SIDE_WALL_REAR)
#define FRONT_SIDE_WALL_HIT !digitalInput(SWITCH_SIDE_WALL_FRONT)
#define FRONT_HIT           !digitalInput(SWITCH_FRONT_WALL)
#define BACK_RIGHT_HIT      !digitalInput(SWITCH_BACK_WALL_RIGHT)
#define BACK_LEFT_HIT       !digitalInput(SWITCH_BACK_WALL_LEFT)
#define PIVOT_HIT           !digitalInput(SWITCH_PIVOT)

// Left robot switches
#define LREAR_SIDE_WALL_HIT  !digitalInput(LSWITCH_SIDE_WALL_REAR)
#define LFRONT_SIDE_WALL_HIT !digitalInput(LSWITCH_SIDE_WALL_FRONT)
#define LFRONT_HIT           !digitalInput(LSWITCH_FRONT)
#define LBACK_HIT            !digitalInput(LSWITCH_BACK)

// Robot ID input
#define LEFT_ROBOT_ID        !digitalInput(SWITCH_ROBOT_ID)

//Back wall length in ticks
#define BACK_WALL_TICK_LEN   330

enum servos
{
	SERVO_SCRAPER, //!< The servo that raises/lowers the scraper arm used to collect balls from a trough.
	SERVO_FEEDER, //!< The servo that pushes balls into the launcher wheels.
	SERVO_LEFT_LAUNCHER, //!< The left launcher wheel motor, controlled via the RoboClaw.
	SERVO_RIGHT_LAUNCHER //!< The right launcher wheel motor, controlled via the RoboClaw.
};

enum servoPositions
{
	RSCRAPER_DOWN          = 6, //!< The final position to lower the right scraper arm to to collect balls.
	RSCRAPER_MOSTLY_DOWN   = 10, //!< The initial position to lower the right scraper arm to to avoid whacking the trough.
	RSCRAPER_UP            = 134, //!< The raised position for the right scraper arm.
	LSCRAPER_DOWN          = 128, //!< The final position to lower the left scraper arm to to collect balls.
	LSCRAPER_MOSTLY_DOWN   = 127, //!< The initial position to lower the left scraper arm to to avoid whacking the trough.
	LSCRAPER_UP            = 6, //!< The raised position for the left scraper arm.
	FEEDER_STOPPED         = 128,
	FEEDER_RUNNING         = 200,
	LAUNCHER_SPEED_STOPPED = 128, //!< The center servo setting that the RoboClaw interprets as stopped.
	LAUNCHER_SPEED_NEAR    = 150, //!< The minimum speed to spin the launcher wheels at, when closest to the goal.
	LAUNCHER_SPEED_FAR     = 155,  //!< The maximum speed to spin the launcher wheels at, when farthest away from the goal.
};

typedef enum
{
	SWITCH_BACK_WALL_LEFT  = 2,
	SWITCH_BACK_WALL_RIGHT = 3,
	SWITCH_SIDE_WALL_REAR  = 4,
	SWITCH_SIDE_WALL_FRONT = 5,
	SWITCH_FRONT_WALL      = 6,
	SWITCH_PIVOT           = 7,
	SWITCH_SCROLL          = 8,
	SWITCH_ROBOT_ID        = 9
} rightSwitch_t;

typedef enum
{
	LSWITCH_BACK            = 3,
	LSWITCH_SIDE_WALL_REAR  = 4,
	LSWITCH_SIDE_WALL_FRONT = 5,
	LSWITCH_FRONT           = 6
} leftSwitch_t;

typedef enum
{
	ANALOG_WHEEL_ENCODER_INNER = 0, //!< The left wheel encoder (QRB-1114 reflective sensor).
	ANALOG_WHEEL_ENCODER_WALL  = 1, //!< The right wheel encoder (QRB-1114 reflective sensor).
	ANALOG_BATTERY_VOLTAGE     = 2, //!< The analog input that reads the motor battery, via a voltage divider.
} analog_t;

//! The number of black and white stripes on the encoder wheel
#define ENCODER_TICKS 46

//! Thresholds used for wheel encoder hysteresis.
enum encoderThresholds
{
	ENCODER_THRESHOLD_INNER_LOW  = 250,
	ENCODER_THRESHOLD_INNER_HIGH = 400,
	ENCODER_THRESHOLD_WALL_LOW   = 250,
	ENCODER_THRESHOLD_WALL_HIGH  = 400
};

//wall motor is stronger/faster
enum motorSpeeds
{
	FAST_SPEED_INNER_WHEEL    = 50,
	FAST_SPEED_WALL_WHEEL     = 50,
	SLOW_SPEED_INNER_WHEEL    = 20,
	SLOW_SPEED_WALL_WHEEL     = 20,
	SLOW_SPEED_BK_INNER_WHEEL = 13,
	SLOW_SPEED_BK_WALL_WHEEL  = 13,
	TURN_SPEED_INNER_WHEEL    = 45,
	TURN_SPEED_WALL_WHEEL     = 45
};

//RobotID values
enum {
	LEFT_ROBOT,
	RIGHT_ROBOT
};

//Prototypes
void pauseCompetition();
void resumeCompetition();

//globals
extern u08 robotID;

extern volatile u16 innerEncoderTicks;
extern volatile u16 totalInnerEncoderTicks;
extern volatile u16 wallEncoderTicks;
extern volatile u16 totalWallEncoderTicks;
extern volatile u08 innerEncoderState;
extern volatile u08 wallEncoderState;
extern volatile u16 innerEncoderReading;
extern volatile u16 wallEncoderReading;
extern volatile u16 batteryReading;
extern volatile s16 error;
extern volatile s16 totalError;
extern volatile bool pause;

#endif
