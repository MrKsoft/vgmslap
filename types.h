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
// TYPES.H - Useful type definitions
//
///////////////////////////////////////////////////////////////////////////////

#ifndef VGMSLAP_TYPES_H
#define VGMSLAP_TYPES_H

///////////////////////////////////////////////////////////////////////////////
// Type definitions
///////////////////////////////////////////////////////////////////////////////

// Good ol' Booleans

#define FALSE 0
#define TRUE 1

// Use these where possible to reduce bit/signing confusion
// Example, recall that "char" is default unsigned, but "int" is default signed

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned long uint32_t;

///////////////////////////////////////////////////////////////////////////////
// Enum declarations
///////////////////////////////////////////////////////////////////////////////

typedef enum{
	STATE_INITIALIZATION,
	STATE_PLAYING,
	STATE_END_OF_SONG,
	STATE_EXIT
} ProgramState;

typedef enum{
	EXIT_OK,
	ERROR_NO_ARGUMENTS,
	ERROR_FILE_MISSING,
	ERROR_BAD_FILETYPE,
	ERROR_VGM_VERSION,
	ERROR_VGM_NO_SUPPORTED_CHIPS,
	ERROR_VGM_BAD_COMMAND,
	ERROR_OPL_DETECTION_FAILED,
	ERROR_DECOMPRESSION_FAILED,
	ERROR_LOAD_FAILED_PLAYLIST,
	ERROR_LOAD_FAILED_VGM,
	ERROR_LOAD_FAILED_ZLIB,
	ERROR_LOAD_FAILED_TEMPFILE
} ProgramExitCode;

typedef enum{
	TEXT_80X25,
	TEXT_80X43,
	TEXT_80X50
} ScreenMode;

typedef enum{
	VGM_NO_OPL,      // Depending on chips in VGM, they'll work on the following chips:
	VGM_SINGLE_OPL1, // OPL2 / 2xOPL2 / OPL3
	VGM_SINGLE_OPL2, // OPL2 / 2xOPL2 / OPL3
	VGM_SINGLE_OPL3, // OPL3
	VGM_DUAL_OPL1,   // 2xOPL2 / OPL3
	VGM_DUAL_OPL2,   // 2xOPL2 / OPL3
	VGM_OPL1_OPL2,   // 2xOPL2 / OPL3
	VGM_DUAL_OPL3    // 2xOPL3 (theoretically possible but no way of testing - yet)
} VgmChipType;

typedef enum{
	DETECTED_NONE,
	DETECTED_OPL2,
	DETECTED_DUAL_OPL2,
	DETECTED_OPL3
} OplDetectedType;

#endif
