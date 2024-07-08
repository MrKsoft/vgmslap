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
// OPL.C - OPL chip functions
//
///////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <conio.h>
#include <dos.h>

#include "settings.h"
#include "opl.h"
#include "ui.h"
#include "vgmslap.h"

///////////////////////////////////////////////////////////////////////////////
// Initialize variables
///////////////////////////////////////////////////////////////////////////////

uint16_t oplBaseAddr;
OplDetectedType detectedChip = DETECTED_NONE;
uint8_t oplDelayReg = 6;
uint8_t oplDelayData = 35;
char oplRegisterMap[0x1FF];
char oplChangeMap[0x1FF];
uint8_t commandReg = 0;
uint8_t commandData = 0;
uint8_t maxChannels = 9;

const uint16_t oplOperatorOrder[] = {
	0x00, 0x03,    // Channel 1 (OPL2)
	0x01, 0x04,    // Channel 2
	0x02, 0x05,    // Channel 3
	0x08, 0x0B,    // Channel 4
	0x09, 0x0C,    // Channel 5
	0x0A, 0x0D,    // Channel 6
	0x10, 0x13,    // Channel 7
	0x11, 0x14,    // Channel 8
	0x12, 0x15,    // Channel 9
	0x100, 0x103,  // Channel 10 (OPL3)
	0x101, 0x104,  // Channel 11
	0x102, 0x105,  // Channel 12
	0x108, 0x10B,  // Channel 13
	0x109, 0x10C,  // Channel 14
	0x10A, 0x10D,  // Channel 15
	0x110, 0x113,  // Channel 16
	0x111, 0x114,  // Channel 17
	0x112, 0x115   // Channel 18
};

const uint8_t oplOperatorToChannel[] = {
0, 1, 2, 0, 1, 2, 0, 0, 3, 4, 5, 3, 4, 5, 0, 0, 6, 7, 8, 6, 7, 8
};
const uint8_t oplOffsetToOperator[] = {
0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 1, 1, 1
};

oplChip oplStatus;

///////////////////////////////////////////////////////////////////////////////
// Functions
///////////////////////////////////////////////////////////////////////////////

void detectOPL(void)
{
	uint8_t statusRegisterResult1;
	uint8_t statusRegisterResult2;

	// Start with assuming nothing is detected
	detectedChip = DETECTED_NONE;

	// Detect OPL2

	// Reset timer 1 and timer 2
	writeOPL(0x04, 0x60);
	// Reset IRQ
	writeOPL(0x04, 0x80);
	// Read status register
	statusRegisterResult1 = inp(oplBaseAddr);
	// Set timer 1
	writeOPL(0x02, 0xFF);
	// Unmask and start timer 1
	writeOPL(0x04, 0x21);
	// Wait at least 80 usec (0.08ms) - a 2ms delay should be enough
	delay(2);
	// Read status register
	statusRegisterResult2 = inp(oplBaseAddr);
	// Reset timer 1, timer 2, and IRQ again
	writeOPL(0x04, 0x60);
	writeOPL(0x04, 0x80);
	// Compare results of the status register reads.  First should be 0x00, second should be 0xC0.  Results are AND with 0xE0 because of unused bits in the chip
	if ((statusRegisterResult1 & 0xE0) == 0x00 && (statusRegisterResult2 & 0xE0) == 0xC0)
	{
		// OPL2 detected
		detectedChip = DETECTED_OPL2;
		// Continue to try detecting OPL3
		if ((statusRegisterResult2 & 0x06) == 0x00)
		{
			detectedChip = DETECTED_OPL3;
		}
		// If no OPL3, try detecting second OPL2 at OPL3 registers.
		// Dual OPL2 usually is at 220h/222h (left/right) and writes to 228 will go to both OPLs
		// Note that this detection works in Dosbox, but not in 86Box with Nuked.  It does work in 86box with YMFM.  Need to test on a real SB Pro 1 which I don't have.
		else
		{
			writeOPL(0x104, 0x60);
			writeOPL(0x104, 0x80);
			statusRegisterResult1 = inp(oplBaseAddr+2);
			writeOPL(0x102, 0xFF);
			writeOPL(0x104, 0x21);
			delay(2);
			statusRegisterResult2 = inp(oplBaseAddr+2);
			writeOPL(0x104, 0x60);
			writeOPL(0x104, 0x80);
			if ((statusRegisterResult1 & 0xE0) == 0x00 && (statusRegisterResult2 & 0xE0) == 0xC0)
			{
				detectedChip = DETECTED_DUAL_OPL2;
			}
		}
	}
	// Set delays, print results
	switch (detectedChip)
	{
		case DETECTED_NONE:
			killProgram(ERROR_OPL_DETECTION_FAILED);
			break;
		case DETECTED_OPL2:
			oplDelayReg = 6;
			oplDelayData = 35;
			printf("OPL2 detected at %Xh!\n", settings.oplBase);
			break;
		case DETECTED_DUAL_OPL2:
			oplDelayReg = 6;
			oplDelayData = 35;
			printf("Dual OPL2 detected at %Xh!\n", settings.oplBase);
			break;
		case DETECTED_OPL3:
			oplDelayReg = 3;
			oplDelayData = 3;
			printf("OPL3 detected at %Xh!\n", settings.oplBase);
			break;

	}
	sleep(1);
}

