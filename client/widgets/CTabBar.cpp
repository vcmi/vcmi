/*
 * CTabBar.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "CTabBar.h"

#include "UiTheme.h"

#include "../GameEngine.h"
#include "media/ISoundPlayer.h"
#include "render/Canvas.h"

CTabBar::CTabBar(const Rect & area, std::vector<std::string> labels, int selected, const std::function<void(int)> & callback)
	: CIntObject(LCLICK)
	, labels(std::move(labels))
	, callback(callback)
	, selected(selected)
{
	setRedrawParent(true);
	pos += area.topLeft();
	pos.w = area.w;
	pos.h = area.h;
}

int CTabBar::tabAt(const Point & position) const
{
	if(labels.empty() || !pos.isInside(position))
		return -1;

	return (position.x - pos.x) * static_cast<int>(labels.size()) / pos.w;
}

int CTabBar::getSelected() const
{
	return selected;
}

void CTabBar::setSelected(int index)
{
	selected = index;
	redraw();
}

void CTabBar::setLabels(std::vector<std::string> newLabels)
{
	labels = std::move(newLabels);
	selected = std::min(selected, static_cast<int>(labels.size()) - 1);
	redraw();
}

void CTabBar::clickPressed(const Point & cursorPosition)
{
	const int index = tabAt(cursorPosition);
	if(index < 0 || index == selected)
		return;

	ENGINE->sound().playSound(soundBase::button);
	selected = index;
	redraw();

	if(callback)
		callback(index);
}

void CTabBar::showAll(Canvas & to)
{
	const int count = static_cast<int>(labels.size());

	for(int i = 0; i < count; ++i)
	{
		const int left = pos.x + pos.w * i / count;
		const int right = pos.x + pos.w * (i + 1) / count;
		const Rect tab(left, pos.y, right - left, pos.h);
		const bool active = i == selected;

		to.drawColorBlended(tab, active ? UiTheme::BUTTON_FILL_PRESSED : UiTheme::BUTTON_FILL_BLOCKED);
		to.drawBorder(tab, active ? UiTheme::BORDER_HIGHLIGHT : UiTheme::BORDER);

		CanvasClipRectGuard guard(to, tab);
		to.drawText(tab.center(), UiTheme::FONT, active ? UiTheme::TEXT_SELECTED : UiTheme::TEXT, ETextAlignment::CENTER, labels[i]);
	}

	CIntObject::showAll(to);
}
