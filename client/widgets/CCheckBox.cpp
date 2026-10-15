/*
 * CCheckBox.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "CCheckBox.h"

#include "UiText.h"
#include "UiTheme.h"

#include "../GameEngine.h"
#include "media/ISoundPlayer.h"
#include "render/Canvas.h"

CCheckBox::CCheckBox(const Rect & area, const std::string & text, bool checked, const std::function<void(bool)> & callback)
	: text(text)
	, callback(callback)
	, checked(checked)
{
	addUsedEvents(LCLICK);
	setRedrawParent(true);
	pos += area.topLeft();
	pos.w = area.w;
	pos.h = area.h;
}

void CCheckBox::setChecked(bool on)
{
	checked = on;
	redraw();
}

bool CCheckBox::isChecked() const
{
	return checked;
}

void CCheckBox::setText(const std::string & newText)
{
	text = newText;
	redraw();
}

void CCheckBox::block(bool on)
{
	blocked = on;
	redraw();
}

void CCheckBox::clickReleased(const Point & cursorPosition)
{
	if(blocked || !pos.isInside(cursorPosition))
		return;

	ENGINE->sound().playSound(soundBase::button);
	checked = !checked;
	redraw();

	if(callback)
		callback(checked);
}

void CCheckBox::showAll(Canvas & to)
{
	const Rect box(pos.x, pos.y + (pos.h - UiTheme::CHECK_SIZE) / 2, UiTheme::CHECK_SIZE, UiTheme::CHECK_SIZE);
	const ColorRGBA textColor = blocked ? UiTheme::TEXT_DISABLED : UiTheme::TEXT;

	to.drawColorBlended(box, UiTheme::FIELD_FILL);
	to.drawBorder(box, blocked ? UiTheme::TEXT_DISABLED : UiTheme::BORDER_HIGHLIGHT);

	if(checked)
	{
		for(int i = 0; i < 2; ++i)
		{
			to.drawLine(Point(box.x + 3, box.y + 7 + i), Point(box.x + 6, box.y + 10 + i), textColor, textColor);
			to.drawLine(Point(box.x + 6, box.y + 10 + i), Point(box.x + 11, box.y + 3 + i), textColor, textColor);
		}
	}

	if(!text.empty())
	{
		CanvasClipRectGuard guard(to, pos);
		UiText::draw(to, Point(box.right() + UiTheme::SPACING, pos.y + pos.h / 2), UiTheme::FONT, textColor, ETextAlignment::CENTERLEFT, text);
	}

	CIntObject::showAll(to);
}
