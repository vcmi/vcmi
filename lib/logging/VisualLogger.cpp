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
	mapTexts[channel].clear();
	battleTexts[channel].clear();

	VisualLogBuilder builder(mapLines[channel], mapTexts[channel], battleTexts[channel]);
	
	func(builder);
}

void VisualLogger::visualize(IMapOverlayLogVisualizer & visulizer)
{
	std::lock_guard<std::mutex> lock(mutex);

	for(auto line : mapLines[keyToShow])
	{
		visulizer.drawLine(line.start, line.end);
	}

	std::map<int3, std::vector<std::string>> textMap;

	for(auto line : mapTexts[keyToShow])
	{
		textMap[line.tile].push_back(line.text);
	}

	for(auto & pair : textMap)
	{
		visulizer.drawText(pair.first, pair.second);
	}
}

void VisualLogger::visualize(IBattleOverlayLogVisualizer & visulizer)
{
	std::lock_guard<std::mutex> lock(mutex);
	std::map<BattleHex, std::vector<std::string>> textMap;

	for(auto line : battleTexts[keyToShow])
	{
		textMap[line.tile].push_back(line.text);
	}

	for(auto & pair : textMap)
	{
		visulizer.drawText(pair.first, pair.second);
	}
}

void VisualLogger::setKey(std::string key)
{
	keyToShow = key;
}

VCMI_LIB_NAMESPACE_END
