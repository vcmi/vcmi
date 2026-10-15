/*
 * CTextButton.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../gui/CIntObject.h"
#include "../gui/Shortcut.h"
#include "../../lib/filesystem/ResourcePath.h"

class IImage;

/// Button drawn from primitives, with a text and/or an icon on it
class CTextButton : public CKeyShortcut
{
	std::string text;
	std::string hoverText;
	std::shared_ptr<IImage> icon;
	std::function<void()> callback;

	bool pressed = false;
	bool blocked = false;
	bool highlighted = false;

public:
	CTextButton(const Rect & area, const std::string & text, const std::function<void()> & callback, EShortcut key = EShortcut::NONE);

	/// Button that shows an icon instead of a text
	CTextButton(const Rect & area, const ImagePath & iconPath, const std::string & hoverText, const std::function<void()> & callback, EShortcut key = EShortcut::NONE);

	void setText(const std::string & newText);
	void setHoverText(const std::string & newText);
	void setCallback(const std::function<void()> & newCallback);

	/// Blocked button ignores the input and is drawn dimmed
	void block(bool on);

	/// Highlighted button is drawn as pressed, e.g. the active tool
	void setHighlighted(bool on);
	bool isHighlighted() const;
	bool isBlocked() const;

	void clickPressed(const Point & cursorPosition) override;
	void clickReleased(const Point & cursorPosition) override;
	void clickCancel(const Point & cursorPosition) override;
	void hover(bool on) override;
	void showAll(Canvas & to) override;
};
