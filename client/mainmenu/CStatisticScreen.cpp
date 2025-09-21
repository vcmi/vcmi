/*
 * CStatisticScreen.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "CStatisticScreen.h"

#include "../GameEngine.h"
#include "../gui/WindowHandler.h"
#include "../eventsSDL/InputHandler.h"
#include "../gui/Shortcut.h"

#include "../render/Graphics.h"
#include "../render/IImage.h"
#include "../render/IRenderHandler.h"

#include "../widgets/ComboBox.h"
#include "../widgets/Images.h"
#include "../widgets/GraphicalPrimitiveCanvas.h"
#include "../widgets/TextControls.h"
#include "../widgets/Buttons.h"
#include "../windows/InfoWindows.h"
#include "../widgets/Slider.h"

#include "../../lib/entities/ResourceTypeHandler.h"
#include "../../lib/gameState/GameStatistics.h"
#include "../../lib/gameState/CGameState.h"
#include "../../lib/texts/CGeneralTextHandler.h"
#include "../../lib/texts/TextOperations.h"
#include "../../lib/GameLibrary.h"

#include <vstd/DateUtils.h>

std::string CStatisticScreen::getDay(int d)
{
	return std::to_string(CGameState::getDate(d, Date::MONTH)) + "/" + std::to_string(CGameState::getDate(d, Date::WEEK)) + "/" + std::to_string(CGameState::getDate(d, Date::DAY_OF_WEEK));
}

CStatisticScreen::CStatisticScreen(const StatisticDataSet & stat)
	: CWindowObject(BORDERED), statistic(stat)
{
	OBJECT_CONSTRUCTION;
	pos = center(Rect(0, 0, 800, 600));
	filledBackground = std::make_shared<FilledTexturePlayerColored>(Rect(0, 0, pos.w, pos.h));
	filledBackground->setPlayerColor(PlayerColor(1));

	contentArea = Rect(10, 40, 780, 510);
	layout.emplace_back(std::make_shared<CLabel>(400, 20, FONT_BIG, ETextAlignment::CENTER, Colors::YELLOW, LIBRARY->generaltexth->translate("vcmi.statisticWindow.statistics")));
	layout.emplace_back(std::make_shared<TransparentFilledRectangle>(contentArea, ColorRGBA(0, 0, 0, 128), ColorRGBA(64, 80, 128, 255), 1));
	layout.emplace_back(std::make_shared<CButton>(Point(725, 558), AnimationPath::builtin("MUBCHCK"), CButton::tooltip(), [this](){ close(); }, EShortcut::GLOBAL_ACCEPT));

	buttonSelect = std::make_shared<CToggleButton>(Point(10, 564), AnimationPath::builtin("GSPBUT2"), CButton::tooltip(), [this](bool on){ onSelectButton(); });
	buttonSelect->setTextOverlay(LIBRARY->generaltexth->translate("vcmi.statisticWindow.selectView"), EFonts::FONT_SMALL, Colors::YELLOW);

	buttonCsvSave = std::make_shared<CToggleButton>(Point(150, 564), AnimationPath::builtin("GSPBUT2"), CButton::tooltip(), [this](bool on){ ENGINE->input().copyToClipBoard(statistic.toCsv("\t"));	});
	buttonCsvSave->setTextOverlay(LIBRARY->generaltexth->translate("vcmi.statisticWindow.tsvCopy"), EFonts::FONT_SMALL, Colors::YELLOW);

	mainContent = getContent(OVERVIEW, EGameResID::NONE);
}

void CStatisticScreen::onSelectButton()
{
	std::vector<std::string> texts;
	for(auto & val : contentInfo)
		texts.emplace_back(LIBRARY->generaltexth->translate(std::get<0>(val.second)));
	ENGINE->windows().createAndPushWindow<StatisticSelector>(texts, [this](int selectedIndex)
	{
		OBJECT_CONSTRUCTION;
		if(!std::get<1>(contentInfo[static_cast<Content>(selectedIndex)]))
			mainContent = getContent(static_cast<Content>(selectedIndex), EGameResID::NONE);
		else
		{
			auto content = static_cast<Content>(selectedIndex);
			auto possibleRes = LIBRARY->resourceTypeHandler->getAllObjects();
			std::vector<std::string> resourceText;
			for(const auto & res : possibleRes)
				resourceText.emplace_back(res.toResource()->getNameTranslated());
			
			ENGINE->windows().createAndPushWindow<StatisticSelector>(resourceText, [this, content, possibleRes](int index)
			{
				OBJECT_CONSTRUCTION;
				mainContent = getContent(content, possibleRes[index]);
			});
		}
	});
}

TData CStatisticScreen::extractData(const StatisticDataSet & stat, const ExtractFunctor & selector) const
{
	auto tmpData = stat.data;
	std::sort(tmpData.begin(), tmpData.end(), [](const StatisticDataSetEntry & v1, const StatisticDataSetEntry & v2){ return v1.player == v2.player ? v1.day < v2.day : v1.player < v2.player; });

	PlayerColor tmpColor = PlayerColor::NEUTRAL;
	std::vector<float> tmpColorSet;
	TData plotData;
	EPlayerStatus statusLastRound = EPlayerStatus::INGAME;
	for(const auto & val : tmpData)
	{
		if(tmpColor != val.player)
		{
			if(tmpColorSet.size())
			{
				plotData.push_back({graphics->playerColors[tmpColor.getNum()], std::vector<float>(tmpColorSet)});
				tmpColorSet.clear();
			}

			tmpColor = val.player;
		}
		if(val.status == EPlayerStatus::INGAME || (statusLastRound == EPlayerStatus::INGAME && val.status == EPlayerStatus::LOSER))
			tmpColorSet.emplace_back(selector(val));
		statusLastRound = val.status; //to keep at least one dataset after loose
	}
	if(tmpColorSet.size())
		plotData.push_back({graphics->playerColors[tmpColor.getNum()], std::vector<float>(tmpColorSet)});

	return plotData;
}

TIcons CStatisticScreen::extractIcons() const
{
	TIcons icons;

	auto tmpData = statistic.data;
	std::sort(tmpData.begin(), tmpData.end(), [](const StatisticDataSetEntry & v1, const StatisticDataSetEntry & v2){ return v1.player == v2.player ? v1.day < v2.day : v1.player < v2.player; });

	auto imageTown = ENGINE->renderHandler().loadImage(AnimationPath::builtin("cradvntr"), 3, 0, EImageBlitMode::COLORKEY);
	auto imageBattle = ENGINE->renderHandler().loadImage(AnimationPath::builtin("cradvntr"), 5, 0, EImageBlitMode::COLORKEY);
	auto imageDefeated = ENGINE->renderHandler().loadImage(AnimationPath::builtin("crcombat"), 0, 0, EImageBlitMode::COLORKEY);
	auto imageGrail = ENGINE->renderHandler().loadImage(AnimationPath::builtin("vwsymbol"), 2, 0, EImageBlitMode::COLORKEY);

	std::map<PlayerColor, bool> foundDefeated;
	std::map<PlayerColor, bool> foundGrail;

	for(const auto & val : tmpData)
	{
		if(val.eventCapturedTown)
			icons.push_back({ graphics->playerColors[val.player], val.day, imageTown, LIBRARY->generaltexth->translate("vcmi.statisticWindow.icon.townCaptured") });
		if(val.eventDefeatedStrongestHero)
			icons.push_back({ graphics->playerColors[val.player], val.day, imageBattle, LIBRARY->generaltexth->translate("vcmi.statisticWindow.icon.strongestHeroDefeated") });
		if(val.status == EPlayerStatus::LOSER && !foundDefeated[val.player])
		{
			foundDefeated[val.player] = true;
			icons.push_back({ graphics->playerColors[val.player], val.day, imageDefeated, LIBRARY->generaltexth->translate("vcmi.statisticWindow.icon.defeated") });
		}
		if(val.hasGrail && !foundGrail[val.player])
		{
			foundGrail[val.player] = true;
			icons.push_back({ graphics->playerColors[val.player], val.day, imageGrail, LIBRARY->generaltexth->translate("vcmi.statisticWindow.icon.grailFound") });
		}
	}

	return icons;
}

std::shared_ptr<CIntObject> CStatisticScreen::getContent(Content c, EGameResID res)
{
	TData plotData;
	TIcons icons = extractIcons();

	switch (c)
	{
	case OVERVIEW:
		return std::make_shared<OverviewPanel>(contentArea.resize(-15), LIBRARY->generaltexth->translate(std::get<0>(contentInfo[c])), statistic);
	
	case CHART_RESOURCES:
		plotData = extractData(statistic, [res](const StatisticDataSetEntry & val) -> float { return val.resources[res]; });
		return std::make_shared<LineChart>(contentArea.resize(-5), LIBRARY->generaltexth->translate(std::get<0>(contentInfo[c])) + " - " + res.toResource()->getNameTranslated(), plotData, icons, 0);
	
	case CHART_INCOME:
		plotData = extractData(statistic, [](const StatisticDataSetEntry & val) -> float { return val.income; });
		return std::make_shared<LineChart>(contentArea.resize(-5), LIBRARY->generaltexth->translate(std::get<0>(contentInfo[c])), plotData, icons, 0);
	
	case CHART_NUMBER_OF_HEROES:
		plotData = extractData(statistic, [](const StatisticDataSetEntry & val) -> float { return val.numberHeroes; });
		return std::make_shared<LineChart>(contentArea.resize(-5), LIBRARY->generaltexth->translate(std::get<0>(contentInfo[c])), plotData, icons, 0);
	
	case CHART_NUMBER_OF_TOWNS:
		plotData = extractData(statistic, [](const StatisticDataSetEntry & val) -> float { return val.numberTowns; });
		return std::make_shared<LineChart>(contentArea.resize(-5), LIBRARY->generaltexth->translate(std::get<0>(contentInfo[c])), plotData, icons, 0);
	
	case CHART_NUMBER_OF_ARTIFACTS:
		plotData = extractData(statistic, [](const StatisticDataSetEntry & val) -> float { return val.numberArtifacts; });
		return std::make_shared<LineChart>(contentArea.resize(-5), LIBRARY->generaltexth->translate(std::get<0>(contentInfo[c])), plotData, icons, 0);
	
	case CHART_NUMBER_OF_DWELLINGS:
		plotData = extractData(statistic, [](const StatisticDataSetEntry & val) -> float { return val.numberDwellings; });
		return std::make_shared<LineChart>(contentArea.resize(-5), LIBRARY->generaltexth->translate(std::get<0>(contentInfo[c])), plotData, icons, 0);
	
	case CHART_NUMBER_OF_MINES:
		plotData = extractData(statistic, [res](StatisticDataSetEntry val) -> float { return val.numMines[res]; });
		return std::make_shared<LineChart>(contentArea.resize(-5), LIBRARY->generaltexth->translate(std::get<0>(contentInfo[c])) + " - " + res.toResource()->getNameTranslated(), plotData, icons, 0);
	
	case CHART_ARMY_STRENGTH:
		plotData = extractData(statistic, [](const StatisticDataSetEntry & val) -> float { return val.armyStrength; });
		return std::make_shared<LineChart>(contentArea.resize(-5), LIBRARY->generaltexth->translate(std::get<0>(contentInfo[c])), plotData, icons, 0);
	
	case CHART_EXPERIENCE:
		plotData = extractData(statistic, [](const StatisticDataSetEntry & val) -> float { return val.totalExperience; });
		return std::make_shared<LineChart>(contentArea.resize(-5), LIBRARY->generaltexth->translate(std::get<0>(contentInfo[c])), plotData, icons, 0);
	
	case CHART_RESOURCES_SPENT_ARMY:
		plotData = extractData(statistic, [res](const StatisticDataSetEntry & val) -> float { return val.spentResourcesForArmy[res]; });
		return std::make_shared<LineChart>(contentArea.resize(-5), LIBRARY->generaltexth->translate(std::get<0>(contentInfo[c])) + " - " + res.toResource()->getNameTranslated(), plotData, icons, 0);
	
	case CHART_RESOURCES_SPENT_BUILDINGS:
		plotData = extractData(statistic, [res](const StatisticDataSetEntry & val) -> float { return val.spentResourcesForBuildings[res]; });
		return std::make_shared<LineChart>(contentArea.resize(-5), LIBRARY->generaltexth->translate(std::get<0>(contentInfo[c])) + " - " + res.toResource()->getNameTranslated(), plotData, icons, 0);
	
	case CHART_MAP_EXPLORED:
		plotData = extractData(statistic, [](const StatisticDataSetEntry & val) -> float { return val.mapExploredRatio; });
		return std::make_shared<LineChart>(contentArea.resize(-5), LIBRARY->generaltexth->translate(std::get<0>(contentInfo[c])), plotData, icons, 1);
	}

	return nullptr;
}

StatisticSelector::StatisticSelector(const std::vector<std::string> & texts, const std::function<void(int selectedIndex)> & cb)
	: CWindowObject(BORDERED | NEEDS_ANIMATED_BACKGROUND), texts(texts), cb(cb)
{
	OBJECT_CONSTRUCTION;
	pos = center(Rect(0, 0, 128 + 16, std::min(static_cast<int>(texts.size()), LINES) * 40));
	filledBackground = std::make_shared<FilledTexturePlayerColored>(Rect(0, 0, pos.w, pos.h));
	filledBackground->setPlayerColor(PlayerColor(1));

	slider = std::make_shared<CSlider>(Point(pos.w - 16, 0), pos.h, [this](int to){ update(to); redraw(); }, LINES, texts.size(), 0, Orientation::VERTICAL, CSlider::BLUE);
	slider->setPanningStep(40);
	slider->setScrollBounds(Rect(-pos.w + slider->pos.w, 0, pos.w, pos.h));

	update(0);
}

void StatisticSelector::update(int to)
{
	OBJECT_CONSTRUCTION;
	buttons.clear();
	for(int i = to; i < LINES + to; i++)
	{
		if(i>=texts.size())
			continue;

		auto button = std::make_shared<CToggleButton>(Point(0, 10 + (i - to) * 40), AnimationPath::builtin("GSPBUT2"), CButton::tooltip(), [this, i](bool on){ close(); cb(i); });
		button->setTextOverlay(texts[i], EFonts::FONT_SMALL, Colors::WHITE);
		buttons.emplace_back(button);
	}
}

OverviewPanel::OverviewPanel(Rect position, std::string title, const StatisticDataSet & stat)
	: CIntObject(), data(stat)
{
	OBJECT_CONSTRUCTION;

	pos = position + pos.topLeft();

	layout.emplace_back(std::make_shared<CLabel>(pos.w / 2, 10, FONT_MEDIUM, ETextAlignment::CENTER, Colors::WHITE, title));

	canvas = std::make_shared<GraphicalPrimitiveCanvas>(Rect(0, Y_OFFS, pos.w - 16, pos.h - Y_OFFS));

	dataExtract = {
		{
			LIBRARY->generaltexth->translate("vcmi.statisticWindow.param.playerName"), [this](PlayerColor color){
				return playerDataFilter(color).front().playerName;
			}
		},
		{
			LIBRARY->generaltexth->translate("vcmi.statisticWindow.param.daysSurvived"), [this](PlayerColor color){
				auto playerData = playerDataFilter(color);
				for(int i = 0; i < playerData.size(); i++)
					if(playerData[i].status == EPlayerStatus::LOSER)
						return CStatisticScreen::getDay(i + 1);
				return CStatisticScreen::getDay(playerData.size());
			}
		},
		{
			LIBRARY->generaltexth->translate("vcmi.statisticWindow.param.maxHeroLevel"), [this](PlayerColor color){
				int maxLevel = 0;
				for(const auto & val : playerDataFilter(color))
					if(maxLevel < val.maxHeroLevel)
						maxLevel = val.maxHeroLevel;
				return std::to_string(maxLevel);
			}
		},
		{
			LIBRARY->generaltexth->translate("vcmi.statisticWindow.param.battleWinRatioHero"), [this](PlayerColor color){
				auto val = playerDataFilter(color).back();
				if(!val.numBattlesPlayer)
					return std::string("");
				float tmp = (static_cast<float>(val.numWinBattlesPlayer) / static_cast<float>(val.numBattlesPlayer)) * 100;
				return std::to_string(static_cast<int>(tmp)) + " %";
			}
		},
		{
			LIBRARY->generaltexth->translate("vcmi.statisticWindow.param.battleWinRatioNeutral"), [this](PlayerColor color){
				auto val = playerDataFilter(color).back();
				if(!val.numWinBattlesNeutral)
					return std::string("");
				float tmp = (static_cast<float>(val.numWinBattlesNeutral) / static_cast<float>(val.numBattlesNeutral)) * 100;
				return std::to_string(static_cast<int>(tmp)) + " %";
			}
		},
		{
			LIBRARY->generaltexth->translate("vcmi.statisticWindow.param.battlesHero"), [this](PlayerColor color){
				auto val = playerDataFilter(color).back();
				return std::to_string(val.numBattlesPlayer);
			}
		},
		{
			LIBRARY->generaltexth->translate("vcmi.statisticWindow.param.battlesNeutral"), [this](PlayerColor color){
				auto val = playerDataFilter(color).back();
				return std::to_string(val.numBattlesNeutral);
			}
		},
		{
			LIBRARY->generaltexth->translate("vcmi.statisticWindow.param.obeliskVisited"), [this](PlayerColor color){
				auto val = playerDataFilter(color).back();
				return std::to_string(static_cast<int>(val.obeliskVisitedRatio * 100)) + " %";
			}
		},
		{
			LIBRARY->generaltexth->translate("vcmi.statisticWindow.param.maxArmyStrength"), [this](PlayerColor color){
				int maxArmyStrength = 0;
				for(const auto & val : playerDataFilter(color))
					if(maxArmyStrength < val.armyStrength)
						maxArmyStrength = val.armyStrength;
				return TextOperations::formatMetric(maxArmyStrength, 6);
			}
		},
		{
			LIBRARY->generaltexth->translate("vcmi.statisticWindow.param.tradeVolume") + " - " + GameResID(EGameResID::GOLD).toResource()->getNameTranslated(), [this](PlayerColor color){
				auto val = playerDataFilter(color).back();
				return std::to_string(val.tradeVolume[EGameResID::GOLD]);
			}
		},
		{
			LIBRARY->generaltexth->translate("vcmi.statisticWindow.param.tradeVolume") + " - " + GameResID(EGameResID::WOOD).toResource()->getNameTranslated(), [this](PlayerColor color){
				auto val = playerDataFilter(color).back();
				return std::to_string(val.tradeVolume[EGameResID::WOOD]);
			}
		},
		{
			LIBRARY->generaltexth->translate("vcmi.statisticWindow.param.tradeVolume") + " - " + GameResID(EGameResID::MERCURY).toResource()->getNameTranslated(), [this](PlayerColor color){
				auto val = playerDataFilter(color).back();
				return std::to_string(val.tradeVolume[EGameResID::MERCURY]);
			}
		},
		{
			LIBRARY->generaltexth->translate("vcmi.statisticWindow.param.tradeVolume") + " - " + GameResID(EGameResID::ORE).toResource()->getNameTranslated(), [this](PlayerColor color){
				auto val = playerDataFilter(color).back();
				return std::to_string(val.tradeVolume[EGameResID::ORE]);
			}
		},
		{
			LIBRARY->generaltexth->translate("vcmi.statisticWindow.param.tradeVolume") + " - " + GameResID(EGameResID::SULFUR).toResource()->getNameTranslated(), [this](PlayerColor color){
				auto val = playerDataFilter(color).back();
				return std::to_string(val.tradeVolume[EGameResID::SULFUR]);
			}
		},
		{
			LIBRARY->generaltexth->translate("vcmi.statisticWindow.param.tradeVolume") + " - " + GameResID(EGameResID::CRYSTAL).toResource()->getNameTranslated(), [this](PlayerColor color){
				auto val = playerDataFilter(color).back();
				return std::to_string(val.tradeVolume[EGameResID::CRYSTAL]);
			}
		},
		{
			LIBRARY->generaltexth->translate("vcmi.statisticWindow.param.tradeVolume") + " - " + GameResID(EGameResID::GEMS).toResource()->getNameTranslated(), [this](PlayerColor color){
				auto val = playerDataFilter(color).back();
				return std::to_string(val.tradeVolume[EGameResID::GEMS]);
			}
		},
	};

	int usedLines = dataExtract.size();

	slider = std::make_shared<CSlider>(Point(pos.w - 16, Y_OFFS), pos.h - Y_OFFS, [this](int to){ update(to); setRedrawParent(true); redraw(); }, LINES - 1, usedLines, 0, Orientation::VERTICAL, CSlider::BLUE);
	slider->setPanningStep(canvas->pos.h / LINES);
	slider->setScrollBounds(Rect(-pos.w + slider->pos.w, 0, pos.w, canvas->pos.h));

	fieldSize = Point(canvas->pos.w / (graphics->playerColors.size() + 2), canvas->pos.h / LINES);
	for(int x = 0; x < graphics->playerColors.size() + 1; x++)
		for(int y = 0; y < LINES; y++)
		{
			int xStart = (x + (x == 0 ? 0 : 1)) * fieldSize.x;
			int yStart = y * fieldSize.y;
			if(x == 0 || y == 0)
				canvas->addBox(Point(xStart, yStart), Point(x == 0 ? 2 * fieldSize.x : fieldSize.x, fieldSize.y), ColorRGBA(0, 0, 0, 100));
			canvas->addRectangle(Point(xStart, yStart), Point(x == 0 ? 2 * fieldSize.x : fieldSize.x, fieldSize.y), ColorRGBA(127, 127, 127, 255));
		}

	update(0);
}

std::vector<StatisticDataSetEntry> OverviewPanel::playerDataFilter(PlayerColor color)
{
	std::vector<StatisticDataSetEntry> tmpData;
	std::copy_if(data.data.begin(), data.data.end(), std::back_inserter(tmpData), [color](const StatisticDataSetEntry & e){ return e.player == color; });
	return tmpData;
}

void OverviewPanel::update(int to)
{
	OBJECT_CONSTRUCTION;

	content.clear();
	for(int y = to; y < LINES - 1 + to; y++)
	{
		if(y >= dataExtract.size())
			continue;

		for(int x = 0; x < PlayerColor::PLAYER_LIMIT_I + 1; x++)
		{
			if(y == to && x < PlayerColor::PLAYER_LIMIT_I)
				content.emplace_back(std::make_shared<CAnimImage>(AnimationPath::builtin("ITGFLAGS"), x, 0, 180 + x * fieldSize.x, 35));
			int xStart = (x + (x == 0 ? 0 : 1)) * fieldSize.x + (x == 0 ? fieldSize.x : (fieldSize.x / 2));
			int yStart = Y_OFFS + (y + 1 - to) * fieldSize.y + (fieldSize.y / 2);
			PlayerColor tmpColor(x - 1);
			if(playerDataFilter(tmpColor).size() || x == 0)
				content.emplace_back(std::make_shared<CLabel>(xStart, yStart, FONT_TINY, ETextAlignment::CENTER, Colors::WHITE, (x == 0 ? dataExtract[y].first : dataExtract[y].second(tmpColor)), x == 0 ? (fieldSize.x * 2) : fieldSize.x));
		}
	}
}

int computeGridStep(int maxAmount, int linesLimit)
{
	for (int lineInterval = 1;;lineInterval *= 10)
	{
		for (int factor : { 1, 2, 5 } )
		{
			int lineIntervalToTest = lineInterval * factor;
			if (maxAmount / lineIntervalToTest <= linesLimit)
				return lineIntervalToTest;
		}
	}
}

LineChart::LineChart(Rect position, std::string title, TData data, TIcons icons, float maxY)
	: CIntObject(), maxVal(0), maxDay(0), data(data)
{
	OBJECT_CONSTRUCTION;

	addUsedEvents(LCLICK | MOVE | GESTURE);

	pos = position + pos.topLeft();

	layout.emplace_back(std::make_shared<CLabel>(pos.w / 2, 20, FONT_MEDIUM, ETextAlignment::CENTER, Colors::WHITE, title));

	chartArea = pos.resize(-50);
	chartArea.moveTo(Point(50, 50));

	canvas = std::make_shared<GraphicalPrimitiveCanvas>(Rect(0, 0, pos.w, pos.h));

	statusBar = CGStatusBar::create(0, 0, ImagePath::builtin("radialMenu/statusBar"));
	static_cast<std::shared_ptr<CIntObject>>(statusBar)->setEnabled(false);

	// additional calculations
	bool skipMaxValCalc = maxY > 0;
	maxVal = maxY;
	for(const auto & line : data)
	{
		for(auto & val : line.second)
			if(maxVal < val && !skipMaxValCalc)
				maxVal = val;
		if(maxDay < line.second.size())
			maxDay = line.second.size();
	}

	//calculate nice maxVal
	int gridLineCount = 10;
	int gridStep = computeGridStep(maxVal, gridLineCount);
	niceMaxVal = gridStep * std::ceil(maxVal / gridStep);
	niceMaxVal = std::max(1, niceMaxVal); // avoid zero size Y axis (if all values are 0)

	// draw grid (vertical lines)
	int dayGridInterval = maxDay < 700 ? 7 : 28;
	if(maxDay > 1)
	{
		for(const auto & line : data)
		{
			for(int i = 0; i < line.second.size(); i += dayGridInterval)
			{
				Point p = getPoint(i, line.second) + chartArea.topLeft();
				canvas->addLine(Point(p.x, chartArea.topLeft().y), Point(p.x, chartArea.topLeft().y + chartArea.h), ColorRGBA(70, 70, 70));
			}
		}
	}

	// draw grid (horizontal lines)
	if(maxVal > 0)
	{
		int gridStepPx = int((static_cast<float>(chartArea.h) / niceMaxVal) * gridStep);
		for(int i = 0; i < std::ceil(maxVal / gridStep) + 1; i++)
		{
			canvas->addLine(chartArea.topLeft() + Point(0, chartArea.h - gridStepPx * i), chartArea.topLeft() + Point(chartArea.w, chartArea.h - gridStepPx * i), ColorRGBA(70, 70, 70));
			layout.emplace_back(std::make_shared<CLabel>(chartArea.topLeft().x - 5, chartArea.topLeft().y + 10 + chartArea.h - gridStepPx * i, FONT_SMALL, ETextAlignment::CENTERRIGHT, Colors::WHITE, TextOperations::formatMetric(i * gridStep, 5)));
		}
	}

	// draw
	for(const auto & line : data)
	{
		Point lastPoint(-1, -1);
		for(int i = 0; i < line.second.size(); i++)
		{
			Point p = getPoint(i, line.second) + chartArea.topLeft();

			if(lastPoint.x != -1)
				canvas->addLine(lastPoint, p, line.first);
			
			// icons
			for(auto & icon : icons)
				if(std::get<0>(icon) == line.first && std::get<1>(icon) == i + 1) // color && day
				{
					auto img = std::get<2>(icon);
					Point imgPos(p.x - (img->contentRect().w / 2) - img->contentRect().x, p.y - (img->contentRect().h / 2) - img->contentRect().y);
					pictures.emplace_back(std::make_shared<CPicture>(img, imgPos));
					pictures.back()->addRClickCallback([icon](){ CRClickPopup::createAndPush(std::get<3>(icon)); });
				}

			lastPoint = p;
		}
	}

	// Axis
	canvas->addLine(chartArea.topLeft() + Point(0, -10), chartArea.topLeft() + Point(0, chartArea.h + 10), Colors::WHITE);
	canvas->addLine(chartArea.topLeft() + Point(-10, chartArea.h), chartArea.topLeft() + Point(chartArea.w + 10, chartArea.h), Colors::WHITE);

	Point p = chartArea.topLeft() + Point(chartArea.w + 10, chartArea.h + 10);
	layout.emplace_back(std::make_shared<CLabel>(p.x, p.y, FONT_SMALL, ETextAlignment::CENTER, Colors::WHITE, CStatisticScreen::getDay(maxDay)));
	p = chartArea.bottomLeft() + Point(chartArea.w / 2, + 20);
	layout.emplace_back(std::make_shared<CLabel>(p.x, p.y, FONT_MEDIUM, ETextAlignment::CENTER, Colors::WHITE, LIBRARY->generaltexth->translate("core.genrltxt.64")));
}

Point LineChart::getPoint(int i, std::vector<float> data)
{
	float x = (static_cast<float>(chartArea.w) / static_cast<float>(maxDay - 1)) * static_cast<float>(i);
	float y = static_cast<float>(chartArea.h) - (static_cast<float>(chartArea.h) / niceMaxVal) * data[i];
	return Point(x, y);
};

void LineChart::updateStatusBar(const Point & cursorPosition)
{
	OBJECT_CONSTRUCTION;

	Rect r(pos.x + chartArea.x, pos.y + chartArea.y, chartArea.w, chartArea.h);
	Point curPos = cursorPosition;
	if(r.isInside(curPos))
	{
		std::vector<std::pair<int, Point>> points;
		for(const auto & line : data)
		{
			for(int i = 0; i < line.second.size(); i++)
			{
				Point p = getPoint(i, line.second) + chartArea.topLeft();
				int len = Point(curPos.x - p.x - pos.x, curPos.y - p.y - pos.y).length();
				points.push_back(std::make_pair(len, p));
			}
		}
		std::sort(points.begin(), points.end(), [](const auto &a, const auto &b) { return a.first < b.first; });
		if(points.size() && points[0].first < 15)
		{
			// Snap in with marker for nearest point
			hoverMarker = std::make_shared<TransparentFilledRectangle>(Rect(points[0].second - Point(3, 3), Point(6, 6)), Colors::ORANGE);
			curPos = points[0].second + pos;
		}
		else
			hoverMarker.reset();

		float x = (static_cast<float>(maxDay - 1) / static_cast<float>(chartArea.w)) * (static_cast<float>(curPos.x) - static_cast<float>(r.x)) + 1.0f;
		float y = niceMaxVal - (niceMaxVal / static_cast<float>(chartArea.h)) * (static_cast<float>(curPos.y) - static_cast<float>(r.y));

		statusBar->write(LIBRARY->generaltexth->translate("core.genrltxt.64") + ": " + CStatisticScreen::getDay(x) + "   " + LIBRARY->generaltexth->translate("vcmi.statisticWindow.value") + ": " + (static_cast<int>(y) > 0 ? std::to_string(static_cast<int>(y)) : std::to_string(y)));
	}

	statusBar->setEnabled(r.resize(1).isInside(curPos));
	statusBar->moveTo(curPos + Point(-statusBar->pos.w / 2, 20));
	statusBar->fitToRect(pos, 10);

	setRedrawParent(true);
	redraw();
}

void LineChart::mouseMoved(const Point & cursorPosition, const Point & lastUpdateDistance)
{
	updateStatusBar(cursorPosition);
}

void LineChart::gesturePanning(const Point & initialPosition, const Point & currentPosition, const Point & lastUpdateDistance)
{
	updateStatusBar(currentPosition);
}