void resetOPL(void)
{
		// Resetting the OPL has to be somewhat systematic - otherwise you run into issues with static sounds, squeaking, etc, not only when cutting off the sound but also when the sound starts back up again.

		uint16_t i;

		// For OPL3, turn on the NEW bit.  This ensures we can write to ALL registers on an OPL3.
		if (detectedChip == DETECTED_OPL3)
		{
			writeOPL(0x105,0x01);
		}

		// Clear out the channel level information

		// OPL2
		for (i=0; i<9; i++)
		{
			writeOPL(0xA0+i, 0x00); // Frequency number (LSB)
			writeOPL(0xB0+i, 0x00); // Key-On + Block + Frequency (MSB)
			writeOPL(0xC0+i, 0x30); // Panning, Feedback, Synthesis Type
		}
		// Dual OPL2 / OPL3
		if (detectedChip == DETECTED_DUAL_OPL2 || detectedChip == DETECTED_OPL3)
		{
			for (i=0; i<9; i++)
			{
				writeOPL(0x1A0+i, 0x00); // Frequency number (LSB)
				writeOPL(0x1B0+i, 0x00); // Key-On + Block + Frequency (MSB)
				writeOPL(0x1C0+i, 0x30); // Panning, Feedback, Synthesis Type
			}
		}

		// Clear out the operators one by one.
		// Why do it this way instead of just writing 0's in order?
		// That would mean that we go through each operator property, by operator, in order, putting a gap between the changes for an operator that can manifest as an audio glitch or "click"
		// Doing it this way we don't get that clicking when we kill the audio.

		// OPL2
		for (i=0; i<18; i++)
		{
			writeOPL(0x20+oplOperatorOrder[i], 0x00); // Tremolo / Vibrato / Sustain / KSR / Multiplier
			writeOPL(0x40+oplOperatorOrder[i], 0x3F); // Output attenuation is set to max
			writeOPL(0x60+oplOperatorOrder[i], 0xFF); // Attack / Decay - Set to "max" to force note decay
			writeOPL(0x80+oplOperatorOrder[i], 0xFF); // Sustain / Relase - Set to "max" to force note decay
			writeOPL(0xE0+oplOperatorOrder[i], 0x00); // Waveform Select
		}
		// Dual OPL2 / OPL3
		if (detectedChip == DETECTED_DUAL_OPL2 || detectedChip == DETECTED_OPL3)
		{
			for (i=18; i<36; i++)
			{
				writeOPL(0x20+oplOperatorOrder[i], 0x00); // Tremolo / Vibrato / Sustain / KSR / Multiplier
				writeOPL(0x40+oplOperatorOrder[i], 0x3F); // Output attenuation is set to max
				writeOPL(0x60+oplOperatorOrder[i], 0xFF); // Attack / Decay - Set to "max" to force note decay
				writeOPL(0x80+oplOperatorOrder[i], 0xFF); // Sustain / Relase - Set to "max" to force note decay
				writeOPL(0xE0+oplOperatorOrder[i], 0x00); // Waveform Select
			}
		}

		// Clear out percussion mode register
		writeOPL(0xBD,0x00);
		if (detectedChip == DETECTED_DUAL_OPL2 || detectedChip == DETECTED_OPL3)
		{
			writeOPL(0x1BD,0x00);
		}

		// Return to the ADSR and set them to zero - we set them to F earlier to force a note decay
		// We also set the output attenuation AGAIN, this time to 0 (no attenuation) as some poorly coded playback engines just expect the OPL to already have 0 there
		// OPL2
		for (i=0; i<18; i++)
		{
			writeOPL(0x60+oplOperatorOrder[i], 0x00); // Attack / Decay
			writeOPL(0x80+oplOperatorOrder[i], 0x00); // Sustain / Release
			writeOPL(0x40+oplOperatorOrder[i], 0x00); // Key Scale / Output Level
		}
		// Dual OPL2 / OPL3
		if (detectedChip == DETECTED_DUAL_OPL2 || detectedChip == DETECTED_OPL3)
		{
			for (i=18; i<36; i++)
			{
				writeOPL(0x60+oplOperatorOrder[i], 0x00); // Attack / Decay
				writeOPL(0x80+oplOperatorOrder[i], 0x00); // Sustain / Release
				writeOPL(0x40+oplOperatorOrder[i], 0x00); // Key Scale / Output Level
			}
		}

		// Finally clear the initial registers
		// OPL2 regs
		for (i = 0x00; i < 0x20; i++)
		{
			writeOPL(i,0x00);
		}

		// If Dual OPL2 just clear it like an OPL2
		if (detectedChip == DETECTED_DUAL_OPL2)
		{
			for (i = 0x100; i < 0x120; i++)
			{
				writeOPL(i,0x00);
			}
		}

		// If OPL3, change plans...
		if (detectedChip == DETECTED_OPL3)
		{
			// OPL3 regs - works a bit differently.  We don't turn off 4-op mode until we have zeroed everything else out, and we must touch 0x105 (OPL3 enable / "NEW" bit) ABSOLUTELY LAST or our writes to OPL3 features will be completely ignored!  (Yes, that includes zeroing them out!!)
			for (i = 0x100; i <= 0x103; i++)
			{
				writeOPL(i,0x00);
			}
			for (i = 0x106; i < 0x120; i++)
			{
				writeOPL(i,0x00);
			}
			// For OPL3, turn off 4-Op mode (if it was on)
			writeOPL(0x104,0x00);
			// For OPL3, write the NEW bit back to 0.  We're now back in OPL2 mode.
			// VGMs should have their own write to this bit to re-enable it for OPL3 songs.
			writeOPL(0x105,0x00);
		}
}

