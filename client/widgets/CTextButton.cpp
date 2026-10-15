/*
 * CTextButton.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "CTextButton.h"

#include "UiTheme.h"

#include "../GameEngine.h"
#include "../gui/Shortcut.h"
#include "media/ISoundPlayer.h"
#include "render/Canvas.h"
#include "render/IImage.h"
#include "render/IRenderHandler.h"

CTextButton::CTextButton(const Rect & area, const std::string & text, const std::function<void()> & callback, EShortcut key)
	: CKeyShortcut(key)
	, text(text)
	, callback(callback)
{
	addUsedEvents(LCLICK | HOVER | KEYBOARD);
	setRedrawParent(true);
	pos += area.topLeft();
	pos.w = area.w;
	pos.h = area.h;
}

CTextButton::CTextButton(const Rect & area, const ImagePath & iconPath, const std::string & hoverText, const std::function<void()> & callback, EShortcut key)
	: CTextButton(area, "", callback, key)
{
	this->hoverText = hoverText;
	icon = ENGINE->renderHandler().loadImage(iconPath, EImageBlitMode::SIMPLE);
}

void CTextButton::setText(const std::string & newText)
{
	text = newText;
	redraw();
}

void CTextButton::setHoverText(const std::string & newText)
{
	hoverText = newText;
}

void CTextButton::setCallback(const std::function<void()> & newCallback)
{
	callback = newCallback;
}

void CTextButton::block(bool on)
{
	blocked = on;
	pressed = false;
	redraw();
}

void CTextButton::setHighlighted(bool on)
{
	if(highlighted == on)
		return;

	highlighted = on;
	redraw();
}

bool CTextButton::isHighlighted() const
{
	return highlighted;
}

bool CTextButton::isBlocked() const
{
	return blocked;
}

void CTextButton::clickPressed(const Point & cursorPosition)
{
	if(blocked)
		return;

	pressed = true;
	ENGINE->sound().playSound(soundBase::button);
	redraw();
}

void CTextButton::clickReleased(const Point & cursorPosition)
{
	if(!pressed)
		return;

	pressed = false;
	redraw();

	if(callback)
		callback();
}

void CTextButton::clickCancel(const Point & cursorPosition)
{
	if(!pressed)
		return;

	pressed = false;
	redraw();
}

void CTextButton::hover(bool on)
{
	if(hoverText.empty())
		return;

	if(on)
		ENGINE->statusbar()->write(hoverText);
	else
		ENGINE->statusbar()->clearIfMatching(hoverText);
}

void CTextButton::showAll(Canvas & to)
{
	const bool down = pressed || highlighted;

	ColorRGBA fill = UiTheme::BUTTON_FILL;
	if(blocked)
		fill = UiTheme::BUTTON_FILL_BLOCKED;
	else if(down)
		fill = UiTheme::BUTTON_FILL_PRESSED;

	to.drawColorBlended(pos, fill);
	to.drawBorder(pos, down && !blocked ? UiTheme::BORDER_HIGHLIGHT : UiTheme::BORDER);

	const Point offset = pressed ? Point(1, 1) : Point(0, 0);

	if(icon)
		to.draw(icon, pos.center() - icon->dimensions() / 2 + offset);

	if(!text.empty())
	{
		CanvasClipRectGuard guard(to, pos);
		to.drawText(pos.center() + offset, UiTheme::FONT, blocked ? UiTheme::TEXT_DISABLED : UiTheme::TEXT, ETextAlignment::CENTER, text);
	}

	CIntObject::showAll(to);
}
