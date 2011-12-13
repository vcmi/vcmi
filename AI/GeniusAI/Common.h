#pragma once

#ifdef __GNUC__
#define strcpy_s(a, b, c) strncpy(a, c, b)
#endif

#pragma warning (disable: 4100 4244)
#include "../../lib/AI_Base.h"
#pragma warning (default: 4100 4244)

void DbgBox(const char *msg, bool messageBox = false);