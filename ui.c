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
// UI.C - User interface control
//
///////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <conio.h>
#include <libgen.h>

#include "opl.h"
#include "playlist.h"
#include "settings.h"
#include "txtgfx.h"
#include "txtmode.h"
#include "ui.h"
#include "vgm.h"
#include "vgmslap.h"

///////////////////////////////////////////////////////////////////////////////
// Initialize variables
///////////////////////////////////////////////////////////////////////////////

uint8_t keyboardCurrent = 0;
uint8_t keyboardPrevious = 0;
uint8_t keyboardExtendedFlag = 0;
uint8_t requestScreenDraw;

///////////////////////////////////////////////////////////////////////////////
// Functions
///////////////////////////////////////////////////////////////////////////////

void clearInterface(void)
{
	uint8_t column;
	uint8_t row;

	// Clear GD3 tag area
	for (row = GD3_START_Y; row < GD3_START_Y+4; row++)
	{
		for (column = GD3_TAG_START_X; column < 80; column++)
		{
		drawCharacterAtPosition(' ',column,row,COLOR_LIGHTGREY,COLOR_BLACK);
		}
	}


	// Clear channel table (only right side, cause of leftover DualOPL2/OPL3 stuff)

	for (row = CHAN_TABLE_START_Y; row < textRows; row++)
	{
		for (column = (CHAN_TABLE_START_X+40); column < 80; column++)
		{
		drawCharacterAtPosition(' ',column,row,COLOR_LIGHTGREY,COLOR_BLACK);
		}
	}
}