void writeOPL(uint16_t reg, uint8_t data)
{
		// Setup delay count
		uint8_t registerDelay = oplDelayReg;
		uint8_t dataDelay = oplDelayData;
		
		// Second OPL2 and/or OPL3 secondary register set
		if (reg >= 0x100)
		{
			// First write to target register... 
			outp(oplBaseAddr+2, (reg - 0x100));
			// Index register write delay.
			// The OPL2 requires a minimum time between writes.  We can execute inp a certain number of times to ensure enough time has passed - why does that work?  Because the time it takes to complete an inp is based on the ISA bus timings!
			while (registerDelay--)
			{
				inp(0x80);
			}
			
			// ...then go to +1 for the data
			outp(oplBaseAddr+3, data);
			// Data register write delay.
			while (dataDelay--)
			{
				inp(0x80);
			}
		}
		// OPL2 and/or OPL3 primary register set
		else
		{
			// First write to target register... 
			outp(oplBaseAddr, reg);
			// Index register write delay.
			// The OPL2 requires a minimum time between writes.  We can execute inp a certain number of times to ensure enough time has passed - why does that work?  Because the time it takes to complete an inp is based on the ISA bus timings!
			while (registerDelay--)
			{
				inp(0x80);
			}
			
			// ...then go to Base+1 for the data
			outp(oplBaseAddr+1, data);
			// Data register write delay.
			while (dataDelay--)
			{
				inp(0x80);
			}
		}
		
		// Write the same data to our "register map", used for visualizing the OPL state, as well as the change map to denote that this bit needs to be interpreted and potentially drawn.
		oplRegisterMap[reg] = data;
		oplChangeMap[reg] = 1;
		
		// Request a screen draw for the display update
		requestScreenDraw = 1;
}
