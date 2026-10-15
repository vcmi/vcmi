/*
 * CDropDown.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../gui/CIntObject.h"

/// Field that shows the selected option and lets to choose another one from a list
class CDropDown : public CIntObject
{
	/// Lists longer than this open in a dialog with a filter instead of a menu
	static constexpr size_t MENU_LIMIT = 24;

	std::vector<std::string> options;
	std::string title;
	std::function<void(int)> callback;
	int selected;
	bool blocked = false;

	void open();
	void choose(int index);

public:
	CDropDown(const Rect & area, std::vector<std::string> options, int selected, const std::function<void(int)> & callback, const std::string & title = "");

	/// Replaces the options, keeps the callback
	void setOptions(std::vector<std::string> newOptions, int newSelected);
	/// Changes the selected option without calling the callback
	void setSelected(int index);
	int getSelected() const;
	std::string getSelectedText() const;
	void setCallback(const std::function<void(int)> & newCallback);
	void block(bool on);

	void clickReleased(const Point & cursorPosition) override;
	void showAll(Canvas & to) override;
};
