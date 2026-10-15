/*
 * EditorProgressWindow.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "EditorProgressWindow.h"

#include "../GameEngine.h"
#include "../gui/WindowHandler.h"
#include "../widgets/CPanel.h"
#include "../widgets/TextControls.h"
#include "../widgets/UiTheme.h"
#include "render/Canvas.h"

#include "../../lib/LoadProgress.h"

EditorProgressWindow::EditorProgressWindow(const std::string & title, Load::Progress & source, const std::function<void()> & onFinished)
	: WindowBase(TIME)
	, source(source)
	, onFinished(onFinished)
{
	OBJECT_CONSTRUCTION;

	pos.w = WIDTH;
	pos.h = HEIGHT;

	panel = std::make_shared<CPanel>(Rect(0, 0, WIDTH, HEIGHT));
	titleLabel = std::make_shared<CLabel>(WIDTH / 2, 20, UiTheme::FONT_TITLE, ETextAlignment::CENTER, UiTheme::TEXT_TITLE, title);
	percentLabel = std::make_shared<CLabel>(WIDTH / 2, HEIGHT - 20, UiTheme::FONT, ETextAlignment::CENTER, UiTheme::TEXT, "0%");

	center();
}

void EditorProgressWindow::show(const std::string & title, Load::Progress & source, const std::function<void()> & onFinished)
{
	ENGINE->windows().createAndPushWindow<EditorProgressWindow>(title, source, onFinished);
}

void EditorProgressWindow::tick(uint32_t msPassed)
{
	if(source.finished())
	{
		auto callback = onFinished;
		ENGINE->windows().popWindows(1);

		if(callback)
			callback();
		return;
	}

	const int percent = static_cast<int>(100.0 * source.get() / std::numeric_limits<Load::Type>::max());
	if(percent != shownPercent)
	{
		shownPercent = percent;
		percentLabel->setText(std::to_string(percent) + "%");
		redraw();
	}
}

void EditorProgressWindow::showAll(Canvas & to)
{
	CIntObject::showAll(to);

	const Rect bar(pos.x + 20, pos.y + HEIGHT / 2 - BAR_HEIGHT / 2, WIDTH - 40, BAR_HEIGHT);
	to.drawColorBlended(bar, UiTheme::FIELD_FILL);
	to.drawBorder(bar, UiTheme::BORDER);

	const int filled = std::clamp(shownPercent, 0, 100) * (bar.w - 2) / 100;
	if(filled > 0)
		to.drawColorBlended(Rect(bar.x + 1, bar.y + 1, filled, bar.h - 2), UiTheme::BUTTON_FILL_PRESSED);
}
