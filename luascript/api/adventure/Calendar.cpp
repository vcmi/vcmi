/*
 * Calendar.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "Calendar.h"

#include "../../LuaCallWrapper.h"

namespace scripting::api
{

void CalendarProxy::registerMethods(MethodRegistrar & R)
{
	R.method<&Calendar::getCurrentDay>("getCurrentDay", {"Total number of days since the start of the game (1..)."},
		"Returns the current day count.");
	R.method<&Calendar::getDayOfWeek>("getDayOfWeek", {"Day within the current week (1..days-per-week)."},
		"Returns the day within the current week.");
	R.method<&Calendar::getDayOfMonth>("getDayOfMonth", {"Day within the current month (1..days-per-month)."},
		"Returns the day within the current month.");
	R.method<&Calendar::getWeek>("getWeek", {"Week within the current month (1..weeks-per-month)."},
		"Returns the week within the current month.");
	R.method<&Calendar::getMonth>("getMonth", {"Current month (1..)."},
		"Returns the current month.");
	R.method<&Calendar::getDaysInWeek>("getDaysInWeek", {"Configured number of days per week."},
		"Returns the number of days in a week.");
	R.method<&Calendar::getDaysInMonth>("getDaysInMonth", {"Configured number of days per month."},
		"Returns the number of days in a month.");
	R.method<&Calendar::getWeeksInMonth>("getWeeksInMonth", {"Configured number of weeks per month."},
		"Returns the number of weeks in a month.");
}

}
