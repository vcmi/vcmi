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
#include <SDL_video.h>
#include <SDL_events.h>
#include <SDL_syswm.h>

#if defined(VCMI_WINDOWS)

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

void NotificationHandler::notify(std::string msg)
{
	NOTIFYICONDATA niData;
	SDL_SysWMinfo info;
	SDL_VERSION(&info.version);

	if(!SDL_GetWindowWMInfo(state.window, &info))
		return;

	if(info.info.win.window == GetForegroundWindow())
		return;

	ZeroMemory(&niData, sizeof(NOTIFYICONDATA));

	niData.cbSize = sizeof(NOTIFYICONDATA);
	niData.hWnd = info.info.win.window;
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

	SDL_EventState(SDL_SYSWMEVENT, SDL_ENABLE);

	NOTIFYICONDATA niData;
	SDL_SysWMinfo info;
	SDL_VERSION(&info.version);

	if(!SDL_GetWindowWMInfo(state.window, &info))
		return;

	ZeroMemory(&niData, sizeof(NOTIFYICONDATA));

	state.hInst = (HINSTANCE)GetModuleHandle("VCMI_client.exe");

	niData.cbSize = sizeof(NOTIFYICONDATA);
	niData.hWnd = info.info.win.window;
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
	SDL_SysWMinfo info;
	SDL_VERSION(&info.version);

	if(!SDL_GetWindowWMInfo(state.window, &info))
		return;

	ZeroMemory(&niData, sizeof(NOTIFYICONDATA));

	niData.cbSize = sizeof(NOTIFYICONDATA);
	niData.hWnd = info.info.win.window;
	niData.uID = 1;

	Shell_NotifyIcon(NIM_DELETE, &niData);
}

bool NotificationHandler::handleSdlEvent(const SDL_Event & ev)
{
	if(ev.syswm.msg->msg.win.msg == WM_USER_SHELLICON)
	{
		auto winMsg = LOWORD(ev.syswm.msg->msg.win.lParam);

		if(winMsg == WM_LBUTTONUP || winMsg == NIN_BALLOONUSERCLICK)
		{
			SDL_MinimizeWindow(state.window);
			SDL_RestoreWindow(state.window);
			SDL_RaiseWindow(state.window);

			return true;
		}
	}

	return false;
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

bool NotificationHandler::handleSdlEvent(const SDL_Event & ev)
{
	return false;
}

#endif
