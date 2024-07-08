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
// TXTGFX.C - Text mode resources
//
///////////////////////////////////////////////////////////////////////////////

#include "txtgfx.h"

///////////////////////////////////////////////////////////////////////////////
// Initialize values
///////////////////////////////////////////////////////////////////////////////

char oplWaveformNames[8][5] = {
	"/\x5C\/\x5C",
	"^-^-",
	"^^^^",
	"/\x1C/\x1C",
	"^v--",
	"^^--",
	"\xA9\xAA__",
	"-\xFB\x5C-"};

char oplAlgorithmNames[6][6] = {
	"FM   ",		// 2 op  1+2
	"AS   ",		// 2 op  1>2
	"FM-FM",		// 4-op  1>2>3>4
	"AS-FM",		// 4-op  1+(2>3>4)
	"FM-AS",		// 4-op  (1>2)+(3>4)
	"AS-AS"};		// 4-op  (2>3)+4

char oplFeedbackNames[8][3] = {
	" 0",
	"\xF6\x46", // div 16
	"\xF6\x38", // div 8
	"\xF6\x34", // div 4
	"\xF6\x32", // div 2
	"\xF6\x31", // div 1
	"x2",
	"x4"};

char oplKSLNames[4][4] = {
	"---",
	"1.5",
	"3.0",
	"6.0"};

char oplMultiplierNames[16][2] = {
	"\xAB",
	"1",
	"2",
	"3",
	"4",
	"5",
	"6",
	"7",
	"8",
	"9",
	"A",
	"A",
	"C",
	"C",
	"F",
	"F"};

char boolIndicator[2][2] = {
	"-",
	"\xFB"};

const char tgAlgoFM[] = {
	// Character
	0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
	0xDB, 0xC4, 0x10, 0xDB, 0xC4, 0x10, 0xDB,
	0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
	// Attribute
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x0B, 0x07, 0x07, 0x0A, 0x07, 0x07, 0x0E,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

const char tgAlgoAS[] = {
	// Character
	0xDB, 0xC4, 0xC4, 0xBF, 0x20, 0x20, 0x20,
	0x20, 0x20, 0x20, 0xC3, 0xC4, 0x10, 0xDB,
	0xDB, 0xC4, 0xC4, 0xD9, 0x20, 0x20, 0x20,
	// Attribute
	0x0B, 0x07, 0x07, 0x07, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x07, 0x07, 0x07, 0x0E,
	0x0A, 0x07, 0x07, 0x07, 0x00, 0x00, 0x00
};

const char tgAlgoFMFM[] = {
	// Character
	0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
	0xDB, 0x10, 0xDB, 0x10, 0xDB, 0x10, 0xDB,
	0x20, 0x20, 0x20, 0xDA, 0xC4, 0xC4, 0xD9,
	0x20, 0x20, 0x20, 0xB3, 0x20, 0x20, 0x20,
	0x20, 0x20, 0x20, 0x1F, 0x20, 0x20, 0x20,
	0x20, 0x20, 0x20, 0xDB, 0x20, 0x20, 0x20,
	0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
	// Attribute
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x0B, 0x07, 0x0A, 0x07, 0x0B, 0x07, 0x0A,
	0x00, 0x00, 0x00, 0x07, 0x07, 0x07, 0x07,
	0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x0E, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

const char tgAlgoASFM[] = {
	// Character
	0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
	0xDB, 0xC4, 0xC4, 0xC4, 0xBF, 0x20, 0x20,
	0x20, 0x20, 0x20, 0x20, 0xB3, 0x20, 0x20,
	0x20, 0x20, 0x20, 0x20, 0xC3, 0x10, 0xDB,
	0x20, 0x20, 0x20, 0x20, 0xB3, 0x20, 0x20,
	0xDB, 0x10, 0xDB, 0x10, 0xDB, 0x20, 0x20,
	0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
	// Attribute
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x0B, 0x07, 0x07, 0x07, 0x07, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x07, 0x07, 0x0E,
	0x00, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00,
	0x0A, 0x07, 0x0B, 0x07, 0x0A, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

const char tgAlgoFMAS[] = {
	// Character
	0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
	0xDB, 0xC4, 0x10, 0xDB, 0xBF, 0x20, 0x20,
	0x20, 0x20, 0x20, 0x20, 0xB3, 0x20, 0x20,
	0x20, 0x20, 0x20, 0x20, 0xC3, 0x10, 0xDB,
	0x20, 0x20, 0x20, 0x20, 0xB3, 0x20, 0x20,
	0xDB, 0xC4, 0x10, 0xDB, 0xD9, 0x20, 0x20,
	0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
	// Attribute
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x0B, 0x07, 0x07, 0x0A, 0x07, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x07, 0x07, 0x0E,
	0x00, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00,
	0x0B, 0x07, 0x07, 0x0A, 0x07, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

const char tgAlgoASAS[] = {
	// Character
	0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
	0xDB, 0xC4, 0xC4, 0xC4, 0xBF, 0x20, 0x20,
	0x20, 0x20, 0x20, 0x20, 0xB3, 0x20, 0x20,
	0xDB, 0xC4, 0x10, 0xDB, 0xC5, 0x10, 0xDB,
	0x20, 0x20, 0x20, 0x20, 0xB3, 0x20, 0x20,
	0xDB, 0xC4, 0xC4, 0xC4, 0xD9, 0x20, 0x20,
	0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
	// Attribute
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x0B, 0x07, 0x07, 0x07, 0x07, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00,
	0x0A, 0x07, 0x07, 0x0B, 0x07, 0x07, 0x0E,
	0x00, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00,
	0x0A, 0x07, 0x07, 0x07, 0x07, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};
