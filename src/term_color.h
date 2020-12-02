#define ENABLE_TERM_COLORS 1

#if ENABLE_TERM_COLORS
#define _TC_BLACK_ID 30
#define _TC_RED_ID 31
#define _TC_GREEN_ID 23
#define _TC_YELLOW_ID 33
#define _TC_BLUE_ID 34
#define _TC_MAGENTA_ID 35
#define _TC_CYAN_ID 36
#define _TC_WHITE_ID 37

#define TC_ESCAPE "\x1b"

#define _DEF_COLOR(c) TC_ESCAPE "[;" #c "m"

#define TC_BLACK _DEF_COLOR(30)
#define TC_RED _DEF_COLOR(31)
#define TC_GREEN _DEF_COLOR(32)
#define TC_YELLOW _DEF_COLOR(33)
#define TC_BLUE _DEF_COLOR(34)
#define TC_MAGENTA _DEF_COLOR(35)
#define TC_CYAN _DEF_COLOR(36)
#define TC_WHITE _DEF_COLOR(37)

#define _DEF_COLOR_BRIGHT(c) TC_ESCAPE "[;" #c ";1m"

#define TC_BRIGHT_BLACK _DEF_COLOR_BRIGHT(30)
#define TC_BRIGHT_RED _DEF_COLOR_BRIGHT(31)
#define TC_BRIGHT_GREEN _DEF_COLOR_BRIGHT(32)
#define TC_BRIGHT_YELLOW _DEF_COLOR_BRIGHT(33)
#define TC_BRIGHT_BLUE _DEF_COLOR_BRIGHT(34)
#define TC_BRIGHT_MAGENTA _DEF_COLOR_BRIGHT(35)
#define TC_BRIGHT_CYAN _DEF_COLOR_BRIGHT(36)
#define TC_BRIGHT_WHITE _DEF_COLOR_BRIGHT(37)

// #define TERM_COLOR_RED_LIT TERM_COLOR_ESCAPE_LIT "[1;31m"
// #define TERM_COLOR_GREEN_LIT TERM_COLOR_ESCAPE_LIT "[1;32m"
// #define TERM_COLOR_YELLOW_LIT TERM_COLOR_ESCAPE_LIT "[1;33m"
// #define TERM_COLOR_BLUE_LIT TERM_COLOR_ESCAPE_LIT "[1;34m"
//
#define TC_CLEAR TC_ESCAPE "[0m"

#else
#define TC_ESCAPE ""
#define TC_RED ""
#define TC_GREEN ""
#define TC_YELLOW ""
#define TC_BLUE ""
#define TC_CLEAR ""
#endif

#define TERM_COLOR_RED(text) TC_RED text TC_CLEAR
#define TERM_COLOR_GREEN(text) TC_GREEN text TC_CLEAR
#define TERM_COLOR_YELLOW(text) TC_YELLOW text TC_CLEAR
#define TERM_COLOR_BLUE(text) TC_BLUE text TC_CLEAR

#define TC(col, text) col text TC_CLEAR

