///////////////////////////////////////////////////////////////////////////////
// __      _______ __  __  _____ _             _ 
// \ \    / / ____|  \/  |/ ____| |           | |
//  \ \  / / |  __| \  / | (___ | | __ _ _ __ | |
//   \ \/ /| | |_ | |\/| |\___ \| |/ _` | '_ \| |   by Wafflenet, 2023-2024
//    \  / | |__| | |  | |____) | | (_| | |_) |_|       www.wafflenet.com
//     \/   \_____|_|  |_|_____/|_|\__,_| .__/(_)
//      (VGM Silly Little AdLib Player) | |      
//                                      |_|      
//
///////////////////////////////////////////////////////////////////////////////

// Text elements for VGMSlap's UI

// Starting coordinates for various channel table elements
#define GD3_LABEL_START_X 0
#define GD3_TAG_START_X 8
#define GD3_START_Y 2

#define CHAN_TABLE_START_X 0
#define CHAN_TABLE_START_Y 7
#define CHAN_DISP_OFFSET_CHANNEL_NOTEINFO 36
#define CHAN_DISP_OFFSET_CHANNEL_FEEDBACK 33
#define CHAN_DISP_OFFSET_OPERATOR_PARAMETERS 9

// Text identifiers for each waveform type, to be printed in the operator display
char oplWaveformNames[8][6] = {
	"/\x5C\/\x5C\0", 
	"^-^-\0", 
	"^^^^\0", 
	"/\x1C/\x1C\0", 
	"^v--\0", 
	"^^--\0", 
	"\xA9\xAA__\0", 
	"-\xFB\x5C-\0"};	
// Text identifiers for each synthesis/algorithm type, to be printed in the channel display
char oplAlgorithmNames[6][7] = {
	"FM   \0",		// 2 op  1+2
	"AS   \0",		// 2 op  1>2
	"FM-FM\0",		// 4-op  1>2>3>4
	"AS-FM\0",		// 4-op  1+(2>3>4)
	"FM-AS\0",		// 4-op  (1>2)+(3>4)
	"AS-AS\0"};		// 4-op  (2>3)+4
// Text identifiers for each feedback type, to be printed in the channel display
char oplFeedbackNames[8][4] = {
	" 0\0",
	"\xF6\x46\0", // div 16
	"\xF6\x38\0", // div 8
	"\xF6\x34\0", // div 4
	"\xF6\x32\0", // div 2
	"\xF6\x31\0", // div 1
	"x2\0",
	"x4\0"};
// Text identifiers for each KSL level, to be printed in the operator display
char oplKSLNames[4][5] = {
	"---\0",
	"1.5\0",
	"3.0\0",
	"6.0\0"};
// Text identifiers for each frequency multiplication level, to be printed in the operator display
char oplMultiplierNames[16][3] = {
	"\xAB\0",
	"1\0",
	"2\0",
	"3\0",
	"4\0",
	"5\0",
	"6\0",
	"7\0",
	"8\0",
	"9\0",
	"A\0",
	"A\0",
	"C\0",
	"C\0",
	"F\0",
	"F\0"};


// "Text Graphics"
// They are stored as arrays, characters followed by attribute colors
// When drawn using the drawCharacterGraphic function, the width and height are passed to it and it uses that to calculate where the text ends and attribute begins.
// There are certainly better ways to do this, but this way is mine!


// 2-op algorithm illustrations - 7x3

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

// 4-op algorithm illustrations - 7x7

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
