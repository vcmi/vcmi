/*
 * Calendar.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "../../LuaWrapper.h"
#include "../MethodRegistrar.h"

#include "../../../lib/callback/Calendar.h"

namespace scripting::api
{

class CalendarProxy : public CopyableWrapper<const Calendar, CalendarProxy>
{
public:
	static constexpr std::string_view luaName = "Calendar";
	static constexpr std::string_view luaDescription =
		"Interprets an in-game day count using the map's calendar settings (days per week, "
		"weeks per month). Obtained from Game:getCalendar().";

	static void registerMethods(MethodRegistrar & R);
};

}
