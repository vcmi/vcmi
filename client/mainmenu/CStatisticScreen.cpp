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

#include "../widgets/ComboBox.h"
#include "../widgets/Images.h"
#include "../widgets/GraphicalPrimitiveCanvas.h"
#include "../widgets/TextControls.h"
#include "../widgets/Buttons.h"
#include "../windows/InfoWindows.h"
#include "../widgets/Slider.h"

#include "../../lib/gameState/GameStatistics.h"
#include "../../lib/texts/CGeneralTextHandler.h"

#include <vstd/DateUtils.h>

CStatisticScreen::CStatisticScreen(StatisticDataSet stat)
	: CWindowObject(BORDERED), statistic(stat)
{
	OBJECT_CONSTRUCTION;
	pos = center(Rect(0, 0, 800, 600));
	filledBackground = std::make_shared<FilledTexturePlayerColored>(ImagePath::builtin("DiBoxBck"), Rect(0, 0, pos.w, pos.h));
	filledBackground->setPlayerColor(PlayerColor(1));

	contentArea = Rect(10, 40, 780, 510);
	layout.push_back(std::make_shared<CLabel>(400, 20, FONT_BIG, ETextAlignment::CENTER, Colors::YELLOW, CGI->generaltexth->translate("vcmi.statisticWindow.statistic")));
	layout.push_back(std::make_shared<TransparentFilledRectangle>(contentArea, ColorRGBA(0, 0, 0, 64), ColorRGBA(64, 80, 128, 255), 1));
	layout.push_back(std::make_shared<CButton>(Point(725, 564), AnimationPath::builtin("MUBCHCK"), CButton::tooltip(), [this](){ close(); }, EShortcut::GLOBAL_ACCEPT));

	buttonSelect = std::make_shared<CToggleButton>(Point(10, 564), AnimationPath::builtin("GSPBUT2"), CButton::tooltip(), [this](bool on){
		std::vector<std::string> texts;
		for(auto & val : contentInfo)
			texts.push_back(CGI->generaltexth->translate(std::get<0>(val.second)));
		GH.windows().createAndPushWindow<StatisticSelector>(texts, [this](int selectedIndex)
		{
			OBJECT_CONSTRUCTION;
			if(!std::get<1>(contentInfo[(Content)selectedIndex]))
				mainContent = getContent((Content)selectedIndex, EGameResID::NONE);
			else
			{
				auto content = (Content)selectedIndex;
				auto possibleRes = std::vector<EGameResID>{EGameResID::GOLD, EGameResID::WOOD, EGameResID::MERCURY, EGameResID::ORE, EGameResID::SULFUR, EGameResID::CRYSTAL, EGameResID::GEMS};
				std::vector<std::string> resourceText;
				for(auto & res : possibleRes)
					resourceText.push_back(CGI->generaltexth->translate(TextIdentifier("core.restypes", res.getNum()).get()));
				
				GH.windows().createAndPushWindow<StatisticSelector>(resourceText, [this, content, possibleRes](int selectedIndex)
				{
					OBJECT_CONSTRUCTION;
					mainContent = getContent(content, possibleRes[selectedIndex]);
				});
			}
		});
	});
	buttonSelect->setTextOverlay(CGI->generaltexth->translate("vcmi.statisticWindow.selectView"), EFonts::FONT_SMALL, Colors::YELLOW);

	buttonCsvSave = std::make_shared<CToggleButton>(Point(150, 564), AnimationPath::builtin("GSPBUT2"), CButton::tooltip(), [this](bool on){
		std::string path = statistic.writeCsv();
		CInfoWindow::showInfoDialog(CGI->generaltexth->translate("vcmi.statisticWindow.csvSaved") + "\n\n" + path, {});
	});
	buttonCsvSave->setTextOverlay(CGI->generaltexth->translate("vcmi.statisticWindow.csvSave"), EFonts::FONT_SMALL, Colors::YELLOW);

	mainContent = getContent(OVERVIEW, EGameResID::NONE);
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

std::shared_ptr<CIntObject> CStatisticScreen::getContent(Content c, EGameResID res)
{
	switch (c)
	{
	case OVERVIEW:
		return std::make_shared<OverviewPanel>(contentArea.resize(-15), CGI->generaltexth->translate(std::get<0>(contentInfo[c])), statistic);
	
	case CHART_RESOURCES:
		auto plotData = extractData(statistic, [res](StatisticDataSetEntry val) -> float { return val.resources[res]; });
		return std::make_shared<LineChart>(contentArea.resize(-5), CGI->generaltexth->translate(std::get<0>(contentInfo[c])), plotData);
	}

	return nullptr;
}

StatisticSelector::StatisticSelector(std::vector<std::string> texts, std::function<void(int selectedIndex)> cb)
	: CWindowObject(BORDERED), texts(texts), cb(cb)
{
	OBJECT_CONSTRUCTION;
	pos = center(Rect(0, 0, 128 + 16, std::min((int)texts.size(), LINES) * 40));
	filledBackground = std::make_shared<FilledTexturePlayerColored>(ImagePath::builtin("DiBoxBck"), Rect(0, 0, pos.w, pos.h));
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
		buttons.push_back(button);
	}
}

