#ifndef REMOTECONTROL_H
#define REMOTECONTROL_H

#include "globals.h"
#include <avr/version.h>

//Baud rate and error tolerance are set in the init functions in serial.c

//Define the versions returned by the GET_VERSION command
//Version of the Remote System firmware - should be updated when new features or breaking changes are added
#define REMOTE_SYSTEM_VERSION "3.0"

//Version of the board being used.
//Note: this version is simply inferred based on MCU setting in the Makefile that the code is compiled with.
//Therefore it may not be accurate if someone is using a different chip in their board.
#if defined (__AVR_ATmega32__)
	#define BOARD_VERSION "PolyBot 1.0"
#elif defined (__AVR_ATmega644__)
	#define BOARD_VERSION "PolyBot 1.1 644"
#elif defined (__AVR_ATmega644P__)
	#define BOARD_VERSION "PolyBot 1.1 644P"
#elif defined (__AVR_ATmega1281__)
	#define BOARD_VERSION "Xiphos 1.0"
#endif

//Build a version string
#define VERSION REMOTE_SYSTEM_VERSION "|" BOARD_VERSION "|" __AVR_LIBC_VERSION_STRING__ "|" __AVR_LIBC_DATE_STRING__

//Remote System Commands
//***The biggest command is PRINT_STRING with 1 command byte, 16 characters for the LCD line, and a null terminator
#define MAX_DATA 18

enum Commands
{
	//parameterless commands
	CMD_GET_VERSION,
	CMD_LED_ON,
	CMD_LED_OFF,
	CMD_RELAY_ON,
	CMD_RELAY_OFF,
	CMD_LCD_ON,
	CMD_LCD_OFF,
	CMD_CLEAR_SCREEN,
	CMD_LOWER_LINE,
	CMD_GET_BUTTON1,
	CMD_KNOB,
	CMD_KNOB10,
	CMD_SOFT_RESET,
	CMD_STOP_SOUND,
	CMD_EXIT_REMOTE,
	LAST_ParameterlessCommand,

	//single-parameter commands
	CMD_DELAY_MS = LAST_ParameterlessCommand,
	CMD_DELAY_US,
	CMD_PRINT_STRING,
	CMD_DIGITAL_INPUT,
	CMD_ANALOG,
	CMD_ANALOG10,
	CMD_SERVO_OFF,
	CMD_GET_SERVO_RANGE,
	CMD_PLAY_SOUND,
	LAST_SingleParameterCommand,

	//two-parameter commands
	CMD_SET_SERVO_RANGE_BY_INDEX = LAST_SingleParameterCommand,
	CMD_SET_SERVO_RANGE,
	CMD_SERVO,
	CMD_SERVO2,
	CMD_MOTOR,
	CMD_LCD_CURSOR,

	NUM_CMD //!< The number of commands defined
};


//Packet responses - packet types that the microcontroller can send
enum Responses
{
	RESP_BOOTED_UP = 0x30,
	RESP_VERSION
};

extern volatile bool remoteExited;

//Prototypes
void remoteSystemInit();
void remoteSystemExec();

#endif
