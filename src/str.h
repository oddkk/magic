#ifndef MAGIC_STRING_H
#define MAGIC_STRING_H

#include "intdef.h"
#include <stdarg.h>

typedef struct {
	char *text;
	size_t length;

} String;

#define LIT(x) ((int)(x).length),((char*)(x).text)
#define STR(cstr) (String){cstr, sizeof(cstr)-1}

#endif
