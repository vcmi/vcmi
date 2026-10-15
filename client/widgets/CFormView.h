/*
 * CFormView.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "CScrollPanel.h"

class CLabel;
class CMultiLineLabel;
class CTextField;
class CNumberInput;
class CCheckBox;
class CDropDown;
class CTextButton;

/// Scrollable vertical list of labeled controls
class CFormView : public CScrollPanel
{
	int labelWidth;
	int rowTop = 0;
	std::vector<std::shared_ptr<CIntObject>> rows;
	/// Removed rows stay alive until the next removal, because the callback of a row can remove the row itself
	std::vector<std::shared_ptr<CIntObject>> removedRows;

	Rect controlRect(int height) const;
	void addLabel(const std::string & label, int height);

public:
	CFormView(const Rect & area, int labelWidth);

	/// Removes all the rows
	void clear();
	/// Updates the scrollable area after rows have been added
	void finish();

	std::shared_ptr<CTextField> addText(const std::string & label, const std::string & value, const std::function<void(const std::string &)> & callback);
	std::shared_ptr<CNumberInput> addNumber(const std::string & label, int value, int minValue, int maxValue, const std::function<void(int)> & callback);
	std::shared_ptr<CCheckBox> addCheck(const std::string & label, bool value, const std::function<void(bool)> & callback);
	std::shared_ptr<CDropDown> addChoice(const std::string & label, const std::vector<std::string> & options, int selected, const std::function<void(int)> & callback);
	std::shared_ptr<CTextButton> addButton(const std::string & label, const std::string & buttonText, const std::function<void()> & callback);
	/// Read only value
	std::shared_ptr<CLabel> addInfo(const std::string & label, const std::string & value);
	std::shared_ptr<CLabel> addHeading(const std::string & text);
	/// Text with the full width of the form and as many lines as it needs
	std::shared_ptr<CMultiLineLabel> addParagraph(const std::string & text);
	/// Any other widget placed in the control column, positioned relative to the row
	void addWidget(const std::string & label, const std::shared_ptr<CIntObject> & widget);
	void addSpace(int height);
};
