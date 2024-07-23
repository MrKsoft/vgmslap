///////////////////////////////////////////////////////////////////////////////
// __      _______ __  __  _____ _             _ 
// \ \    / / ____|  \/  |/ ____| |           | |
//  \ \  / / |  __| \  / | (___ | | __ _ _ __ | |
//   \ \/ /| | |_ | |\/| |\___ \| |/ _` | '_ \| |         by Wafflenet
//    \  / | |__| | |  | |____) | | (_| | |_) |_|       www.wafflenet.com
//     \/   \_____|_|  |_|_____/|_|\__,_| .__/(_)
//      (VGM Silly Little AdLib Player) | |      
//                                      |_|      
//
///////////////////////////////////////////////////////////////////////////////
//
// TIMER.C - Functions for interrupt timer control
//
///////////////////////////////////////////////////////////////////////////////

#include <bios.h>
#include <conio.h>
#include <dos.h>

#include "timer.h"

///////////////////////////////////////////////////////////////////////////////
// Initialize variables
///////////////////////////////////////////////////////////////////////////////

void interrupt (*biosISR8)(void);
volatile uint32_t tickCounter = 0;
volatile uint32_t screenCounter = 0;
uint16_t biosCounter = 0;
uint32_t fastTickRate;
const uint16_t playbackFrequency = 44100;
uint8_t playbackFrequencyDivider = 1;

///////////////////////////////////////////////////////////////////////////////
// Functions
///////////////////////////////////////////////////////////////////////////////

void initTimer(uint16_t frequency)
{
	// Divide the PIT's hz rate by the target hz rate
	fastTickRate = 1193182 / (frequency/playbackFrequencyDivider);
	// Store pointer to the original BIOS ISR8
	biosISR8 = _dos_getvect(8);
	// Hijack ISR8 to point to our handler routine instead
	_dos_setvect(8, timerHandler);
	// Configure and set the timer, which sits at 40h-43h.
	// Let's break down these magic-looking values.
	//
	// 43h = Command to set a channel's mode of operation
	// 36h = Command data:
	//       0 0 1 1 0 1 1 0
	//       └┬┘ └┬┘ └─┬─┘ └─► Process count as binary
	//        │   │    └─────► Mode 3: square-wave rate generator
	//        │   └──────────► Read/load LSB then MSB
	//        └──────────────► Select counter #0
	outp(0x43, 0x36);
	// 40h = Frequency divider for counter #0
	outp(0x40, fastTickRate & 0xFF); // Low byte
	outp(0x40, fastTickRate >> 8); // High byte
}

void resetTimer(void)
{
	// Configure and set the timer, which sits at 40h-43h.
	outp(0x43, 0x36);
	// 40h = Frequency divider for counter #0 - Setting to zero puts us back to the default rate
	outp(0x40, 0x00);
	outp(0x40, 0x00);
	// Point ISR8 back to the original BIOS routine
	_dos_setvect(8, biosISR8);
}

void interrupt timerHandler(void)
{
	// Increment the counter cause the interrupt has happened
	tickCounter=tickCounter+playbackFrequencyDivider;
	screenCounter=screenCounter+playbackFrequencyDivider;
	
	// Since we have changed the timer rate, we have to determine when to call the BIOS ISR8 at its old rate.  This avoids the clock getting messed up.  While it may appear that biosCounter will only be less than fastTickRate one time, it will actually overflow back to 0 over time, making this a constant cycle.
	biosCounter += fastTickRate;
	if (biosCounter < fastTickRate)
	{
		biosISR8();
	}
	else
	{
		// If we aren't calling the BIOS ISR8 routine, we have to manually send an EOI (End of Interrupt) like it usually does.
		outp(0x20, 0x20);
	}
}
