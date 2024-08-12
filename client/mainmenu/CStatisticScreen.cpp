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
#include "../CGameInfo.h"

#include "../gui/CGuiHandler.h"
#include "../gui/WindowHandler.h"
#include "../gui/Shortcut.h"

#include "../render/Graphics.h"

#include "../widgets/Images.h"
#include "../widgets/GraphicalPrimitiveCanvas.h"
#include "../widgets/TextControls.h"
#include "../widgets/Buttons.h"
#include "../windows/InfoWindows.h"

#include "../../lib/gameState/GameStatistics.h"
#include "../../lib/texts/CGeneralTextHandler.h"

#include <vstd/DateUtils.h>

CStatisticScreen::CStatisticScreen(StatisticDataSet stat)
	: CWindowObject(BORDERED), statistic(stat)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL_NO_DISPOSE;
	pos = center(Rect(0, 0, 800, 600));
	filledBackground = std::make_shared<FilledTexturePlayerColored>(ImagePath::builtin("DiBoxBck"), Rect(0, 0, pos.w, pos.h));
	filledBackground->setPlayerColor(PlayerColor(1));

	auto contentArea = Rect(10, 40, 780, 510);
	layout.push_back(std::make_shared<CLabel>(400, 20, FONT_BIG, ETextAlignment::CENTER, Colors::YELLOW, CGI->generaltexth->translate("vcmi.statisticWindow.statistic")));
	layout.push_back(std::make_shared<TransparentFilledRectangle>(contentArea, ColorRGBA(0, 0, 0, 64), ColorRGBA(64, 80, 128, 255), 1));
	layout.push_back(std::make_shared<CButton>(Point(725, 564), AnimationPath::builtin("MUBCHCK"), CButton::tooltip(), [this](){ close(); }, EShortcut::GLOBAL_ACCEPT));

	buttonCsvSave = std::make_shared<CToggleButton>(Point(10, 564), AnimationPath::builtin("GSPBUT2"), CButton::tooltip(), [this](bool on){
		std::string path = statistic.writeCsv();
		CInfoWindow::showInfoDialog(CGI->generaltexth->translate("vcmi.statisticWindow.csvSaved") + "\n\n" + path, {});
	});
	buttonCsvSave->setTextOverlay(CGI->generaltexth->translate("vcmi.statisticWindow.csvSave"), EFonts::FONT_SMALL, Colors::YELLOW);

	auto plotData = extractData(stat, [](StatisticDataSetEntry val){ return val.resources[EGameResID::GOLD]; });
	chart = std::make_shared<LineChart>(contentArea.resize(-5), "Total Gold", plotData);
}

std::map<ColorRGBA, std::vector<float>> CStatisticScreen::extractData(StatisticDataSet stat, std::function<float(StatisticDataSetEntry val)> selector)
{
	auto tmpData = stat.data;
	std::sort(tmpData.begin(), tmpData.end(), [](StatisticDataSetEntry v1, StatisticDataSetEntry v2){ return v1.player == v2.player ? v1.day < v2.day : v1.player < v2.player; });

	PlayerColor tmpColor = PlayerColor::NEUTRAL;
	std::vector<float> tmpColorSet;
	std::map<ColorRGBA, std::vector<float>> plotData;
	for(auto & val : tmpData)
	{
		if(tmpColor != val.player)
		{
			if(tmpColorSet.size())
			{
				plotData.emplace(graphics->playerColors[tmpColor.getNum()], std::vector<float>(tmpColorSet));
				tmpColorSet.clear();
			}

			tmpColor = val.player;
		}
		if(val.status == EPlayerStatus::INGAME)
			tmpColorSet.push_back(selector(val));
	}
	if(tmpColorSet.size())
		plotData.emplace(graphics->playerColors[tmpColor.getNum()], std::vector<float>(tmpColorSet));

	return plotData;
}

