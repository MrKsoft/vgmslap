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
// TIMER.H - Functions for interrupt timer control
//
///////////////////////////////////////////////////////////////////////////////

#ifndef VGMSLAP_TIMER_H
#define VGMSLAP_TIMER_H

#include "types.h"

#define VGA_REFRESH_TICKS 1260 // Using 44100hz base rate, 1260 ticks = 35hz

///////////////////////////////////////////////////////////////////////////////
// Function declarations
///////////////////////////////////////////////////////////////////////////////

void initTimer(uint16_t frequency);	// Reprogram the PIT to run at our desired playback rate and insert our interrupt service routine 8 handler.
void resetTimer (void);				// Restore PIT to its original rate and remove our interrupt service routine 8 handler.
void interrupt timerHandler(void);	// Handler executes every PIT frequency cycle (ISR8)

///////////////////////////////////////////////////////////////////////////////
// Variable declarations
///////////////////////////////////////////////////////////////////////////////

extern void interrupt (*biosISR8)(void);	// Pointer to the BIOS interrupt service routine 8
extern volatile uint32_t tickCounter;		// Counts the number of timer ticks elapsed
extern volatile uint32_t screenCounter;		// Tracks 70hz screen refresh
extern uint16_t biosCounter;				// Used to determine when to run the original BIOS ISR8
extern uint32_t fastTickRate;				// Divider to apply to the 8253 PIT
extern const uint16_t playbackFrequency;	// Playback frequency (VGM files are set to 44100 Hz)
extern uint8_t playbackFrequencyDivider;	// Performance hack available for slower machines

#endif
