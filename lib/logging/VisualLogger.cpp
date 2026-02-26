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


void VisualLogger::updateWithLock(const std::string & channel, const std::function<void(IVisualLogBuilder & logBuilder)> & func)
{
	std::lock_guard lock(mutex);

	mapLines[channel].clear();
	mapTexts[channel].clear();
	battleTexts[channel].clear();

	VisualLogBuilder builder(mapLines[channel], mapTexts[channel], battleTexts[channel]);
	
	func(builder);
}

void VisualLogger::visualize(IMapOverlayLogVisualizer & visulizer)
{
	std::lock_guard lock(mutex);

	for(const auto & line : mapLines[keyToShow])
	{
		visulizer.drawLine(line.start, line.end);
	}

	std::map<int3, std::vector<Text<int3>>> textMap;

	for(const auto & line : mapTexts[keyToShow])
	{
		textMap[line.tile].push_back(line);
	}

	for(const auto & pair : textMap)
	{
		for(int i = 0; i < pair.second.size(); i++)
		{
			visulizer.drawText(pair.first, i, pair.second[i].text, pair.second[i].background);
		}
	}
}

void VisualLogger::visualize(IBattleOverlayLogVisualizer & visulizer)
{
	std::lock_guard lock(mutex);
	std::map<BattleHex, std::vector<std::string>> textMap;

	for(auto line : battleTexts[keyToShow])
	{
		textMap[line.tile].push_back(line.text);
	}

	for(auto & pair : textMap)
	{
		for(int i = 0; i < pair.second.size(); i++)
		{
			visulizer.drawText(pair.first, i, pair.second[i]);
		}
	}
}

void VisualLogger::setKey(const std::string & key)
{
	keyToShow = key;
}

void IVisualLogBuilder::addText(int3 tile, const std::string & text, PlayerColor background)
{
	std::optional<ColorRGBA> rgbColor;

	switch(background.getNum())
	{
	case 0:
		rgbColor = ColorRGBA(255, 0, 0);
		break;
	case 1:
		rgbColor = ColorRGBA(0, 0, 255);
		break;
	case 2:
		rgbColor = ColorRGBA(128, 128, 128);
		break;
	case 3:
		rgbColor = ColorRGBA(0, 255, 0);
		break;
	case 4:
		rgbColor = ColorRGBA(255, 128, 0);
		break;
	case 5:
		rgbColor = ColorRGBA(128, 0, 128);
		break;
	case 6:
		rgbColor = ColorRGBA(0, 255, 255);
		break;
	case 7:
		rgbColor = ColorRGBA(255, 128, 255);
		break;
	}

	addText(tile, text, rgbColor);
}

VCMI_LIB_NAMESPACE_END
