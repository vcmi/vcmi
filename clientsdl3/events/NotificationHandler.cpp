/*
* NotificationHandler.cpp, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/

#include "StdInc.h"
#include "NotificationHandler.h"

#if defined(VCMI_WINDOWS)
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_properties.h>
#include <SDL3/SDL_system.h>
#include <SDL3/SDL_video.h>

#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
// Windows Header Files:
#include <windows.h>
#include <shellapi.h>
// C RunTime Header Files

#define	WM_USER_SHELLICON WM_USER + 1

// Global Variables:

struct NotificationState
{
	HINSTANCE		hInst;	// current instance
	NOTIFYICONDATA	niData;	// notify icon data
	bool initialized = false;
	SDL_Window * window;
};

NotificationState state;

/// SDL3 exposes native handles through window properties instead of SDL_SysWMinfo
static HWND getWindowHandle(SDL_Window * window)
{
	if(window == nullptr)
		return nullptr;

	return static_cast<HWND>(SDL_GetPointerProperty(SDL_GetWindowProperties(window), SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr));
}

/// SDL3 has no SDL_SYSWMEVENT, native messages are delivered through this hook instead
static bool SDLCALL windowsMessageHook(void * userdata, MSG * msg)
{
	if(msg->message == WM_USER_SHELLICON && LOWORD(msg->lParam) == WM_LBUTTONUP)
	{
		SDL_MinimizeWindow(state.window);
		SDL_RestoreWindow(state.window);
		SDL_RaiseWindow(state.window);
	}

	return true;
}

void NotificationHandler::notify(std::string msg)
{
	NOTIFYICONDATA niData;
	HWND windowHandle = getWindowHandle(state.window);

	if(windowHandle == nullptr)
		return;

	if(windowHandle == GetForegroundWindow())
		return;

	ZeroMemory(&niData, sizeof(NOTIFYICONDATA));

	niData.cbSize = sizeof(NOTIFYICONDATA);
	niData.hWnd = windowHandle;
	niData.uID = 1;
	niData.uFlags = NIF_INFO | NIF_MESSAGE;
	niData.uCallbackMessage = WM_USER_SHELLICON;

	niData.dwInfoFlags = NIIF_INFO;
	msg.copy(niData.szInfo, msg.length());

	Shell_NotifyIcon(NIM_MODIFY, &niData);
}

void NotificationHandler::init(SDL_Window * window)
{
	state.window = window;

	if(state.initialized)
		return;

	SDL_SetWindowsMessageHook(windowsMessageHook, nullptr);

	NOTIFYICONDATA niData;
	HWND windowHandle = getWindowHandle(state.window);

	if(windowHandle == nullptr)
		return;

	ZeroMemory(&niData, sizeof(NOTIFYICONDATA));

	state.hInst = (HINSTANCE)GetModuleHandle("VCMI_client.exe");

	niData.cbSize = sizeof(NOTIFYICONDATA);
	niData.hWnd = windowHandle;
	niData.uID = 1;
	niData.uFlags = NIF_ICON | NIF_MESSAGE;
	niData.uCallbackMessage = WM_USER_SHELLICON;

	niData.hIcon = (HICON)LoadImage(
		state.hInst,
		"IDI_ICON1",
		IMAGE_ICON,
		GetSystemMetrics(SM_CXSMICON),
		GetSystemMetrics(SM_CYSMICON),
		LR_DEFAULTSIZE);

	Shell_NotifyIcon(NIM_ADD, &niData);

	state.initialized = true;
}

void NotificationHandler::destroy()
{
	NOTIFYICONDATA niData;
	HWND windowHandle = getWindowHandle(state.window);

	if(windowHandle == nullptr)
		return;

	SDL_SetWindowsMessageHook(nullptr, nullptr);

	ZeroMemory(&niData, sizeof(NOTIFYICONDATA));

	niData.cbSize = sizeof(NOTIFYICONDATA);
	niData.hWnd = windowHandle;
	niData.uID = 1;

	Shell_NotifyIcon(NIM_DELETE, &niData);
}

#elif defined(VCMI_ANDROID)

#include "../../lib/CAndroidVMHelper.h"

void NotificationHandler::notify(std::string msg)
{
	// java decides whether this is worth a notification - it knows whether the game is on screen
	CAndroidVMHelper vmHelper;
	vmHelper.callCustomMethod(CAndroidVMHelper::NATIVE_METHODS_DEFAULT_CLASS, "showNotification", "(Ljava/lang/String;)V",
		[&msg](JNIEnv * env, jclass cls, jmethodID method)
		{
			jstring message = env->NewStringUTF(msg.c_str());
			env->CallStaticVoidMethod(cls, method, message);
			env->DeleteLocalRef(message);
		}, true);
}

void NotificationHandler::init(SDL_Window * window)
{
}

void NotificationHandler::destroy()
{
}

#else

void NotificationHandler::notify(std::string msg)
{
}

void NotificationHandler::init(SDL_Window * window)
{
}

void NotificationHandler::destroy()
{
}

#endif