void drawChannelTable(void)
{
	uint16_t i;
	uint16_t j;
	uint16_t k;
	uint8_t targetChannel;
	uint8_t targetOperator;
	uint8_t tempAttribute = 0x0;

	// Start at 0x20 as that is the lowest register we care about
	for (i = 20; i <= displayRegisterMax; i++)
	{
		// Found a changed register
		if (oplChangeMap[i] == 1)
		{
			// What register changed?

			// Tremolo / Vibrato / Sustain / KSR Flags & Multiplier
			if ((i >= 0x20 && i <= 0x35) || (i >= 0x120 && i <= 0x135))
			{
				if (i >= 0x20 && i <= 0x35)
				{
					targetChannel = oplOperatorToChannel[(i - 0x20)];
					targetOperator = oplOffsetToOperator[(i - 0x20)];
				}
				else if (i >= 0x120 && i <= 0x135)
				{
					targetChannel = oplOperatorToChannel[(i - 0x120)] + 9;
					targetOperator = oplOffsetToOperator[(i - 0x120)];
				}

				// Multiplier
				oplStatus.channels[targetChannel].operators[targetOperator].frequencyMultiplierFactor = (oplRegisterMap[i] & 0x0F);

				// Tremolo
				oplStatus.channels[targetChannel].operators[targetOperator].flagTremolo = ((oplRegisterMap[i] >> 7) & 0x01);

				// Vibrato
				oplStatus.channels[targetChannel].operators[targetOperator].flagFrequencyVibrato = ((oplRegisterMap[i] >> 6) & 0x01);

				// Sustain
				oplStatus.channels[targetChannel].operators[targetOperator].flagSoundSustaining = ((oplRegisterMap[i] >> 5) & 0x01);

				// KSR
				oplStatus.channels[targetChannel].operators[targetOperator].flagKSR = ((oplRegisterMap[i] >> 4) & 0x01);

				if (targetOperator == 0)
				{
					// Multiplier
					drawStringAtPosition(oplMultiplierNames[oplStatus.channels[targetChannel].operators[targetOperator].frequencyMultiplierFactor], oplStatus.channels[targetChannel].displayX+CHAN_DISP_OFFSET_OPERATOR_PARAMETERS+15, oplStatus.channels[targetChannel].displayY+1,COLOR_LIGHTCYAN,COLOR_BLACK);

					// Tremolo
					if (oplStatus.channels[targetChannel].operators[targetOperator].flagTremolo == 1)
					{
						drawCharacterAtPosition('T', oplStatus.channels[targetChannel].displayX+CHAN_DISP_OFFSET_OPERATOR_PARAMETERS+10, oplStatus.channels[targetChannel].displayY+1, COLOR_LIGHTGREY, COLOR_BLACK);
					}
					else
					{
						drawCharacterAtPosition('-', oplStatus.channels[targetChannel].displayX+CHAN_DISP_OFFSET_OPERATOR_PARAMETERS+10, oplStatus.channels[targetChannel].displayY+1, COLOR_DARKGREY, COLOR_BLACK);
					}

					// Vibrato
					if (oplStatus.channels[targetChannel].operators[targetOperator].flagFrequencyVibrato == 1)
					{
						drawCharacterAtPosition('V', oplStatus.channels[targetChannel].displayX+CHAN_DISP_OFFSET_OPERATOR_PARAMETERS+11, oplStatus.channels[targetChannel].displayY+1, COLOR_LIGHTGREY, COLOR_BLACK);
					}
					else
					{
						drawCharacterAtPosition('-', oplStatus.channels[targetChannel].displayX+CHAN_DISP_OFFSET_OPERATOR_PARAMETERS+11, oplStatus.channels[targetChannel].displayY+1, COLOR_DARKGREY, COLOR_BLACK);
					}

					// Sustain
					if (oplStatus.channels[targetChannel].operators[targetOperator].flagSoundSustaining == 1)
					{
						drawCharacterAtPosition('S', oplStatus.channels[targetChannel].displayX+CHAN_DISP_OFFSET_OPERATOR_PARAMETERS+12, oplStatus.channels[targetChannel].displayY+1, COLOR_LIGHTGREY, COLOR_BLACK);
					}
					else
					{
						drawCharacterAtPosition('-', oplStatus.channels[targetChannel].displayX+CHAN_DISP_OFFSET_OPERATOR_PARAMETERS+12, oplStatus.channels[targetChannel].displayY+1, COLOR_DARKGREY, COLOR_BLACK);
					}

					// KSR
					if (oplStatus.channels[targetChannel].operators[targetOperator].flagKSR == 1)
					{
						drawCharacterAtPosition('K', oplStatus.channels[targetChannel].displayX+CHAN_DISP_OFFSET_OPERATOR_PARAMETERS+13, oplStatus.channels[targetChannel].displayY+1, COLOR_LIGHTGREY, COLOR_BLACK);
					}
					else
					{
						drawCharacterAtPosition('-', oplStatus.channels[targetChannel].displayX+CHAN_DISP_OFFSET_OPERATOR_PARAMETERS+13, oplStatus.channels[targetChannel].displayY+1, COLOR_DARKGREY, COLOR_BLACK);
					}
				}
				else if (targetOperator == 1)
				{
					// Multiplier
					drawStringAtPosition(oplMultiplierNames[oplStatus.channels[targetChannel].operators[targetOperator].frequencyMultiplierFactor],oplStatus.channels[targetChannel].displayX+CHAN_DISP_OFFSET_OPERATOR_PARAMETERS+15,oplStatus.channels[targetChannel].displayY+3,COLOR_LIGHTGREEN, COLOR_BLACK);

					// Tremolo
					if (oplStatus.channels[targetChannel].operators[targetOperator].flagTremolo == 1)
					{
						drawCharacterAtPosition('T', oplStatus.channels[targetChannel].displayX+CHAN_DISP_OFFSET_OPERATOR_PARAMETERS+10, oplStatus.channels[targetChannel].displayY+3, COLOR_LIGHTGREY, COLOR_BLACK);
					}
					else
					{
						drawCharacterAtPosition('-', oplStatus.channels[targetChannel].displayX+CHAN_DISP_OFFSET_OPERATOR_PARAMETERS+10, oplStatus.channels[targetChannel].displayY+3, COLOR_DARKGREY, COLOR_BLACK);
					}

					// Vibrato
					if (oplStatus.channels[targetChannel].operators[targetOperator].flagFrequencyVibrato == 1)
					{
						drawCharacterAtPosition('V', oplStatus.channels[targetChannel].displayX+CHAN_DISP_OFFSET_OPERATOR_PARAMETERS+11, oplStatus.channels[targetChannel].displayY+3, COLOR_LIGHTGREY, COLOR_BLACK);
					}
					else
					{
						drawCharacterAtPosition('-', oplStatus.channels[targetChannel].displayX+CHAN_DISP_OFFSET_OPERATOR_PARAMETERS+11, oplStatus.channels[targetChannel].displayY+3, COLOR_DARKGREY, COLOR_BLACK);
					}

					// Sustain
					if (oplStatus.channels[targetChannel].operators[targetOperator].flagSoundSustaining == 1)
					{
						drawCharacterAtPosition('S', oplStatus.channels[targetChannel].displayX+CHAN_DISP_OFFSET_OPERATOR_PARAMETERS+12, oplStatus.channels[targetChannel].displayY+3, COLOR_LIGHTGREY, COLOR_BLACK);
					}
					else
					{
						drawCharacterAtPosition('-', oplStatus.channels[targetChannel].displayX+CHAN_DISP_OFFSET_OPERATOR_PARAMETERS+12, oplStatus.channels[targetChannel].displayY+3, COLOR_DARKGREY, COLOR_BLACK);
					}

					// KSR
					if (oplStatus.channels[targetChannel].operators[targetOperator].flagKSR == 1)
					{
						drawCharacterAtPosition('K', oplStatus.channels[targetChannel].displayX+CHAN_DISP_OFFSET_OPERATOR_PARAMETERS+13, oplStatus.channels[targetChannel].displayY+3, COLOR_LIGHTGREY, COLOR_BLACK);
					}
					else
					{
						drawCharacterAtPosition('-', oplStatus.channels[targetChannel].displayX+CHAN_DISP_OFFSET_OPERATOR_PARAMETERS+13, oplStatus.channels[targetChannel].displayY+3, COLOR_DARKGREY, COLOR_BLACK);
					}
				}
			}

			// KSL / Output Level
			else if ((i >= 0x40 && i <= 0x55) || (i >= 0x140 && i <= 0x155))
			{
				if (i >= 0x40 && i <= 0x55)
				{
					targetChannel = oplOperatorToChannel[(i - 0x40)];
					targetOperator = oplOffsetToOperator[(i - 0x40)];
				}
				else if (i >= 0x140 && i <= 0x155)
				{
					targetChannel = oplOperatorToChannel[(i - 0x140)] + 9;
					targetOperator = oplOffsetToOperator[(i - 0x140)];
				}

				// Key Scaling Level
				oplStatus.channels[targetChannel].operators[targetOperator].keyScaleLevel = (oplRegisterMap[i] >> 6);

				// Output level
				oplStatus.channels[targetChannel].operators[targetOperator].outputLevel = (oplRegisterMap[i] & 0x3F);

				if (targetOperator == 0)
				{
					// Output level
					sprintf(txtDrawBuffer, "%.2X", oplStatus.channels[targetChannel].operators[targetOperator].outputLevel);
					drawStringAtPosition(txtDrawBuffer,oplStatus.channels[targetChannel].displayX+CHAN_DISP_OFFSET_OPERATOR_PARAMETERS+21,oplStatus.channels[targetChannel].displayY+1,COLOR_LIGHTCYAN, COLOR_BLACK);

					// Key Scaling Level
					drawStringAtPosition(oplKSLNames[oplStatus.channels[targetChannel].operators[targetOperator].keyScaleLevel],oplStatus.channels[targetChannel].displayX+CHAN_DISP_OFFSET_OPERATOR_PARAMETERS+17,oplStatus.channels[targetChannel].displayY+1,COLOR_LIGHTCYAN, COLOR_BLACK);

				}
				else if (targetOperator == 1)
				{
					// Output level
					sprintf(txtDrawBuffer, "%.2X", oplStatus.channels[targetChannel].operators[targetOperator].outputLevel);
					drawStringAtPosition(txtDrawBuffer,oplStatus.channels[targetChannel].displayX+CHAN_DISP_OFFSET_OPERATOR_PARAMETERS+21,oplStatus.channels[targetChannel].displayY+3,COLOR_LIGHTGREEN, COLOR_BLACK);

					// Key Scaling Level
					drawStringAtPosition(oplKSLNames[oplStatus.channels[targetChannel].operators[targetOperator].keyScaleLevel],oplStatus.channels[targetChannel].displayX+CHAN_DISP_OFFSET_OPERATOR_PARAMETERS+17,oplStatus.channels[targetChannel].displayY+3,COLOR_LIGHTGREEN, COLOR_BLACK);
				}
			}

			// Attack/Decay
			else if ((i >= 0x60 && i <= 0x75) || (i >= 0x160 && i <= 0x175))
			{
				if (i >= 0x60 && i <= 0x75)
				{
					targetChannel = oplOperatorToChannel[(i - 0x60)];
					targetOperator = oplOffsetToOperator[(i - 0x60)];
				}
				else if (i >= 0x160 && i <= 0x175)
				{
					targetChannel = oplOperatorToChannel[(i - 0x160)] + 9;
					targetOperator = oplOffsetToOperator[(i - 0x160)];
				}

				oplStatus.channels[targetChannel].operators[targetOperator].attackRate = (oplRegisterMap[i] >> 4);
				oplStatus.channels[targetChannel].operators[targetOperator].decayRate = (oplRegisterMap[i] & 0x0F);

				if (targetOperator == 0)
				{
					drawCharacterAtPosition(numToHex[oplStatus.channels[targetChannel].operators[targetOperator].attackRate],oplStatus.channels[targetChannel].displayX+CHAN_DISP_OFFSET_OPERATOR_PARAMETERS+5,oplStatus.channels[targetChannel].displayY+1, COLOR_LIGHTCYAN, COLOR_BLACK);
					drawCharacterAtPosition(numToHex[oplStatus.channels[targetChannel].operators[targetOperator].decayRate],oplStatus.channels[targetChannel].displayX+CHAN_DISP_OFFSET_OPERATOR_PARAMETERS+6,oplStatus.channels[targetChannel].displayY+1,COLOR_LIGHTCYAN, COLOR_BLACK);
				}
				else if (targetOperator == 1)
				{
					drawCharacterAtPosition(numToHex[oplStatus.channels[targetChannel].operators[targetOperator].attackRate],oplStatus.channels[targetChannel].displayX+CHAN_DISP_OFFSET_OPERATOR_PARAMETERS+5,oplStatus.channels[targetChannel].displayY+3, COLOR_LIGHTGREEN, COLOR_BLACK);
					drawCharacterAtPosition(numToHex[oplStatus.channels[targetChannel].operators[targetOperator].decayRate],oplStatus.channels[targetChannel].displayX+CHAN_DISP_OFFSET_OPERATOR_PARAMETERS+6,oplStatus.channels[targetChannel].displayY+3, COLOR_LIGHTGREEN, COLOR_BLACK);
				}
			}

			// Sustain/Release
			else if ((i >= 0x80 && i <= 0x95) || (i >= 0x180 && i <= 0x195))
			{
				if (i >= 0x80 && i <= 0x95)
				{
					targetChannel = oplOperatorToChannel[(i - 0x80)];
					targetOperator = oplOffsetToOperator[(i - 0x80)];
				}
				else if (i >= 0x180 && i <= 0x195)
				{
					targetChannel = oplOperatorToChannel[(i - 0x180)] + 9;
					targetOperator = oplOffsetToOperator[(i - 0x180)];
				}

				oplStatus.channels[targetChannel].operators[targetOperator].sustainLevel = (oplRegisterMap[i] >> 4);
				oplStatus.channels[targetChannel].operators[targetOperator].releaseRate = (oplRegisterMap[i] & 0x0F);

				if (targetOperator == 0)
				{
					drawCharacterAtPosition(numToHex[oplStatus.channels[targetChannel].operators[targetOperator].sustainLevel],oplStatus.channels[targetChannel].displayX+CHAN_DISP_OFFSET_OPERATOR_PARAMETERS+7,oplStatus.channels[targetChannel].displayY+1, COLOR_LIGHTCYAN, COLOR_BLACK);
					drawCharacterAtPosition(numToHex[oplStatus.channels[targetChannel].operators[targetOperator].releaseRate],oplStatus.channels[targetChannel].displayX+CHAN_DISP_OFFSET_OPERATOR_PARAMETERS+8,oplStatus.channels[targetChannel].displayY+1, COLOR_LIGHTCYAN, COLOR_BLACK);
				}
				else if (targetOperator == 1)
				{
					drawCharacterAtPosition(numToHex[oplStatus.channels[targetChannel].operators[targetOperator].sustainLevel],oplStatus.channels[targetChannel].displayX+CHAN_DISP_OFFSET_OPERATOR_PARAMETERS+7,oplStatus.channels[targetChannel].displayY+3, COLOR_LIGHTGREEN, COLOR_BLACK);
					drawCharacterAtPosition(numToHex[oplStatus.channels[targetChannel].operators[targetOperator].releaseRate],oplStatus.channels[targetChannel].displayX+CHAN_DISP_OFFSET_OPERATOR_PARAMETERS+8,oplStatus.channels[targetChannel].displayY+3, COLOR_LIGHTGREEN, COLOR_BLACK);
				}
			}

			// Frequency (Low)

			else if ((i >= 0xA0 && i <= 0xA8) || (i >= 0x1A0 && i <= 0x1A8))
			{
				if (i >= 0xA0 && i <= 0xA8)
				{
					targetChannel = i - 0xA0;
				}
				else if (i >= 0x1A0 && i <= 0x1A8)
				{
					targetChannel = (i - 0x1A0) + 9;
				}

				oplStatus.channels[targetChannel].frequencyNumber = (oplRegisterMap[i]) + ((oplRegisterMap[i+0x10] & 0x03) << 8);

				// Entire frequency must be redrawn because it's split between bytes
				if (targetChannel <= 5 || (targetChannel >= 9 && targetChannel <= 14))
				{
					if (oplStatus.channels[targetChannel].flag4Op == 1)
					{
						if (targetChannel <= 2 || (targetChannel >= 9 && targetChannel <= 11))
						{
							// Frequency number
							sprintf(txtDrawBuffer, "%.3X", oplStatus.channels[targetChannel].frequencyNumber);
							drawStringAtPosition(txtDrawBuffer,oplStatus.channels[targetChannel].displayX+CHAN_DISP_OFFSET_CHANNEL_NOTEINFO,oplStatus.channels[targetChannel].displayY+4,COLOR_YELLOW,COLOR_BLACK);
						}
					}
					else
					{
						// Frequency number
						sprintf(txtDrawBuffer, "%.3X", oplStatus.channels[targetChannel].frequencyNumber);
						drawStringAtPosition(txtDrawBuffer,oplStatus.channels[targetChannel].displayX+CHAN_DISP_OFFSET_CHANNEL_NOTEINFO,oplStatus.channels[targetChannel].displayY+2,COLOR_YELLOW,COLOR_BLACK);
					}
				}
				// Positioning for 2-op channels
				else
				{
					// Frequency number
					sprintf(txtDrawBuffer, "%.3X", oplStatus.channels[targetChannel].frequencyNumber);
					drawStringAtPosition(txtDrawBuffer,oplStatus.channels[targetChannel].displayX+CHAN_DISP_OFFSET_CHANNEL_NOTEINFO,oplStatus.channels[targetChannel].displayY+2,COLOR_YELLOW,COLOR_BLACK);
				}
			}

			// Key-On, Block, Frequency (High)

			else if ((i >= 0xB0 && i <= 0xB8) || (i >= 0x1B0 && i <= 0x1B8))
			{
				if (i >= 0xB0 && i <= 0xB8)
				{
					targetChannel = i - 0xB0;
				}
				else if (i >= 0x1B0 && i <= 0x1B8)
				{
					targetChannel = (i - 0x1B0) + 9;
				}

				// Frequency (High)
				oplStatus.channels[targetChannel].frequencyNumber = (oplRegisterMap[i-0x10]) + ((oplRegisterMap[i] & 0x03) << 8);

				// Block number
				oplStatus.channels[targetChannel].blockNumber = ((oplRegisterMap[i] >> 2) & 0x07);

				// Key on
				oplStatus.channels[targetChannel].keyOn = ((oplRegisterMap[i] >> 5) & 0x01);
				// Set what color to draw the Note icon with based on whether Key-On was set.
				if (oplStatus.channels[targetChannel].keyOn == 1)
				{
					tempAttribute = COLOR_LIGHTMAGENTA;
				}
				else
				{
					tempAttribute = COLOR_DARKGREY;
				}

				// Draw
				// Entire frequency must be redrawn because it's split between bytes

				if (targetChannel <= 5 || (targetChannel >= 9 && targetChannel <= 14))
				{
					if (oplStatus.channels[targetChannel].flag4Op == 1)
					{
						if (targetChannel <= 2 || (targetChannel >= 9 && targetChannel <= 11))
						{
							// Block number
							sprintf(txtDrawBuffer, " %X ", oplStatus.channels[targetChannel].blockNumber);
							drawStringAtPosition(txtDrawBuffer,oplStatus.channels[targetChannel].displayX+CHAN_DISP_OFFSET_CHANNEL_NOTEINFO,oplStatus.channels[targetChannel].displayY+3, COLOR_BROWN, COLOR_BLACK);
							// Frequency number
							sprintf(txtDrawBuffer, "%.3X", oplStatus.channels[targetChannel].frequencyNumber);
							drawStringAtPosition(txtDrawBuffer,oplStatus.channels[targetChannel].displayX+CHAN_DISP_OFFSET_CHANNEL_NOTEINFO,oplStatus.channels[targetChannel].displayY+4, COLOR_YELLOW, COLOR_BLACK);
							// Key on
							drawCharacterAtPosition(CHAR_MUSIC_NOTE, oplStatus.channels[targetChannel].displayX+CHAN_DISP_OFFSET_CHANNEL_NOTEINFO+1, oplStatus.channels[targetChannel].displayY+5, tempAttribute, COLOR_BLACK);
						}
					}
					else
					{
						// Block number
						sprintf(txtDrawBuffer, " %X ", oplStatus.channels[targetChannel].blockNumber);
						drawStringAtPosition(txtDrawBuffer,oplStatus.channels[targetChannel].displayX+CHAN_DISP_OFFSET_CHANNEL_NOTEINFO,oplStatus.channels[targetChannel].displayY+1, COLOR_BROWN, COLOR_BLACK);
						// Frequency number
						sprintf(txtDrawBuffer, "%.3X", oplStatus.channels[targetChannel].frequencyNumber);
						drawStringAtPosition(txtDrawBuffer,oplStatus.channels[targetChannel].displayX+CHAN_DISP_OFFSET_CHANNEL_NOTEINFO,oplStatus.channels[targetChannel].displayY+2, COLOR_YELLOW, COLOR_BLACK);
						// Key on
						drawCharacterAtPosition(CHAR_MUSIC_NOTE, oplStatus.channels[targetChannel].displayX+CHAN_DISP_OFFSET_CHANNEL_NOTEINFO+1, oplStatus.channels[targetChannel].displayY+3, tempAttribute, COLOR_BLACK);
					}
				}
				// Positioning for 2-op channels
				else
				{
					// Block number
					sprintf(txtDrawBuffer, " %X ", oplStatus.channels[targetChannel].blockNumber);
					drawStringAtPosition(txtDrawBuffer,oplStatus.channels[targetChannel].displayX+CHAN_DISP_OFFSET_CHANNEL_NOTEINFO,oplStatus.channels[targetChannel].displayY+1, COLOR_BROWN, COLOR_BLACK);
					// Frequency number
					sprintf(txtDrawBuffer, "%.3X", oplStatus.channels[targetChannel].frequencyNumber);
					drawStringAtPosition(txtDrawBuffer,oplStatus.channels[targetChannel].displayX+CHAN_DISP_OFFSET_CHANNEL_NOTEINFO,oplStatus.channels[targetChannel].displayY+2, COLOR_YELLOW, COLOR_BLACK);
					// Key on
					drawCharacterAtPosition(CHAR_MUSIC_NOTE, oplStatus.channels[targetChannel].displayX+CHAN_DISP_OFFSET_CHANNEL_NOTEINFO+1, oplStatus.channels[targetChannel].displayY+3, tempAttribute, COLOR_BLACK);
				}
			}

			// Panning, Feedback, Algorithm
			else if ((i >= 0xC0 && i <= 0xC8) || (i >= 0x1C0 && i <= 0x1C8))
			{
				if (i >= 0xC0 && i <= 0xC8)
				{
					targetChannel = i - 0xC0;
				}
				else if (i >= 0x1C0 && i <= 0x1C8)
				{
					targetChannel = (i - 0x1C0) + 9;
				}

				// Panning
				oplStatus.channels[targetChannel].panning = ((oplRegisterMap[i] >> 4) & 0x03);

				// Algorithm type
				oplStatus.channels[targetChannel].synthesisType = ((oplRegisterMap[i]) & 0x01);

				// Feedback
				oplStatus.channels[targetChannel].feedback = ((oplRegisterMap[i] >> 1) & 0x07);

				// Draw Panning (only if the OPL3 is turned on, or playing DualOPL2)
				if (oplStatus.flagOPL3Mode != 0 || vgmChipType == VGM_DUAL_OPL2)
				{
					if (oplStatus.channels[targetChannel].flag4Op == 0)
					{
						// Left
						if ((oplStatus.channels[targetChannel].panning & 0x01) == 1)
						{
							drawCharacterAtPosition('(', oplStatus.channels[targetChannel].displayX+CHAN_DISP_OFFSET_CHANNEL_NOTEINFO, (oplStatus.channels[targetChannel].displayY+3), COLOR_WHITE, COLOR_BLACK);
						}
						else
						{
							drawCharacterAtPosition('(', oplStatus.channels[targetChannel].displayX+CHAN_DISP_OFFSET_CHANNEL_NOTEINFO, (oplStatus.channels[targetChannel].displayY+3), COLOR_DARKGREY, COLOR_BLACK);
						}
						// Right
						if ((oplStatus.channels[targetChannel].panning & 0x02) == 2)
						{
							drawCharacterAtPosition(')', oplStatus.channels[targetChannel].displayX+CHAN_DISP_OFFSET_CHANNEL_NOTEINFO+2, (oplStatus.channels[targetChannel].displayY+3), COLOR_WHITE, COLOR_BLACK);
						}
						else
						{
							drawCharacterAtPosition(')', oplStatus.channels[targetChannel].displayX+CHAN_DISP_OFFSET_CHANNEL_NOTEINFO+2, (oplStatus.channels[targetChannel].displayY+3), COLOR_DARKGREY, COLOR_BLACK);
						}
					}
					else if (oplStatus.channels[targetChannel].flag4Op != 0)
					{
						// Positioning for 4-op channels
						if (targetChannel <= 2 || (targetChannel >= 9 && targetChannel <= 11))
						{
							// Left
							if ((oplStatus.channels[targetChannel].panning & 0x01) == 1)
							{
								drawCharacterAtPosition('(', oplStatus.channels[targetChannel].displayX+CHAN_DISP_OFFSET_CHANNEL_NOTEINFO, (oplStatus.channels[targetChannel].displayY+5), COLOR_WHITE, COLOR_BLACK);
							}
							else
							{
								drawCharacterAtPosition('(', oplStatus.channels[targetChannel].displayX+CHAN_DISP_OFFSET_CHANNEL_NOTEINFO, (oplStatus.channels[targetChannel].displayY+5), COLOR_DARKGREY, COLOR_BLACK);
							}
							// Right
							if ((oplStatus.channels[targetChannel].panning & 0x02) == 2)
							{
								drawCharacterAtPosition(')', oplStatus.channels[targetChannel].displayX+CHAN_DISP_OFFSET_CHANNEL_NOTEINFO+2, (oplStatus.channels[targetChannel].displayY+5), COLOR_WHITE, COLOR_BLACK);
							}
							else
							{
								drawCharacterAtPosition(')', oplStatus.channels[targetChannel].displayX+CHAN_DISP_OFFSET_CHANNEL_NOTEINFO+2, (oplStatus.channels[targetChannel].displayY+5), COLOR_DARKGREY, COLOR_BLACK);
							}
						}
					}
				}

				// Draw Algorithm type
				if (oplStatus.channels[targetChannel].flag4Op == 0)
				{
					// FM Algorithm
					if(oplStatus.channels[targetChannel].synthesisType == 0)
					{
						drawGraphicAtPosition(tgAlgoFM, 7, 3, oplStatus.channels[targetChannel].displayX+1, oplStatus.channels[targetChannel].displayY+1);
					}
					// AS Algorithm
					else if (oplStatus.channels[targetChannel].synthesisType == 1)
					{
						drawGraphicAtPosition(tgAlgoAS, 7, 3, oplStatus.channels[targetChannel].displayX+1, oplStatus.channels[targetChannel].displayY+1);
					}
				}
				else if (oplStatus.channels[targetChannel].flag4Op != 0)
				{
					// Only do this if we are on the first channel of the 4-op pairing
					if (targetChannel <= 2 || (targetChannel >= 9 && targetChannel <= 11))
					{
						// Clean up previously written text from switching between 2/4op
						drawStringAtPosition("   ",oplStatus.channels[targetChannel].displayX+CHAN_DISP_OFFSET_OPERATOR_PARAMETERS+24,oplStatus.channels[targetChannel].displayY+2, COLOR_BLACK, COLOR_BLACK);
						drawStringAtPosition("   ",oplStatus.channels[targetChannel].displayX+CHAN_DISP_OFFSET_OPERATOR_PARAMETERS+24,oplStatus.channels[targetChannel].displayY+6, COLOR_BLACK, COLOR_BLACK);
						drawStringAtPosition("   ",oplStatus.channels[targetChannel].displayX+CHAN_DISP_OFFSET_CHANNEL_NOTEINFO,oplStatus.channels[targetChannel].displayY+1, COLOR_BLACK, COLOR_BLACK);
						drawStringAtPosition("   ",oplStatus.channels[targetChannel].displayX+CHAN_DISP_OFFSET_CHANNEL_NOTEINFO,oplStatus.channels[targetChannel].displayY+2, COLOR_BLACK, COLOR_BLACK);
						drawStringAtPosition("   ",oplStatus.channels[targetChannel].displayX+CHAN_DISP_OFFSET_CHANNEL_NOTEINFO,oplStatus.channels[targetChannel].displayY+6, COLOR_BLACK, COLOR_BLACK);
						drawStringAtPosition("   ",oplStatus.channels[targetChannel].displayX+CHAN_DISP_OFFSET_CHANNEL_NOTEINFO,oplStatus.channels[targetChannel].displayY+7, COLOR_BLACK, COLOR_BLACK);

						// FM+FM
						if(oplStatus.channels[targetChannel].synthesisType == 0 && oplStatus.channels[targetChannel+3].synthesisType == 0 )
						{
							drawGraphicAtPosition(tgAlgoFMFM, 7, 7, oplStatus.channels[targetChannel+3].displayX+1, oplStatus.channels[targetChannel].displayY+1);
						}
						// AS+FM
						else if(oplStatus.channels[targetChannel].synthesisType == 1 && oplStatus.channels[targetChannel+3].synthesisType == 0 )
						{
							drawGraphicAtPosition(tgAlgoASFM, 7, 7, oplStatus.channels[targetChannel].displayX+1, oplStatus.channels[targetChannel].displayY+1);
						}
						// FM+AS
						else if(oplStatus.channels[targetChannel].synthesisType == 0 && oplStatus.channels[targetChannel+3].synthesisType == 1 )
						{
							drawGraphicAtPosition(tgAlgoFMAS, 7, 7, oplStatus.channels[targetChannel].displayX+1, oplStatus.channels[targetChannel].displayY+1);
						}
						// AS+AS
						else if(oplStatus.channels[targetChannel].synthesisType == 1 && oplStatus.channels[targetChannel+3].synthesisType == 1 )
						{
							drawGraphicAtPosition(tgAlgoASAS, 7, 7, oplStatus.channels[targetChannel].displayX+1, oplStatus.channels[targetChannel].displayY+1);
						}
					}
				}

				// Draw Feedback
				if (oplStatus.channels[targetChannel].flag4Op == 0)
				{
					drawStringAtPosition(oplFeedbackNames[oplStatus.channels[targetChannel].feedback],oplStatus.channels[targetChannel].displayX+CHAN_DISP_OFFSET_OPERATOR_PARAMETERS+24,oplStatus.channels[targetChannel].displayY+2, COLOR_YELLOW, COLOR_BLACK);

				}
				else if (oplStatus.channels[targetChannel].flag4Op != 0)
				{
					if (targetChannel <= 2 || (targetChannel >= 9 && targetChannel <= 11))
					{
					drawStringAtPosition(oplFeedbackNames[oplStatus.channels[targetChannel].feedback],oplStatus.channels[targetChannel].displayX+CHAN_DISP_OFFSET_OPERATOR_PARAMETERS+24,oplStatus.channels[targetChannel].displayY+4, COLOR_YELLOW, COLOR_BLACK);
					}
				}
			}

			// Waveform Select
			else if ((i >= 0xE0 && i <= 0xF5) || (i >= 0x1E0 && i <= 0x1F5))
			{
				if (i >= 0xE0 && i <= 0xF5)
				{
					targetChannel = oplOperatorToChannel[(i - 0xE0)];
					targetOperator = oplOffsetToOperator[(i - 0xE0)];
				}
				else if (i >= 0x1E0 && i <= 0x1F5)
				{
					targetChannel = oplOperatorToChannel[(i - 0x1E0)] + 9;
					targetOperator = oplOffsetToOperator[(i - 0x1E0)];
				}

				// If OPL2, ignore the highest bit as those waveforms can't be used
				if (oplStatus.flagOPL3Mode == 0)
				{
					oplStatus.channels[targetChannel].operators[targetOperator].waveform = (oplRegisterMap[i] & 0x03);
				}
				else
				{
					oplStatus.channels[targetChannel].operators[targetOperator].waveform = (oplRegisterMap[i] & 0x07);
				}

				if (targetOperator == 0)
				{
					drawStringAtPosition(oplWaveformNames[oplStatus.channels[targetChannel].operators[targetOperator].waveform],oplStatus.channels[targetChannel].displayX+CHAN_DISP_OFFSET_OPERATOR_PARAMETERS,oplStatus.channels[targetChannel].displayY+1, COLOR_LIGHTCYAN, COLOR_BLACK);
				}
				else if (targetOperator == 1)
				{
					drawStringAtPosition(oplWaveformNames[oplStatus.channels[targetChannel].operators[targetOperator].waveform],oplStatus.channels[targetChannel].displayX+CHAN_DISP_OFFSET_OPERATOR_PARAMETERS,oplStatus.channels[targetChannel].displayY+3, COLOR_LIGHTGREEN, COLOR_BLACK);
				}
			}

			// OPL3 4-op flags
			else if (i == 0x104)
			{
				oplStatus.channels[0].flag4Op = oplRegisterMap[0x104] & 0x01;
				oplStatus.channels[3].flag4Op = oplRegisterMap[0x104] & 0x01;
				oplStatus.channels[1].flag4Op = oplRegisterMap[0x104] & 0x02 >> 1;
				oplStatus.channels[4].flag4Op = oplRegisterMap[0x104] & 0x02 >> 1;
				oplStatus.channels[2].flag4Op = oplRegisterMap[0x104] & 0x04 >> 2;
				oplStatus.channels[5].flag4Op = oplRegisterMap[0x104] & 0x04 >> 2;
				oplStatus.channels[9].flag4Op = oplRegisterMap[0x104] & 0x08 >> 3;
				oplStatus.channels[12].flag4Op = oplRegisterMap[0x104] & 0x08 >> 3;
				oplStatus.channels[10].flag4Op = oplRegisterMap[0x104] & 0x10 >> 4;
				oplStatus.channels[13].flag4Op = oplRegisterMap[0x104] & 0x10 >> 4;
				oplStatus.channels[11].flag4Op = oplRegisterMap[0x104] & 0x20 >> 5;
				oplStatus.channels[14].flag4Op = oplRegisterMap[0x104] & 0x20 >> 5;

				// After changing 4-op flags, force redraw of things that may need repositioning or changed
				
				// Recalculate Y positions of channels
				// Go in channel number order if we are in 2-op
				if (oplStatus.channels[0].flag4Op == FALSE && oplStatus.channels[1].flag4Op == FALSE && oplStatus.channels[2].flag4Op == FALSE && oplStatus.channels[9].flag4Op == FALSE && oplStatus.channels[10].flag4Op == FALSE && oplStatus.channels[11].flag4Op == FALSE)
				{
					// Set default Y positions for channels (2-op in-order)
					for (j=0; j<18; j++)
					{
						if (j < 9)
						{
							oplStatus.channels[j].displayY = CHAN_TABLE_START_Y+(j*4);
						}
						else
						{
							oplStatus.channels[j].displayY = CHAN_TABLE_START_Y+((j-9)*4);				
						}
					}
				}
				// If any channels switch to 4-op, use the 4-op ordering
				if (oplStatus.channels[0].flag4Op == TRUE || oplStatus.channels[1].flag4Op == TRUE || oplStatus.channels[2].flag4Op == TRUE || oplStatus.channels[9].flag4Op == TRUE || oplStatus.channels[10].flag4Op == TRUE || oplStatus.channels[11].flag4Op == TRUE)
				{
					oplStatus.channels[0].displayY = CHAN_TABLE_START_Y;
					oplStatus.channels[3].displayY = CHAN_TABLE_START_Y+4;
					oplStatus.channels[1].displayY = CHAN_TABLE_START_Y+8;
					oplStatus.channels[4].displayY = CHAN_TABLE_START_Y+12;
					oplStatus.channels[2].displayY = CHAN_TABLE_START_Y+16;
					oplStatus.channels[5].displayY = CHAN_TABLE_START_Y+20;
					oplStatus.channels[9].displayY = CHAN_TABLE_START_Y;
					oplStatus.channels[12].displayY = CHAN_TABLE_START_Y+4;
					oplStatus.channels[10].displayY = CHAN_TABLE_START_Y+8;
					oplStatus.channels[13].displayY = CHAN_TABLE_START_Y+12;
					oplStatus.channels[11].displayY = CHAN_TABLE_START_Y+16;
					oplStatus.channels[14].displayY = CHAN_TABLE_START_Y+20;
				}

				// 0xAx Frequency (Low)
				// 0xBx Key-On / Block / Frequency (High)
				// 0xCx Algorithm / Feedback / Panning
				for (j = 0x00; j <= 0x02; j++)
				{
					oplChangeMap[0xA0+j] = 1;
					oplChangeMap[0xB0+j] = 1;
					oplChangeMap[0xC0+j] = 1;
				}
				for (j = 0x00; j <= 0x02; j++)
				{
					oplChangeMap[0x1A0+j] = 1;
					oplChangeMap[0x1B0+j] = 1;
					oplChangeMap[0x1C0+j] = 1;
				}

				// Channel names
				for (j=0; j < maxChannels; j++)
				{
					// Rename 4-op channels
					if (j <=2 || (j >= 9 && j <= 11))
					{

						if (oplStatus.channels[j].flag4Op == 1)
						{
							drawStringAtPosition("4OP", oplStatus.channels[j].displayX+36, oplStatus.channels[j].displayY, COLOR_LIGHTRED, COLOR_BLACK);
							if (j == 0)
							{
								drawStringAtPosition("01+04", oplStatus.channels[0].displayX+2, oplStatus.channels[0].displayY, COLOR_WHITE, COLOR_BLACK);
							}
							else if (j == 1)
							{
								drawStringAtPosition("02+05", oplStatus.channels[1].displayX+2, oplStatus.channels[1].displayY, COLOR_WHITE, COLOR_BLACK);
							}
							else if (j == 2)
							{
								drawStringAtPosition("03+06", oplStatus.channels[2].displayX+2, oplStatus.channels[2].displayY, COLOR_WHITE, COLOR_BLACK);
							}
							else if (j == 9)
							{
								drawStringAtPosition("10+13", oplStatus.channels[9].displayX+2, oplStatus.channels[9].displayY, COLOR_WHITE, COLOR_BLACK);
							}
							else if (j == 10)
							{
								drawStringAtPosition("11+14", oplStatus.channels[10].displayX+2, oplStatus.channels[10].displayY, COLOR_WHITE, COLOR_BLACK);
							}
							else if (j == 11)
							{
								drawStringAtPosition("12+15", oplStatus.channels[11].displayX+2, oplStatus.channels[11].displayY, COLOR_WHITE, COLOR_BLACK);
							}
						}
					}
					// Channel has reverted to 2-op - need to redraw the paired channels too
					if ( j <=5 || (j >= 9 && j <= 14))
					{
						if (oplStatus.channels[j].flag4Op == 0)
						{

							sprintf(txtDrawBuffer, "Ch.%02d", j+1);
							drawStringAtPosition(txtDrawBuffer, oplStatus.channels[j].displayX+2, oplStatus.channels[j].displayY, COLOR_WHITE, COLOR_BLACK);
							drawStringAtPosition("\xCD\xCD\xCD", oplStatus.channels[j].displayX+36, oplStatus.channels[j].displayY, COLOR_DARKGREY, COLOR_BLACK);
						}
					}
				}
				// Redraw lines 'n stuff
				for (j=0; j < maxChannels; j++)
				{

					// Channel headers (for channels that can switch between 2op and 4op)
					if (j < 3 || (j > 5 && j < 12) || j > 14)
					{
						for (k=1; k<36; k++)
						{
							// Skip the channel number location
							if ( k<2 || k>6 )
							{	// Draw horizontal line
								drawCharacterAtPosition(CHAR_BOX_DOUBLE_HORIZONTAL, oplStatus.channels[j].displayX+k, oplStatus.channels[j].displayY, COLOR_DARKGREY, COLOR_BLACK);
							}
						}
						// Draw corner pieces
						drawCharacterAtPosition(CHAR_BOX_DOWN_SINGLE_RIGHT_DOUBLE, oplStatus.channels[j].displayX, oplStatus.channels[j].displayY, COLOR_DARKGREY, COLOR_BLACK);
						drawCharacterAtPosition(CHAR_BOX_DOWN_SINGLE_LEFT_DOUBLE, oplStatus.channels[j].displayX+39, oplStatus.channels[j].displayY, COLOR_DARKGREY, COLOR_BLACK);
						// Clear extra characters that may have been left behind in 2-op mode
						if (oplStatus.channels[j].flag4Op == 1)
						{
							drawCharacterAtPosition(CHAR_BOX_SINGLE_VERTICAL, oplStatus.channels[j].displayX, oplStatus.channels[j].displayY+4, COLOR_DARKGREY, COLOR_BLACK);
							drawCharacterAtPosition(CHAR_BOX_SINGLE_VERTICAL, oplStatus.channels[j].displayX+39, oplStatus.channels[j].displayY+4, COLOR_DARKGREY, COLOR_BLACK);
							for (k=8; k<33; k++)
							{
								drawCharacterAtPosition(' ', oplStatus.channels[j].displayX+k, oplStatus.channels[j].displayY+4, COLOR_BLACK, COLOR_BLACK);
							}
							drawCharacterAtPosition(' ', oplStatus.channels[j].displayX+35, oplStatus.channels[j].displayY+4, COLOR_BLACK, COLOR_BLACK);
						}

					}
					// Channel headers (2op channels)
					else
					{
						if (oplStatus.channels[j].flag4Op == 0)
						{
							for (k=1; k<36; k++)
							{
								// Skip the channel number location
								if ( k<2 || k>6 )
								{
									// Draw horizontal line
									drawCharacterAtPosition(CHAR_BOX_DOUBLE_HORIZONTAL, oplStatus.channels[j].displayX+k, oplStatus.channels[j].displayY, COLOR_DARKGREY, COLOR_BLACK);
								}
							}
							// Draw corner pieces
							drawCharacterAtPosition(CHAR_BOX_DOWN_SINGLE_RIGHT_DOUBLE, oplStatus.channels[j].displayX, oplStatus.channels[j].displayY, COLOR_DARKGREY, COLOR_BLACK);
							drawCharacterAtPosition(CHAR_BOX_DOWN_SINGLE_LEFT_DOUBLE, oplStatus.channels[j].displayX+39, oplStatus.channels[j].displayY, COLOR_DARKGREY, COLOR_BLACK);
						}
					}
				}
			}

			// OPL3 "new bit"
			else if (i == 0x105)
			{
				oplStatus.flagOPL3Mode = (oplRegisterMap[0x105] & 0x01);

				// Force redraw panning
				for (j = 0x00; j <= 0x08; j++)
				{
					oplChangeMap[0xC0+j] = 1;
				}
				for (j = 0x00; j <= 0x08; j++)
				{
					oplChangeMap[0x1C0+j] = 1;
				}
			}

			// End of processing, this byte has been handled
			oplChangeMap[i] = 0;
		}
	}
	// Draw operation is done
	requestScreenDraw = 0;
}

