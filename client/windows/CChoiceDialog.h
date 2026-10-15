/*
 * CChoiceDialog.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "CDialogWindow.h"

class CSelectList;
class CTextField;
class IImage;

/// Dialog with a filterable list to choose one or many items from
class CChoiceDialog : public CDialogWindow
{
public:
	struct Item
	{
		std::string text;
		std::string detail;
		std::shared_ptr<IImage> icon;
		bool enabled = true;
	};

private:
	std::vector<Item> items;
	std::vector<bool> checked;
	std::vector<int> shown;
	std::string filterText;
	bool multiple;
	int selected = -1;

	std::shared_ptr<CTextField> filterField;
	std::shared_ptr<CSelectList> list;

	void applyFilter(const std::string & text);
	void setAllShownChecked(bool on);

public:
	CChoiceDialog(const std::string & title, std::vector<Item> items, bool multiple);

	/// Index of the chosen item, or -1
	int getSelected() const;
	/// Indexes of all the checked items
	std::set<int> getChecked() const;

	void setSelected(int index);
	void setChecked(const std::set<int> & indexes);

	/// Opens the dialog, onChosen gets the index of the chosen item
	static void chooseOne(const std::string & title, std::vector<Item> items, int selected, const std::function<void(int)> & onChosen);
	/// Opens the dialog, onChosen gets indexes of all the checked items
	static void chooseMany(const std::string & title, std::vector<Item> items, const std::set<int> & checked, const std::function<void(const std::set<int> &)> & onChosen);

	/// Items made of plain texts
	static std::vector<Item> makeItems(const std::vector<std::string> & texts);
};
