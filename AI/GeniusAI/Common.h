#ifndef __GENIUS_COMMON__
#define __GENIUS_COMMON__

#ifdef __GNUC__
#define strcpy_s(a, b, c) strncpy(a, c, b)
#endif

#pragma warning (disable: 4100 4244)
#include "../../AI_Base.h"
#pragma warning (default: 4100 4244)

void DbgBox(const char *msg, bool messageBox = false);

#endif/*__GENIUS_COMMON__*/