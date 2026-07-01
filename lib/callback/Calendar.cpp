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

#include "../IGameSettings.h"

VCMI_LIB_NAMESPACE_BEGIN

Calendar::Calendar(const IGameSettings & settings, int day)
	: gameSettings(&settings), day(day)
{
}

int Calendar::getCurrentDay() const
{
	return day;
}

int Calendar::getDayOfWeek() const
{
	int daysPerWeek = getDaysInWeek();
	int temp = day % daysPerWeek;
	return temp ? temp : daysPerWeek;
}

int Calendar::getDayOfMonth() const
{
	int daysPerMonth = getDaysInMonth();
	int temp = day % daysPerMonth;
	return temp ? temp : daysPerMonth;
}

int Calendar::getWeek() const
{
	int daysPerWeek = getDaysInWeek();
	int daysPerMonth = getDaysInMonth();
	int temp = ((day - 1) % daysPerMonth) + 1;
	return ((temp - 1) / daysPerWeek) + 1;
}

int Calendar::getMonth() const
{
	return ((day - 1) / getDaysInMonth()) + 1;
}

int Calendar::getDaysInWeek() const
{
	return gameSettings->getInteger(EGameSettings::GENERAL_DAYS_PER_WEEK);
}

int Calendar::getDaysInMonth() const
{
	return getWeeksInMonth() * getDaysInWeek();
}

int Calendar::getWeeksInMonth() const
{
	return gameSettings->getInteger(EGameSettings::GENERAL_WEEKS_PER_MONTH);
}

Calendar Calendar::nextDay() const
{
	return Calendar(*gameSettings, day + 1);
}

VCMI_LIB_NAMESPACE_END
