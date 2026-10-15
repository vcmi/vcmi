/*
 * CNumberInput.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../gui/CIntObject.h"

class CTextField;
class CTextButton;

/// Integer input with decrease and increase buttons on the sides
class CNumberInput : public CIntObject
{
	std::shared_ptr<CTextField> field;
	std::shared_ptr<CTextButton> decrease;
	std::shared_ptr<CTextButton> increase;
	std::function<void(int)> callback;

	int value;
	int minValue;
	int maxValue;
	int step;

	void applyValue(int newValue);

public:
	CNumberInput(const Rect & area, int value, int minValue, int maxValue, const std::function<void(int)> & callback = {}, int step = 1);

	int getValue() const;
	/// Changes the value without calling the callback
	void setValue(int newValue);
	void setCallback(const std::function<void(int)> & newCallback);
	void setRange(int newMin, int newMax);
	/// Blocked input can not be changed by the user
	void block(bool on);
};
