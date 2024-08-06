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
#include "../battle/BattleHex.h"

VCMI_LIB_NAMESPACE_BEGIN

class IMapOverlayLogVisualizer
{
public:
	virtual void drawLine(int3 start, int3 end) = 0;
};

class IBattleOverlayLogVisualizer
{
public:
	virtual void drawText(BattleHex tile, std::vector<std::string> texts) = 0;
};

class IVisualLogBuilder
{
public:
	virtual void addLine(int3 start, int3 end) = 0;
	virtual void addText(BattleHex tile, std::string text) = 0;
};

/// The logger is used to show screen overlay
class DLL_LINKAGE VisualLogger
{
private:
	template<typename T>
	struct Line
	{
		T start;
		T end;

		Line(T start, T end)
			:start(start), end(end)
		{
		}
	};

	template<typename T>
	struct Text
	{
		T tile;
		std::string text;

		Text(T tile, std::string text)
			:tile(tile), text(text)
		{
		}
	};

	class VisualLogBuilder : public IVisualLogBuilder
	{
	private:
		std::vector<Line<int3>> & mapLines;
		std::vector<Text<BattleHex>> & battleTexts;

	public:
		VisualLogBuilder(
			std::vector<Line<int3>> & mapLines,
			std::vector<Text<BattleHex>> & battleTexts)
			:mapLines(mapLines), battleTexts(battleTexts)
		{
		}

		void addLine(int3 start, int3 end) override
		{
			mapLines.emplace_back(start, end);
		}

		void addText(BattleHex tile, std::string text) override
		{
			battleTexts.emplace_back(tile, text);
		}
	};

private:
	std::map<std::string, std::vector<Line<int3>>> mapLines;
	std::map<std::string, std::vector<Text<BattleHex>>> battleTexts;
	std::mutex mutex;
	std::string keyToShow;

public:

	void updateWithLock(std::string channel, std::function<void(IVisualLogBuilder & logBuilder)> func);
	void visualize(IMapOverlayLogVisualizer & visulizer);
	void visualize(IBattleOverlayLogVisualizer & visulizer);
	void setKey(std::string key);
};

extern DLL_LINKAGE VisualLogger * logVisual;

VCMI_LIB_NAMESPACE_END