LineChart::LineChart(Rect position, std::string title, std::map<ColorRGBA, std::vector<float>> data)
	: CIntObject(), maxVal(0), maxDay(0)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL_NO_DISPOSE;

	addUsedEvents(LCLICK | MOVE);

	pos = position + pos.topLeft();

	chartArea = pos.resize(-50);
	chartArea.moveTo(Point(50, 50));

	layout.push_back(std::make_shared<CLabel>(pos.w / 2, 20, FONT_MEDIUM, ETextAlignment::CENTER, Colors::WHITE, title));

	canvas = std::make_shared<GraphicalPrimitiveCanvas>(Rect(0, 0, pos.w, pos.h));

	statusBar = CGStatusBar::create(0, 0, ImagePath::builtin("radialMenu/statusBar"));
	statusBar->setEnabled(false);

	// additional calculations
	for(auto & line : data)
	{
		for(auto & val : line.second)
			if(maxVal < val)
				maxVal = val;
		if(maxDay < line.second.size())
			maxDay = line.second.size();
	}

	// draw
	for(auto & line : data)
	{
		Point lastPoint = Point(-1, -1);
		for(int i = 0; i < line.second.size(); i++)
		{
			float x = ((float)chartArea.w / (float)(maxDay-1)) * (float)i;
			float y = (float)chartArea.h - ((float)chartArea.h / maxVal) * line.second[i];
			Point p = Point(x, y) + chartArea.topLeft();

			if(lastPoint.x != -1)
				canvas->addLine(lastPoint, p, line.first);

			lastPoint = p;
		}
	}

	// Axis
	canvas->addLine(chartArea.topLeft() + Point(0, -10), chartArea.topLeft() + Point(0, chartArea.h + 10), Colors::WHITE);
	canvas->addLine(chartArea.topLeft() + Point(-10, chartArea.h), chartArea.topLeft() + Point(chartArea.w + 10, chartArea.h), Colors::WHITE);

	Point p = chartArea.topLeft() + Point(-5, chartArea.h + 10);
	layout.push_back(std::make_shared<CLabel>(p.x, p.y, FONT_SMALL, ETextAlignment::CENTERRIGHT, Colors::WHITE, "0"));
	p = chartArea.topLeft() + Point(chartArea.w + 10, chartArea.h + 10);
	layout.push_back(std::make_shared<CLabel>(p.x, p.y, FONT_SMALL, ETextAlignment::CENTER, Colors::WHITE, std::to_string(maxDay)));
	p = chartArea.topLeft() + Point(-5, -10);
	layout.push_back(std::make_shared<CLabel>(p.x, p.y, FONT_SMALL, ETextAlignment::CENTERRIGHT, Colors::WHITE, std::to_string((int)maxVal)));
}

void LineChart::updateStatusBar(const Point & cursorPosition)
{
	statusBar->moveTo(cursorPosition + Point(-statusBar->pos.w / 2, 20));
	Rect r(pos.x + chartArea.x, pos.y + chartArea.y, chartArea.w, chartArea.h);
	statusBar->setEnabled(r.isInside(cursorPosition));
	if(r.isInside(cursorPosition))
	{
		float x = ((float)maxDay / (float)chartArea.w) * ((float)cursorPosition.x - (float)r.x) + 1.0f;
		float y = maxVal - ((float)maxVal / (float)chartArea.h) * ((float)cursorPosition.y - (float)r.y);
		statusBar->write(CGI->generaltexth->translate("core.genrltxt.64") + ": " + std::to_string((int)x) + "   " + CGI->generaltexth->translate("vcmi.statisticWindow.value") + ": " + std::to_string((int)y));
	}
	GH.windows().totalRedraw();
}

void LineChart::mouseMoved(const Point & cursorPosition, const Point & lastUpdateDistance)
{
	updateStatusBar(cursorPosition);
}

void LineChart::clickPressed(const Point & cursorPosition)
{
	updateStatusBar(cursorPosition);
}
