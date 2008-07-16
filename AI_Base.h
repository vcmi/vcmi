#pragma once
#include <vector>
#include <iostream>
#include "int3.h"
#include "CGameInterface.h"

#define AI_INTERFACE_VER 1
#ifdef _WIN32
	#define DLL_EXPORT extern "C" __declspec(dllexport)
	#define VCMI_API
#elif __GNUC__ >= 4
	#define DLL_EXPORT extern "C" __attribute__ ((visibility("default")))
	#define VCMI_API __attribute__ ((visibility("default")))
#else
	#define VCMI_EXPORT extern "C"
#endif