void drawTextUI(void)
{
	uint8_t i;
	uint8_t j;

	// Blue bar at top
	for (i=0; i<55; i++)
	{
		drawStringAtPosition(" ",i,0,COLOR_WHITE,COLOR_BLUE);
	}
	sprintf(txtDrawBuffer, "VGMSlap! %s", VGMSLAP_VERSION);
	drawStringAtPosition(txtDrawBuffer,0,0,COLOR_WHITE,COLOR_BLUE);
	if (playlistMode == FALSE)
	{
		drawStringAtPosition("Now playing:             ",55,0,COLOR_WHITE,COLOR_BLUE);
		drawStringAtPosition(basename(fileName),68,0,COLOR_YELLOW,COLOR_BLUE);
	}
	else if (playlistMode == TRUE)
	{
		drawStringAtPosition("Playlist:               ",55,0,COLOR_WHITE,COLOR_BLUE);
		sprintf(txtDrawBuffer, "[%5u / %5u]", playlistLineNumber, playlistMax);
		drawStringAtPosition(txtDrawBuffer,65,0,COLOR_YELLOW,COLOR_BLUE);
	}

	// GD3 tag info
	drawStringAtPosition("Title:  ",GD3_LABEL_START_X,GD3_START_Y,COLOR_LIGHTGREY,COLOR_BLACK);
	sprintf(txtDrawBuffer, "%S", currentGD3Tag.trackNameE);
	drawStringAtPosition(txtDrawBuffer,GD3_TAG_START_X,GD3_START_Y,COLOR_LIGHTCYAN,COLOR_BLACK);

	drawStringAtPosition("Artist: ",GD3_LABEL_START_X,GD3_START_Y+1,COLOR_LIGHTGREY,COLOR_BLACK);
	sprintf(txtDrawBuffer, "%S", currentGD3Tag.originalAuthorE);
	drawStringAtPosition(txtDrawBuffer,GD3_TAG_START_X,GD3_START_Y+1,COLOR_LIGHTCYAN,COLOR_BLACK);

	drawStringAtPosition("Game:   ",GD3_LABEL_START_X,GD3_START_Y+2,COLOR_LIGHTGREY,COLOR_BLACK);
	sprintf(txtDrawBuffer, "%S", currentGD3Tag.gameNameE);
	drawStringAtPosition(txtDrawBuffer,GD3_TAG_START_X,GD3_START_Y+2,COLOR_LIGHTCYAN,COLOR_BLACK);

	drawStringAtPosition("Date:   ",GD3_LABEL_START_X,GD3_START_Y+3,COLOR_LIGHTGREY,COLOR_BLACK);
	sprintf(txtDrawBuffer, "%S", currentGD3Tag.releaseDate);
	drawStringAtPosition(txtDrawBuffer,GD3_TAG_START_X,GD3_START_Y+3,COLOR_LIGHTCYAN,COLOR_BLACK);

	if (settings.struggleBus == FALSE)
	{
		// Set fixed X positions for the channels
		for (i=0; i<9; i++)
		{
		oplStatus.channels[i].displayX = CHAN_TABLE_START_X;
		}
		if (maxChannels > 9)
		{
			for (i=9; i<18; i++)
			{
				oplStatus.channels[i].displayX = CHAN_TABLE_START_X+40;
			}
		}
		
		// Set default Y positions for channels (2-op in-order)
		for (i=0; i<9; i++)
		{
		oplStatus.channels[i].displayY = CHAN_TABLE_START_Y+(i*4);
		}
		if (maxChannels > 9)
		{
			for (i=9; i<18; i++)
			{
				oplStatus.channels[i].displayY = CHAN_TABLE_START_Y+((i-9)*4);
			}
		}
			
		for (i=0; i<maxChannels; i++)
		{
			// Initial channel numbers
			sprintf(txtDrawBuffer, "Ch.%02d", i+1);
			drawStringAtPosition(txtDrawBuffer, oplStatus.channels[i].displayX+2, oplStatus.channels[i].displayY, COLOR_WHITE, COLOR_BLACK);
			drawStringAtPosition("\xCD\xCD\xCD", oplStatus.channels[i].displayX+36, oplStatus.channels[i].displayY, COLOR_DARKGREY, COLOR_BLACK);

			// Initial horizontal lines
			// Will be redrawn if we change to 4op mode
			for (j=1; j<36; j++)
			{
				// Skip the channel number location
				if ( j<2 || j>6 )
				{	// Draw horizontal line
					drawCharacterAtPosition(CHAR_BOX_DOUBLE_HORIZONTAL, oplStatus.channels[i].displayX+j, oplStatus.channels[i].displayY, COLOR_DARKGREY, COLOR_BLACK);
				}
			}
			// Draw corner pieces
			drawCharacterAtPosition(CHAR_BOX_DOWN_SINGLE_RIGHT_DOUBLE, oplStatus.channels[i].displayX, oplStatus.channels[i].displayY, COLOR_DARKGREY, COLOR_BLACK);
			drawCharacterAtPosition(CHAR_BOX_DOWN_SINGLE_LEFT_DOUBLE, oplStatus.channels[i].displayX+39, oplStatus.channels[i].displayY, COLOR_DARKGREY, COLOR_BLACK);
		}

		// Some vertical lines (these never change)
		for (i=0; i<maxChannels; i++)
		{
			drawCharacterAtPosition(CHAR_BOX_SINGLE_VERTICAL, oplStatus.channels[i].displayX, oplStatus.channels[i].displayY+1, COLOR_DARKGREY, COLOR_BLACK);
			drawCharacterAtPosition(CHAR_BOX_SINGLE_VERTICAL, oplStatus.channels[i].displayX, oplStatus.channels[i].displayY+2, COLOR_DARKGREY, COLOR_BLACK);
			drawCharacterAtPosition(CHAR_BOX_SINGLE_VERTICAL, oplStatus.channels[i].displayX, oplStatus.channels[i].displayY+3, COLOR_DARKGREY, COLOR_BLACK);
			drawCharacterAtPosition(CHAR_BOX_SINGLE_VERTICAL, oplStatus.channels[i].displayX+39, oplStatus.channels[i].displayY+1, COLOR_DARKGREY, COLOR_BLACK);
			drawCharacterAtPosition(CHAR_BOX_SINGLE_VERTICAL, oplStatus.channels[i].displayX+39, oplStatus.channels[i].displayY+2, COLOR_DARKGREY, COLOR_BLACK);
			drawCharacterAtPosition(CHAR_BOX_SINGLE_VERTICAL, oplStatus.channels[i].displayX+39, oplStatus.channels[i].displayY+3, COLOR_DARKGREY, COLOR_BLACK);
		}

		// Bottom of channel display (never changes)

		drawCharacterAtPosition(CHAR_BOX_UP_SINGLE_RIGHT_SINGLE, oplStatus.channels[8].displayX, oplStatus.channels[8].displayY+4, COLOR_DARKGREY, COLOR_BLACK);
		drawCharacterAtPosition(CHAR_BOX_UP_SINGLE_LEFT_SINGLE, oplStatus.channels[8].displayX+39, oplStatus.channels[8].displayY+4, COLOR_DARKGREY, COLOR_BLACK);
		for (i=1; i<39; i++)
		{
			drawCharacterAtPosition(CHAR_BOX_SINGLE_HORIZONTAL, oplStatus.channels[8].displayX+i, oplStatus.channels[8].displayY+4, COLOR_DARKGREY, COLOR_BLACK);
		}
		if (maxChannels > 9)
		{
			drawCharacterAtPosition(CHAR_BOX_UP_SINGLE_RIGHT_SINGLE, oplStatus.channels[17].displayX, oplStatus.channels[17].displayY+4, COLOR_DARKGREY, COLOR_BLACK);
			drawCharacterAtPosition(CHAR_BOX_UP_SINGLE_LEFT_SINGLE, oplStatus.channels[17].displayX+39, oplStatus.channels[17].displayY+4, COLOR_DARKGREY, COLOR_BLACK);
			for (i=1; i<39; i++)
			{
				drawCharacterAtPosition(CHAR_BOX_SINGLE_HORIZONTAL, oplStatus.channels[17].displayX+i, oplStatus.channels[8].displayY+4, COLOR_DARKGREY, COLOR_BLACK);
			}
		}
	}
}

