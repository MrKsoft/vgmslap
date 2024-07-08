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
// SETTINGS.H - Handling for configuration options
//
///////////////////////////////////////////////////////////////////////////////

#ifndef VGMSLAP_SETTINGS_H
#define VGMSLAP_SETTINGS_H

#include "types.h"

// Default setting definitions
#define CONFIG_DEFAULT_PORT 0x388
#define CONFIG_DEFAULT_LOOPS 1
#define CONFIG_DEFAULT_DIVIDER 1
#define CONFIG_DEFAULT_STRUGGLE 0

///////////////////////////////////////////////////////////////////////////////
// Function declarations
///////////////////////////////////////////////////////////////////////////////

void setConfig(void);	// Parse the config file and set settings

///////////////////////////////////////////////////////////////////////////////
// Variable declarations
///////////////////////////////////////////////////////////////////////////////

extern FILE *configFilePointer;	// Pointer to config file

///////////////////////////////////////////////////////////////////////////////
// Struct declarations
///////////////////////////////////////////////////////////////////////////////

typedef struct
{
	char filePath[PATH_MAX];
	char tempPath[PATH_MAX];
	uint16_t oplBase;
	uint8_t loopCount;
	uint8_t frequencyDivider; // Range should be 1-100
	uint8_t struggleBus;
} programSettings;

// Storage spot for program settings
extern programSettings settings;

#endif
