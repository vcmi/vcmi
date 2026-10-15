/*
 * CMenuBar.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../gui/CIntObject.h"
#include "../windows/CPopupMenu.h"

/// Horizontal bar of menu titles. A click on a title opens the menu below it
class CMenuBar : public CIntObject
{
public:
	struct Menu
	{
		std::string title;
		/// Called every time the menu is opened, so the entries can reflect the current state
		std::function<std::vector<CPopupMenu::Entry>()> entries;
	};

private:
	std::vector<Menu> menus;
	std::vector<Rect> titleAreas;
	int hovered = -1;

	int titleAt(const Point & position) const;
	void layoutTitles();

public:
	CMenuBar(const Rect & area, std::vector<Menu> menus);

	/// Total width taken by the titles
	int getTitlesWidth() const;

	void clickPressed(const Point & cursorPosition) override;
	void mouseMoved(const Point & cursorPosition, const Point & lastUpdateDistance) override;
	void hover(bool on) override;
	void showAll(Canvas & to) override;
};
