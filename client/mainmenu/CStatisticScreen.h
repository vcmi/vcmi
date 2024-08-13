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

class CStatisticScreen : public CWindowObject
{
	std::shared_ptr<FilledTexturePlayerColored> filledBackground;
	std::vector<std::shared_ptr<CIntObject>> layout;
	std::shared_ptr<CToggleButton> buttonCsvSave;
	std::shared_ptr<CToggleButton> buttonSelect;
	StatisticDataSet statistic;
	std::shared_ptr<CIntObject> mainContent;

	std::map<ColorRGBA, std::vector<float>> extractData(StatisticDataSet stat, std::function<float(StatisticDataSetEntry val)> selector);
public:
	CStatisticScreen(StatisticDataSet stat);
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
	StatisticSelector(std::vector<std::string> texts, std::function<void(int selectedIndex)> cb);
};

class OverviewPanel : public CIntObject
{
	std::shared_ptr<GraphicalPrimitiveCanvas> canvas;
	std::vector<std::shared_ptr<CIntObject>> layout;
	std::shared_ptr<CSlider> slider;

	const int LINES = 15;

	void update();
public:
	OverviewPanel(Rect position, StatisticDataSet data);
};

class LineChart : public CIntObject
{
	std::shared_ptr<GraphicalPrimitiveCanvas> canvas;
	std::vector<std::shared_ptr<CIntObject>> layout;
	std::shared_ptr<CGStatusBar> statusBar;

	Rect chartArea;
	float maxVal;
	int maxDay;

	void updateStatusBar(const Point & cursorPosition);
public:
	LineChart(Rect position, std::string title, std::map<ColorRGBA, std::vector<float>> data);

	void mouseMoved(const Point & cursorPosition, const Point & lastUpdateDistance) override;
	void clickPressed(const Point & cursorPosition) override;
};
