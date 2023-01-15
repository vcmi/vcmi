/*
* NotificationHandler.h, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/

#pragma once

#include <SDL_events.h>

class NotificationHandler
{
public:
	static void notify(std::string msg);
	static void init(SDL_Window * window);
	static bool handleSdlEvent(const SDL_Event & ev);
	static void destroy();
};