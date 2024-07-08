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
// SETTINGS.C - Handling for configuration options
//
///////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "settings.h"

///////////////////////////////////////////////////////////////////////////////
// Initialize variables
///////////////////////////////////////////////////////////////////////////////

FILE *configFilePointer;

programSettings settings;

///////////////////////////////////////////////////////////////////////////////
// Functions
///////////////////////////////////////////////////////////////////////////////

void setConfig(void)
{
	char keyName[9];
	char keyValueText[8];
	uint16_t keyValueHex;
	uint16_t keyValueDecimal;
	char configLineBuffer[80];	// Buffer of a config line.  I picked 80 chars cause... uh... DOS
	// Try to load the settings file
	configFilePointer = fopen(settings.filePath,"rt");
	if (configFilePointer == NULL)
	{
		// If there's no config file, just give up.  Defaults have already been set.
		return;
	}
	while (fgets(configLineBuffer,sizeof(configLineBuffer),configFilePointer) != NULL)
	{
		if (configLineBuffer[0] != ';')
		{
			// Populate the setting name found and its value
			sscanf(configLineBuffer, "%s %s\n", keyName, keyValueText);
			// Convert the value to both hex and decimal so we can use whichever is best suited
			keyValueHex = (uint16_t)strtol(keyValueText, NULL, 16);
			keyValueDecimal = (uint16_t)strtol(keyValueText, NULL, 10);
			// Based on what we actually read in, set the appropriate settings value
			// OPL port
			if (strcmp(keyName, "PORT") == 0)
			{
				settings.oplBase = keyValueHex;
			}
			// Number of loops
			if (strcmp(keyName, "LOOPS") == 0)
			{
				// Bounds check
				if (keyValueDecimal > 255)
				{
					keyValueDecimal = 255;
				}
				settings.loopCount = keyValueDecimal;
			}
			// Timing frequency divider
			if (strcmp(keyName, "DIVIDER") == 0)
			{
				// Bounds check
				if (keyValueDecimal < 1)
				{
					keyValueDecimal = 1;
				}
				if (keyValueDecimal > 255)
				{
					keyValueDecimal = 255;
				}
				settings.frequencyDivider = keyValueDecimal;
			}
			// Struggle bus mode (display off)
			if (strcmp(keyName, "STRUGGLE") == 0)
			{
				// Bounds check
				if (keyValueDecimal > 1)
				{
					keyValueDecimal = 1;
				}
				settings.struggleBus = keyValueDecimal;
			}
		}
	}
}
