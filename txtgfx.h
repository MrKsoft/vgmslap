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
// TXTGFX.H - Text mode resources
//
///////////////////////////////////////////////////////////////////////////////

#ifndef VGMSLAP_TXTGFX_H
#define VGMSLAP_TXTGFX_H

// Starting coordinates for various channel table elements
#define GD3_LABEL_START_X 0
#define GD3_TAG_START_X 8
#define GD3_START_Y 2

#define CHAN_TABLE_START_X 0
#define CHAN_TABLE_START_Y 7
#define CHAN_DISP_OFFSET_CHANNEL_NOTEINFO 36
#define CHAN_DISP_OFFSET_CHANNEL_FEEDBACK 33
#define CHAN_DISP_OFFSET_OPERATOR_PARAMETERS 9

#define CHAN_BARS_START_X 7
#define CHAN_BARS_START_Y 44

// Attribute colors
#define COLOR_BLACK 0x0
#define COLOR_BLUE 0x1
#define COLOR_GREEN 0x2
#define COLOR_CYAN 0x3
#define COLOR_RED 0x4
#define COLOR_MAGENTA 0x5
#define COLOR_BROWN 0x6
#define COLOR_LIGHTGREY 0x7
#define COLOR_DARKGREY 0x8
#define COLOR_LIGHTBLUE 0x9
#define COLOR_LIGHTGREEN 0xA
#define COLOR_LIGHTCYAN 0xB
#define COLOR_LIGHTRED 0xC
#define COLOR_LIGHTMAGENTA 0xD
#define COLOR_YELLOW 0xE
#define COLOR_WHITE 0xF

// Define special Code Page 437 characters

#define CHAR_MUSIC_NOTE 0x0E
#define CHAR_EXCLAMATION 0x21
#define CHAR_BOX_SINGLE_HORIZONTAL 0xC4
#define CHAR_BOX_DOUBLE_HORIZONTAL 0xCD
#define CHAR_BOX_SINGLE_VERTICAL 0xB3
#define CHAR_BOX_UP_SINGLE_RIGHT_SINGLE 0xC0
#define CHAR_BOX_UP_SINGLE_LEFT_SINGLE 0xD9
#define CHAR_BOX_DOWN_SINGLE_LEFT_DOUBLE 0xB8
#define CHAR_BOX_DOWN_SINGLE_RIGHT_DOUBLE 0xD5

///////////////////////////////////////////////////////////////////////////////
// Variable declarations
///////////////////////////////////////////////////////////////////////////////

// Text identifiers for each waveform type, to be printed in the operator display
extern char oplWaveformNames[8][5];

// Text identifiers for each synthesis/algorithm type, to be printed in the channel display
// Not currently used - replaced by graphical representations!
extern char oplAlgorithmNames[6][6];

// Text identifiers for each feedback type, to be printed in the channel display
extern char oplFeedbackNames[8][3];

// Text identifiers for each KSL level, to be printed in the operator display
extern char oplKSLNames[4][4];

// Text identifiers for each frequency multiplication level, to be printed in the operator display
extern char oplMultiplierNames[16][2];

// Text identifiers for boolean display values (checkmark)
extern char boolIndicator[2][2];

// "Text Graphics"
// They are stored as arrays, characters followed by attribute colors
// When drawn using the drawCharacterGraphic function, the width and height are passed to it and it uses that to calculate where the text ends and attribute begins.
// There are certainly better ways to do this, but this way is mine!

// 2-op algorithm illustrations - 7x3

extern const char tgAlgoFM[];
extern const char tgAlgoAS[];

// 4-op algorithm illustrations - 7x7

extern const char tgAlgoFMFM[];
extern const char tgAlgoASFM[];
extern const char tgAlgoFMAS[];
extern const char tgAlgoASAS[];

// Output level bars - 1x6 (ish)
// This array should be drawn with the alternative drawing function, due to vertical drawing and shared attribute
// Each row is a "level" of the bar from max to zero (16 units) - weighted based on how much attenuation you can hear

extern const char tgLevelBars[];

#endif