OverviewPanel::OverviewPanel(Rect position, std::string title, StatisticDataSet data)
	: CIntObject()
{
	OBJECT_CONSTRUCTION;

	pos = position + pos.topLeft();

	layout.push_back(std::make_shared<CLabel>(pos.w / 2, 10, FONT_MEDIUM, ETextAlignment::CENTER, Colors::WHITE, title));

	int yOffs = 30;

	canvas = std::make_shared<GraphicalPrimitiveCanvas>(Rect(0, yOffs, pos.w - 16, pos.h - yOffs));

	int usedLines = 100;

	slider = std::make_shared<CSlider>(Point(pos.w - 16, yOffs), pos.h - yOffs, [this](int to){ update(); setRedrawParent(true); redraw(); }, LINES, usedLines, 0, Orientation::VERTICAL, CSlider::BLUE);
	slider->setPanningStep(canvas->pos.h / LINES);
	slider->setScrollBounds(Rect(-pos.w + slider->pos.w, 0, pos.w, canvas->pos.h));

	Point fieldSize(canvas->pos.w / (graphics->playerColors.size() + 2), canvas->pos.h / LINES);
	for(int x = 0; x < graphics->playerColors.size() + 1; x++)
		for(int y = 0; y < LINES; y++)
		{
			int xStart = (x + (x == 0 ? 0 : 1)) * fieldSize.x;
			int yStart = y * fieldSize.y;
			if(x == 0 || y == 0)
				canvas->addBox(Point(xStart, yStart), Point(x == 0 ? 2 * fieldSize.x : fieldSize.x, fieldSize.y), ColorRGBA(0, 0, 0, 100));
			canvas->addRectangle(Point(xStart, yStart), Point(x == 0 ? 2 * fieldSize.x : fieldSize.x, fieldSize.y), ColorRGBA(127, 127, 127, 255));
		}

	update();
}

void OverviewPanel::update()
{
	OBJECT_CONSTRUCTION;

	content.clear();
	content.push_back(std::make_shared<CLabel>(pos.w / 2, 100, FONT_MEDIUM, ETextAlignment::CENTER, Colors::WHITE, "blah" + vstd::getDateTimeISO8601Basic(std::time(0))));
}

LineChart::LineChart(Rect position, std::string title, std::map<ColorRGBA, std::vector<float>> data)
	: CIntObject(), maxVal(0), maxDay(0)
{
	OBJECT_CONSTRUCTION;

	addUsedEvents(LCLICK | MOVE);

	pos = position + pos.topLeft();

	layout.push_back(std::make_shared<CLabel>(pos.w / 2, 20, FONT_MEDIUM, ETextAlignment::CENTER, Colors::WHITE, title));

	chartArea = pos.resize(-50);
	chartArea.moveTo(Point(50, 50));

	canvas = std::make_shared<GraphicalPrimitiveCanvas>(Rect(0, 0, pos.w, pos.h));

	statusBar = CGStatusBar::create(0, 0, ImagePath::builtin("radialMenu/statusBar"));
	((std::shared_ptr<CIntObject>)statusBar)->setEnabled(false);

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
