#ifndef DEBUG_H
#define DEBUG_H

#include "globals.h"
#include <avr/pgmspace.h>

//Prototypes
void debugInit();
void logDebug(const char *messageFormat, ...);
void logWarning(const char *messageFormat, ...);
void logCritical(const char *messageFormat, ...);
void logSoftwareFault(const char *filename, u16 lineNumber, const char *message, u16 arg1, u16 arg2);

//! Macro to log/print a software fault. It stores its strings in flash instead of SRAM to save memory.
#define SOFTWARE_FAULT(msg, arg1, arg2) logSoftwareFault(PSTR(__FILE__), __LINE__, (msg), (arg1), (arg2))

#endif
