/*
 * CListEditor.h, part of VCMI engine
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
class CTextButton;

/// List of entries with buttons to add, edit and remove them. The owner of the data reacts to the callbacks
class CListEditor : public CIntObject
{
	static constexpr int BUTTON_HEIGHT = 24;

	std::shared_ptr<CSelectList> list;
	std::shared_ptr<CTextButton> addButton;
	std::shared_ptr<CTextButton> editButton;
	std::shared_ptr<CTextButton> removeButton;

public:
	/// Called by the buttons, with the index of the selected entry for edit and remove
	std::function<void()> onAdd;
	std::function<void(int)> onEdit;
	std::function<void(int)> onRemove;
	std::function<void(int)> onSelect;

	/// editable adds the edit button
	CListEditor(const Rect & area, bool editable);

	void setEntries(const std::vector<std::string> & entries, int selected = -1);
	int getSelected() const;
	size_t size() const;
};
