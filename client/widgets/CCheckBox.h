/*
 * CCheckBox.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../gui/CIntObject.h"

/// Check mark with a text on its right
class CCheckBox : public CIntObject
{
	std::string text;
	std::function<void(bool)> callback;

	bool checked;
	bool blocked = false;

public:
	CCheckBox(const Rect & area, const std::string & text, bool checked, const std::function<void(bool)> & callback);

	/// Changes the state without calling the callback
	void setChecked(bool on);
	bool isChecked() const;

	void setText(const std::string & newText);
	void block(bool on);

	void clickReleased(const Point & cursorPosition) override;
	void showAll(Canvas & to) override;
};
