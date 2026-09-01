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

#include <vcmi/scripting/ApiTags.h>

class IGameSettings;

/// Small value type that interprets a day count using the game's calendar settings
/// (days per week, weeks per month). Holds the day count and a non-owning pointer
/// to the IGameSettings that provides week/month length.
class DLL_LINKAGE Calendar final : public scripting::ApiCopyable<Calendar>
{
	const IGameSettings * gameSettings = nullptr;
	int day = 0;

	/// Throws if this Calendar was default-constructed and so has no settings to interpret `day` with
	const IGameSettings & settings() const;

public:
	/// Only exists to satisfy the scripting tag - every accessor throws until a real Calendar is assigned
	Calendar() = default;
	Calendar(const IGameSettings & settings, int day);

	/// Total number of days since start of the game (1..inf)
	int getCurrentDay() const;
	/// Day within current week (1..daysPerWeek)
	int getDayOfWeek() const;
	/// Day within current month (1..daysPerMonth)
	int getDayOfMonth() const;
	/// Week within current month (1..weeksPerMonth)
	int getWeek() const;
	/// Current month (1..inf)
	int getMonth() const;
	/// Configured number of days per week
	int getDaysInWeek() const;
	/// Configured number of days per month (daysPerWeek * weeksPerMonth)
	int getDaysInMonth() const;
	/// Configured number of weeks per month
	int getWeeksInMonth() const;

	/// Returns a Calendar advanced by one day
	Calendar nextDay() const;
};
