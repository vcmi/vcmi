/*
 * CMenuBar.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "CMenuBar.h"

#include "UiTheme.h"

#include "../GameEngine.h"
#include "media/ISoundPlayer.h"
#include "render/Canvas.h"
#include "render/IFont.h"
#include "render/IRenderHandler.h"

namespace
{
constexpr int TITLE_PADDING = 10;
}

CMenuBar::CMenuBar(const Rect & area, std::vector<Menu> menus)
	: CIntObject(LCLICK | MOVE | HOVER)
	, menus(std::move(menus))
{
	setRedrawParent(true);
	pos += area.topLeft();
	pos.w = area.w;
	pos.h = area.h;

	layoutTitles();
}

void CMenuBar::layoutTitles()
{
	const auto font = ENGINE->renderHandler().loadFont(UiTheme::FONT);

	titleAreas.clear();
	int x = pos.x;
	for(const auto & menu : menus)
	{
		const int width = static_cast<int>(font->getStringWidth(menu.title)) + 2 * TITLE_PADDING;
		titleAreas.emplace_back(x, pos.y, width, pos.h);
		x += width;
	}
}

int CMenuBar::getTitlesWidth() const
{
	return titleAreas.empty() ? 0 : titleAreas.back().right() - pos.x;
}

int CMenuBar::titleAt(const Point & position) const
{
	for(size_t i = 0; i < titleAreas.size(); ++i)
		if(titleAreas[i].isInside(position))
			return static_cast<int>(i);

	return -1;
}

void CMenuBar::clickPressed(const Point & cursorPosition)
{
	const int index = titleAt(cursorPosition);
	if(index < 0)
		return;

	ENGINE->sound().playSound(soundBase::button);

	std::vector<CPopupMenu::Entry> entries;
	if(menus[index].entries)
		entries = menus[index].entries();

	if(!entries.empty())
		CPopupMenu::showBelow(titleAreas[index], std::move(entries));
}

void CMenuBar::mouseMoved(const Point & cursorPosition, const Point & lastUpdateDistance)
{
	const int index = titleAt(cursorPosition);
	if(index == hovered)
		return;

	hovered = index;
	redraw();
}

void CMenuBar::hover(bool on)
{
	if(!on && hovered != -1)
	{
		hovered = -1;
		redraw();
	}
}

void CMenuBar::showAll(Canvas & to)
{
	to.drawColorBlended(pos, UiTheme::FIELD_FILL);
	to.drawColor(Rect(pos.x, pos.bottom() - 1, pos.w, 1), UiTheme::BORDER);

	for(size_t i = 0; i < menus.size(); ++i)
	{
		if(static_cast<int>(i) == hovered)
			to.drawColorBlended(titleAreas[i], UiTheme::ROW_SELECTED);

		to.drawText(titleAreas[i].center(), UiTheme::FONT, static_cast<int>(i) == hovered ? UiTheme::TEXT_HOVER : UiTheme::TEXT, ETextAlignment::CENTER, menus[i].title);
	}

	CIntObject::showAll(to);
}
