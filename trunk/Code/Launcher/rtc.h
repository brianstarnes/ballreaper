#include "globals.h"

//Elapsed seconds
extern volatile u08 secCount;

//Prototypes
void rtcInit();
void rtcRestart();
void rtcPause();
void rtcResume();
u32 getMsCount();
