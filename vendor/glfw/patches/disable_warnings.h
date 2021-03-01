#ifndef TARGET_FILE
#error "A TARGET_FILE must be defined for disable_warnings.h"
#endif

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
#include TARGET_FILE
#pragma GCC diagnostic pop
