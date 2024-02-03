/*
 * CLogger.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "VisualLogger.h"

VCMI_LIB_NAMESPACE_BEGIN

DLL_LINKAGE VisualLogger * logVisual = new VisualLogger();


void VisualLogger::updateWithLock(std::string channel, std::function<void(IVisualLogBuilder & logBuilder)> func)
{
	std::lock_guard<std::mutex> lock(mutex);

	mapLines[channel].clear();

	VisualLogBuilder builder(mapLines[channel]);
	
	func(builder);
}

void VisualLogger::visualize(ILogVisualizer & visulizer)
{
	std::lock_guard<std::mutex> lock(mutex);

	for(auto line : mapLines[keyToShow])
	{
		visulizer.drawLine(line.start, line.end);
	}
}

void VisualLogger::setKey(std::string key)
{
	keyToShow = key;
}

VCMI_LIB_NAMESPACE_END
