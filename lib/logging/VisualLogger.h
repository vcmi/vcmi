/*
 * VisualLogger.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../int3.h"
#include "../constants/EntityIdentifiers.h"

VCMI_LIB_NAMESPACE_BEGIN

class ILogVisualizer
{
public:
	virtual void drawLine(int3 start, int3 end) = 0;
};

class IVisualLogBuilder
{
public:
	virtual void addLine(int3 start, int3 end) = 0;
};

/// The logger is used to show screen overlay
class DLL_LINKAGE VisualLogger
{
private:
	struct MapLine
	{
		int3 start;
		int3 end;

		MapLine(int3 start, int3 end)
			:start(start), end(end)
		{
		}
	};

	class VisualLogBuilder : public IVisualLogBuilder
	{
	private:
		std::vector<MapLine> & mapLines;

	public:
		VisualLogBuilder(std::vector<MapLine> & mapLines)
			:mapLines(mapLines)
		{
		}

		virtual void addLine(int3 start, int3 end) override
		{
			mapLines.push_back(MapLine(start, end));
		}
	};

private:
	std::map<std::string, std::vector<MapLine>> mapLines;
	std::mutex mutex;
	std::string keyToShow;

public:

	void updateWithLock(std::string channel, std::function<void(IVisualLogBuilder & logBuilder)> func);
	void visualize(ILogVisualizer & visulizer);
	void setKey(std::string key);
};

extern DLL_LINKAGE VisualLogger * logVisual;

VCMI_LIB_NAMESPACE_END
