#ifndef MAIN_H
#define MAIN_H

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
#define RESISTOR_LOGIC_UPPER 10000
//! The resistance in Ohms of the logic battery's voltage divider resistor connected to ground.
#define RESISTOR_LOGIC_LOWER 10000
//! The resistance in Ohms of the motor battery's voltage divider resistor connected to positive.
#define RESISTOR_MOTOR_UPPER 10000
//! The resistance in Ohms of the motor battery's voltage divider resistor connected to ground.
#define RESISTOR_MOTOR_LOWER 10000

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
	SCRAPER_UP = 89, //134, //!< The raised position for the scraper arm.
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
	SWITCH_FRONT_WALL = 4
} switch_t;

typedef enum
{
	ANALOG_MOTOR_BATTERY = 0, //!< The analog input that reads the motor battery, via a voltage divider.
	ANALOG_LOGIC_BATTERY = 1, //!< The analog input that reads the logic/servo battery, via a voltage divider.
	ANALOG_WHEEL_ENCODER_LEFT = 8, //!< The left wheel encoder (QRB-1114 reflective sensor).
	ANALOG_WHEEL_ENCODER_RIGHT = 9, //!< The right wheel encoder (QRB-1114 reflective sensor).
} analog_t;

//! The number of black and white stripes on the encoder wheel
#define ENCODER_TICKS 46

//! Thresholds used for wheel encoder hysteresis.
enum encoderThresholds
{
	ENCODER_THRESHOLD_LEFT_LOW = 100,
	ENCODER_THRESHOLD_LEFT_HIGH = 600,
	ENCODER_THRESHOLD_RIGHT_LOW = 100,
	ENCODER_THRESHOLD_RIGHT_HIGH = 500
};

typedef enum
{
	MOTOR_INNER = 1, //!< The drive motor that is always on the inside of the course.
	MOTOR_WALL = 0 //!< The drive motor that always runs along the wall.
} motor_t;

//wall motor is stronger/faster
enum motorSpeeds
{
	FAST_SPEED_INNER_WHEEL = 55,
	FAST_SPEED_WALL_WHEEL = 50,
	SLOW_SPEED_INNER_WHEEL = 50,
	SLOW_SPEED_WALL_WHEEL = 45,
	TURN_SPEED_INNER_WHEEL = 65,
	TURN_SPEED_WALL_WHEEL = 60
};

//Prototypes
void mainMenu();

#endif
