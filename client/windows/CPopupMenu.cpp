/*
 * CPopupMenu.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "CPopupMenu.h"

#include "../GameEngine.h"
#include "../gui/Shortcut.h"
#include "../gui/WindowHandler.h"
#include "../widgets/CSelectList.h"
#include "../widgets/UiTheme.h"
#include "render/IFont.h"
#include "render/IRenderHandler.h"

namespace
{
constexpr int MENU_MARGIN = 24;
constexpr int SCREEN_MARGIN = 4;
}

CPopupMenu::Entry CPopupMenu::Entry::makeSeparator()
{
	Entry result;
	result.separator = true;
	return result;
}

CPopupMenu::CPopupMenu(const Rect & anchor, std::vector<Entry> entries, bool submenu)
	: WindowBase(LCLICK | SHOW_POPUP | KEYBOARD)
	, entries(std::move(entries))
{
	OBJECT_CONSTRUCTION;

	const Point screen = ENGINE->screenDimensions();
	pos = Rect(0, 0, screen.x, screen.y);

	const auto font = ENGINE->renderHandler().loadFont(UiTheme::FONT);

	int textWidth = 0;
	int shortcutWidth = 0;
	for(const auto & entry : this->entries)
	{
		textWidth = std::max(textWidth, static_cast<int>(font->getStringWidth(entry.text)));
		shortcutWidth = std::max(shortcutWidth, static_cast<int>(font->getStringWidth(entry.shortcut)));
	}

	const int width = std::min(screen.x - 2 * SCREEN_MARGIN, textWidth + shortcutWidth + 2 * MENU_MARGIN + (shortcutWidth > 0 ? MENU_MARGIN : 0) + UiTheme::SLIDER_WIDTH);
	const int fullHeight = static_cast<int>(this->entries.size()) * UiTheme::ROW_HEIGHT + 2;

	Point corner = submenu ? Point(anchor.right(), anchor.y) : Point(anchor.x, anchor.bottom());
	int height = std::min(fullHeight, screen.y - 2 * SCREEN_MARGIN);

	if(corner.x + width > screen.x - SCREEN_MARGIN)
		corner.x = submenu ? anchor.x - width : screen.x - SCREEN_MARGIN - width;
	corner.x = std::max(corner.x, SCREEN_MARGIN);

	if(corner.y + height > screen.y - SCREEN_MARGIN)
	{
		// above the anchor if there is more room there
		if(!submenu && anchor.y > screen.y - anchor.bottom())
		{
			height = std::min(height, anchor.y - SCREEN_MARGIN);
			corner.y = anchor.y - height;
		}
		else
		{
			corner.y = std::max(SCREEN_MARGIN, screen.y - SCREEN_MARGIN - height);
			height = std::min(height, screen.y - SCREEN_MARGIN - corner.y);
		}
	}

	list = std::make_shared<CSelectList>(Rect(corner, Point(width, height)), CSelectList::Mode::SINGLE, UiTheme::ROW_HEIGHT);

	std::vector<CSelectList::Row> rows;
	for(const auto & entry : this->entries)
	{
		CSelectList::Row row;
		row.text = (entry.checked ? "* " : "") + entry.text;
		row.detail = entry.shortcut;
		row.enabled = entry.enabled;
		row.separator = entry.separator;
		rows.push_back(std::move(row));
	}
	list->setRows(std::move(rows));
	list->onSelect = [this](int index) { onEntrySelected(index); };
}

void CPopupMenu::showBelow(const Rect & anchor, std::vector<Entry> entries)
{
	ENGINE->windows().createAndPushWindow<CPopupMenu>(anchor, std::move(entries), false);
}

void CPopupMenu::showAt(const Point & position, std::vector<Entry> entries)
{
	ENGINE->windows().createAndPushWindow<CPopupMenu>(Rect(position, Point(0, 0)), std::move(entries), false);
}

void CPopupMenu::closeAll()
{
	while(ENGINE->windows().topWindow<CPopupMenu>())
		ENGINE->windows().popWindows(1);
}

void CPopupMenu::onEntrySelected(int index)
{
	const Entry entry = entries.at(index);

	if(!entry.children.empty())
	{
		const Rect rowRect(list->pos.x, list->pos.y + index * UiTheme::ROW_HEIGHT, list->pos.w, UiTheme::ROW_HEIGHT);
		ENGINE->windows().createAndPushWindow<CPopupMenu>(rowRect, entry.children, true);
		return;
	}

	closeAll();

	if(entry.callback)
		entry.callback();
}

void CPopupMenu::clickPressed(const Point & cursorPosition)
{
	if(!list->pos.isInside(cursorPosition))
		closeAll();
}

void CPopupMenu::showPopupWindow(const Point & cursorPosition)
{
	if(!list->pos.isInside(cursorPosition))
		closeAll();
}

void CPopupMenu::keyPressed(EShortcut key)
{
	if(key == EShortcut::GLOBAL_CANCEL)
		closeAll();
}

void CPopupMenu::onScreenResize()
{
	const Point screen = ENGINE->screenDimensions();
	pos = Rect(0, 0, screen.x, screen.y);
}

bool CPopupMenu::receiveEvent(const Point & position, int eventType) const
{
	return true;
}
