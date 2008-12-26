#ifndef __STDAFX_H__
#define __STDAFX_H__
#ifndef __STDAFX_H__
#define __STDAFX_H__

// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
#include <cstdio>
#ifdef _WIN32
#include <tchar.h>
#else
#include "tchar_amigaos4.h"
#endif
#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <fstream>
#include "global.h"
// TODO: reference additional headers your program requires here

#endif // __STDAFX_H__
#endif // __STDAFX_H__
