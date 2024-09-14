/*
 * CStatisticScreen.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once
#include "../windows/CWindowObject.h"
#include "../../lib/gameState/GameStatistics.h"

class FilledTexturePlayerColored;
class CToggleButton;
class GraphicalPrimitiveCanvas;
class LineChart;
class CGStatusBar;
class ComboBox;
class CSlider;
class IImage;
class CPicture;

using TData = std::vector<std::pair<ColorRGBA, std::vector<float>>>;
using TIcons = std::vector<std::tuple<ColorRGBA, int, std::shared_ptr<IImage>, std::string>>; // Color, Day, Image, Helptext

class CStatisticScreen : public CWindowObject
{
	enum Content {
		OVERVIEW,
		CHART_RESOURCES,
		CHART_INCOME,
		CHART_NUMBER_OF_HEROES,
		CHART_NUMBER_OF_TOWNS,
		CHART_NUMBER_OF_ARTIFACTS,
		CHART_NUMBER_OF_DWELLINGS,
		CHART_NUMBER_OF_MINES,
		CHART_ARMY_STRENGTH,
		CHART_EXPERIENCE,
		CHART_RESOURCES_SPENT_ARMY,
		CHART_RESOURCES_SPENT_BUILDINGS,
		CHART_MAP_EXPLORED,
	};
	std::map<Content, std::tuple<std::string, bool>> contentInfo = { // tuple: textid, resource selection needed
		{ OVERVIEW,                        { "vcmi.statisticWindow.title.overview",                false } },
		{ CHART_RESOURCES,                 { "vcmi.statisticWindow.title.resources",               true  } },
		{ CHART_INCOME,                    { "vcmi.statisticWindow.title.income",                  false } },
		{ CHART_NUMBER_OF_HEROES,          { "vcmi.statisticWindow.title.numberOfHeroes",          false } },
		{ CHART_NUMBER_OF_TOWNS,           { "vcmi.statisticWindow.title.numberOfTowns",           false } },
		{ CHART_NUMBER_OF_ARTIFACTS,       { "vcmi.statisticWindow.title.numberOfArtifacts",       false } },
		{ CHART_NUMBER_OF_DWELLINGS,       { "vcmi.statisticWindow.title.numberOfDwellings",       false } },
		{ CHART_NUMBER_OF_MINES,           { "vcmi.statisticWindow.title.numberOfMines",           true  } },
		{ CHART_ARMY_STRENGTH,             { "vcmi.statisticWindow.title.armyStrength",            false } },
		{ CHART_EXPERIENCE,                { "vcmi.statisticWindow.title.experience",              false } },
		{ CHART_RESOURCES_SPENT_ARMY,      { "vcmi.statisticWindow.title.resourcesSpentArmy",      true  } },
		{ CHART_RESOURCES_SPENT_BUILDINGS, { "vcmi.statisticWindow.title.resourcesSpentBuildings", true  } },
		{ CHART_MAP_EXPLORED,              { "vcmi.statisticWindow.title.mapExplored",             false } },
	};

	std::shared_ptr<FilledTexturePlayerColored> filledBackground;
	std::vector<std::shared_ptr<CIntObject>> layout;
	std::shared_ptr<CToggleButton> buttonCsvSave;
	std::shared_ptr<CToggleButton> buttonSelect;
	StatisticDataSet statistic;
	std::shared_ptr<CIntObject> mainContent;
	Rect contentArea;

	using ExtractFunctor = std::function<float(StatisticDataSetEntry val)>;
	TData extractData(const StatisticDataSet & stat, const ExtractFunctor & selector) const;
	TIcons extractIcons() const;
	std::shared_ptr<CIntObject> getContent(Content c, EGameResID res);
	void onSelectButton();
public:
	CStatisticScreen(const StatisticDataSet & stat);
	static std::string getDay(int day);
};

class StatisticSelector : public CWindowObject
{
	std::shared_ptr<FilledTexturePlayerColored> filledBackground;
	std::vector<std::shared_ptr<CToggleButton>> buttons;
	std::shared_ptr<CSlider> slider;

	const int LINES = 10;

	std::vector<std::string> texts;
	std::function<void(int selectedIndex)> cb;

	void update(int to);
public:
	StatisticSelector(const std::vector<std::string> & texts, const std::function<void(int selectedIndex)> & cb);
};

class OverviewPanel : public CIntObject
{
	std::shared_ptr<GraphicalPrimitiveCanvas> canvas;
	std::vector<std::shared_ptr<CIntObject>> layout;
	std::vector<std::shared_ptr<CIntObject>> content;
	std::shared_ptr<CSlider> slider;

	Point fieldSize;
	StatisticDataSet data;

	std::vector<std::pair<std::string, std::function<std::string(PlayerColor color)>>> dataExtract;

	const int LINES = 15;
	const int Y_OFFS = 30;

	std::vector<StatisticDataSetEntry> playerDataFilter(PlayerColor color);
	void update(int to);
public:
	OverviewPanel(Rect position, std::string title, const StatisticDataSet & stat);
};

class LineChart : public CIntObject
{
	std::shared_ptr<GraphicalPrimitiveCanvas> canvas;
	std::vector<std::shared_ptr<CIntObject>> layout;
	std::shared_ptr<CGStatusBar> statusBar;
	std::vector<std::shared_ptr<CPicture>> pictures;

	Rect chartArea;
	float maxVal;
	int niceMaxVal;
	int maxDay;

	void updateStatusBar(const Point & cursorPosition);
public:
	LineChart(Rect position, std::string title, TData data, TIcons icons, float maxY);

	void mouseMoved(const Point & cursorPosition, const Point & lastUpdateDistance) override;
	void gesturePanning(const Point & initialPosition, const Point & currentPosition, const Point & lastUpdateDistance) override;
};
