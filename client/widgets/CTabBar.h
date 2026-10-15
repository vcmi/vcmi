/*
 * CTabBar.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../gui/CIntObject.h"

/// Row of tab headers, exactly one of them is selected
class CTabBar : public CIntObject
{
	std::vector<std::string> labels;
	std::function<void(int)> callback;
	int selected;

	int tabAt(const Point & position) const;

public:
	CTabBar(const Rect & area, std::vector<std::string> labels, int selected, const std::function<void(int)> & callback);

	int getSelected() const;
	/// Changes the selected tab without calling the callback
	void setSelected(int index);
	void setLabels(std::vector<std::string> newLabels);

	void clickPressed(const Point & cursorPosition) override;
	void showAll(Canvas & to) override;
};
