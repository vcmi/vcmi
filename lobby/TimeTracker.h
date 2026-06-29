/*
 * TimeTracker.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

/// Class that collects timing statistics of one or more events
/// and prints statistics data in log file on request
class TimeTracker : boost::noncopyable
{
public:
	/// Store time spent on single event of type "key"
	void store(const std::string & key, std::chrono::steady_clock::duration timeSpent)
	{
		auto & target = statistics[key];
		target.count += 1;
		target.total += timeSpent;
		target.worst = std::max(target.worst, timeSpent);
	}

	/// Prints statistics for all recorded events in logs and resets statistics storage
	void dumpToLog()
	{
		logGlobal->info("Performance statistics report");
		logGlobal->info("Operation \tCount \tTotal,s \tAverage,mks \tWorst,mks");

		for (const auto & entry : statistics)
		{
			logGlobal->info("%s\t%d\t%f\t%d\t%d",
							entry.first,
							entry.second.count,
							std::chrono::duration<float>(entry.second.total).count(),
							static_cast<int>(std::chrono::duration<float>(entry.second.total).count() * 1000 * 1000 / entry.second.count),
							static_cast<int>(std::chrono::duration<float>(entry.second.worst).count() * 1000 * 1000)
							);
		}
		statistics.clear();
	}

private:
	struct TimeStats
	{
		std::chrono::steady_clock::duration worst = {};
		std::chrono::steady_clock::duration total = {};
		uint32_t count = {};
	};
	std::map<std::string, TimeStats> statistics;
};

/// Convenience RAII wrapper for TimeTracker class usage
class TimeGuard : boost::noncopyable
{
	TimeTracker & owner;
	std::chrono::steady_clock::time_point startTime;
	std::string name;
public:
	explicit TimeGuard(TimeTracker & owner, const std::string & name)
		:owner(owner), startTime(std::chrono::steady_clock::now()), name(name)
	{}
	~TimeGuard()
	{
		auto endTime = std::chrono::steady_clock::now();
		owner.store(name, endTime - startTime);
	}
};
