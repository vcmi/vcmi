/*
 * CItemValuePicker.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../gui/CIntObject.h"
#include "../windows/CDialogWindow.h"

class CDropDown;
class CListEditor;
class CNumberInput;
class CTextButton;

/// Description of a list of items, every item has a value, e.g. the creatures and their amounts
struct ItemValueConfig
{
	std::string title;
	std::vector<std::string> itemNames;
	int minValue = 1;
	int maxValue = 999999;
	/// When not empty the value is chosen from these names, the first one is the value minValue
	std::vector<std::string> valueNames;
};

using ItemValueEntries = std::vector<std::pair<int, int>>;

/// Window that edits a list of items with values
class CItemValueDialog : public CDialogWindow
{
	ItemValueConfig config;
	ItemValueEntries entries;
	std::shared_ptr<CListEditor> list;
	std::shared_ptr<CDropDown> itemChoice;
	std::shared_ptr<CNumberInput> valueInput;
	std::shared_ptr<CDropDown> valueChoice;

	void refresh(int selected);
	std::string describe(const std::pair<int, int> & entry) const;

public:
	CItemValueDialog(const ItemValueConfig & config, const ItemValueEntries & entries, const std::function<void(const ItemValueEntries &)> & onSaved);

	static std::string describe(const ItemValueConfig & config, const ItemValueEntries & entries);
};

/// Button that shows the list of items with values and opens a window to change it
class CItemValuePicker : public CIntObject
{
	ItemValueConfig config;
	ItemValueEntries entries;
	std::function<void(const ItemValueEntries &)> callback;
	std::shared_ptr<CTextButton> button;

	void updateText();

public:
	CItemValuePicker(const Rect & area, const ItemValueConfig & config, const ItemValueEntries & entries, const std::function<void(const ItemValueEntries &)> & callback = {});

	const ItemValueEntries & getEntries() const;
	void setEntries(const ItemValueEntries & newEntries);
};