void inputHandler (void)
{
	// Capture key
	if (kbhit())
	{
		keyboardCurrent = getch();
		// If extended key code (arrows for instance) get the extended code and flip the extended flag
		if (keyboardCurrent == 0x00)
		{
			keyboardExtendedFlag = 1;
			keyboardCurrent = getch();
		}
	}
	// No key is being pressed - reset key tracking
	else
	{

		keyboardPrevious = 0;
		keyboardCurrent = 0;
		keyboardExtendedFlag = 0;
	}
	// Process input if this is a new keypress
	if (keyboardPrevious != keyboardCurrent)
	{
		// Extended keycodes
		if (keyboardExtendedFlag == 1)
		{
			// Only do these if there's a playlist
			if (playlistMode == TRUE)
			{
				// Left Arrow
				if (keyboardCurrent == 0x4B)
				{
					// Go back 2 because the state handler already increments by 1
					if (playlistLineNumber < 2)
					{
						playlistLineNumber = 0;
					}
					else
					{
						playlistLineNumber = playlistLineNumber - 2;
					}
					programState = STATE_END_OF_SONG;
				}

				// Right Arrow
				if (keyboardCurrent == 0x4D)
				{
					// Treat like the track finished
					programState = STATE_END_OF_SONG;

				}
			}
		}

		// R - Resets OPL (panic button)
		if (keyboardCurrent == 0x52 || keyboardCurrent == 0x72)
		{
			resetOPL();
		}
		keyboardPrevious = keyboardCurrent;

		// Esc - Quit
		if (keyboardCurrent == 0x1B)
		{
			programState = STATE_EXIT;
		}
		keyboardPrevious = keyboardCurrent;
	}
}
