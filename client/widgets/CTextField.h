/*
 * CTextField.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../gui/CIntObject.h"

class CTextInput;

/// Single line text input with a frame around it
class CTextField : public CIntObject
{
	std::shared_ptr<CTextInput> input;
	std::function<void(const std::string &)> callback;
	bool readOnly = false;

public:
	CTextField(const Rect & area, const std::string & text, const std::function<void(const std::string &)> & callback = {});

	const std::string & getText() const;
	/// Changes the text without calling the callback
	void setText(const std::string & text);
	void setCallback(const std::function<void(const std::string &)> & newCallback);

	void setFilterFilename();
	void setFilterNumber(int minValue, int maxValue);
	void setReadOnly(bool on);
	void giveFocus();

	void showAll(Canvas & to) override;
};
