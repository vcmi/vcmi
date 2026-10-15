/*
 * CDropDown.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "CDropDown.h"

#include "UiTheme.h"

#include "../GameEngine.h"
#include "../windows/CChoiceDialog.h"
#include "../windows/CPopupMenu.h"
#include "media/ISoundPlayer.h"
#include "render/Canvas.h"

CDropDown::CDropDown(const Rect & area, std::vector<std::string> options, int selected, const std::function<void(int)> & callback, const std::string & title)
	: options(std::move(options))
	, title(title)
	, callback(callback)
	, selected(selected)
{
	addUsedEvents(LCLICK);
	setRedrawParent(true);
	pos += area.topLeft();
	pos.w = area.w;
	pos.h = area.h;
}

void CDropDown::setOptions(std::vector<std::string> newOptions, int newSelected)
{
	options = std::move(newOptions);
	selected = newSelected;
	redraw();
}

void CDropDown::setSelected(int index)
{
	selected = index;
	redraw();
}

int CDropDown::getSelected() const
{
	return selected;
}

std::string CDropDown::getSelectedText() const
{
	if(selected < 0 || selected >= static_cast<int>(options.size()))
		return "";

	return options[selected];
}

void CDropDown::setCallback(const std::function<void(int)> & newCallback)
{
	callback = newCallback;
}

void CDropDown::block(bool on)
{
	blocked = on;
	redraw();
}

void CDropDown::choose(int index)
{
	selected = index;
	redraw();

	if(callback)
		callback(index);
}

void CDropDown::open()
{
	if(options.size() > MENU_LIMIT)
	{
		CChoiceDialog::chooseOne(title, CChoiceDialog::makeItems(options), selected, [this](int index) { choose(index); });
		return;
	}

	std::vector<CPopupMenu::Entry> entries;
	for(int i = 0; i < static_cast<int>(options.size()); ++i)
	{
		CPopupMenu::Entry entry;
		entry.text = options[i];
		entry.checked = i == selected;
		entry.callback = [this, i]() { choose(i); };
		entries.push_back(std::move(entry));
	}
	CPopupMenu::showBelow(pos, std::move(entries));
}

void CDropDown::clickReleased(const Point & cursorPosition)
{
	if(blocked || !pos.isInside(cursorPosition) || options.empty())
		return;

	ENGINE->sound().playSound(soundBase::button);
	open();
}

void CDropDown::showAll(Canvas & to)
{
	to.drawColorBlended(pos, UiTheme::FIELD_FILL);
	to.drawBorder(pos, UiTheme::BORDER);

	const ColorRGBA textColor = blocked ? UiTheme::TEXT_DISABLED : UiTheme::TEXT;

	{
		CanvasClipRectGuard guard(to, Rect(pos.x + 1, pos.y, pos.w - pos.h, pos.h));
		to.drawText(Point(pos.x + 6, pos.y + pos.h / 2), UiTheme::FONT, textColor, ETextAlignment::CENTERLEFT, getSelectedText());
	}

	const Point arrow(pos.right() - pos.h / 2, pos.y + pos.h / 2);
	for(int i = 0; i < 4; ++i)
		to.drawLine(Point(arrow.x - 4 + i, arrow.y - 2 + i), Point(arrow.x + 4 - i, arrow.y - 2 + i), textColor, textColor);

	CIntObject::showAll(to);
}
