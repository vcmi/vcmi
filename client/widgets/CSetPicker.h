/*
 * CSetPicker.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../gui/CIntObject.h"
#include "../windows/CChoiceDialog.h"

class CTextButton;

/// Button that shows how many items are chosen and opens a dialog to choose them
class CSetPicker : public CIntObject
{
	std::string title;
	std::vector<CChoiceDialog::Item> items;
	std::set<int> selected;
	std::function<void(const std::set<int> &)> callback;
	std::shared_ptr<CTextButton> button;

	void updateText();

public:
	/// The selected items are numbers of the items in the vector
	CSetPicker(const Rect & area, const std::string & title, std::vector<CChoiceDialog::Item> items, const std::set<int> & selected, const std::function<void(const std::set<int> &)> & callback = {});

	const std::set<int> & getSelected() const;
	/// Changes the selection without calling the callback
	void setSelected(const std::set<int> & newSelection);
};
