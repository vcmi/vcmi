/*
 * CPopupMenu.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../gui/CIntObject.h"

class CSelectList;

/// Menu that is shown on top of the other windows and closes on the first click outside of it
class CPopupMenu : public WindowBase
{
public:
	struct Entry
	{
		std::string text;
		std::string shortcut; ///< hint aligned to the right
		bool enabled = true;
		bool checked = false;
		bool separator = false;
		std::function<void()> callback;
		std::vector<Entry> children;

		static Entry makeSeparator();
	};

private:
	std::vector<Entry> entries;
	std::shared_ptr<CSelectList> list;

	void onEntrySelected(int index);

public:
	/// anchor is the rectangle the menu is attached to. The menu opens below it, or on its right side for submenus
	CPopupMenu(const Rect & anchor, std::vector<Entry> entries, bool submenu);

	/// Opens the menu below the anchor
	static void showBelow(const Rect & anchor, std::vector<Entry> entries);
	/// Opens the menu with its corner at the position, e.g. the cursor
	static void showAt(const Point & position, std::vector<Entry> entries);
	/// Closes the menu with all its submenus
	static void closeAll();

	void clickPressed(const Point & cursorPosition) override;
	void showPopupWindow(const Point & cursorPosition) override;
	void keyPressed(EShortcut key) override;
	void onScreenResize() override;
	bool receiveEvent(const Point & position, int eventType) const override;
};
