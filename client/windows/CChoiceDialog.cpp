/*
 * CChoiceDialog.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "CChoiceDialog.h"

#include "../GameEngine.h"
#include "../gui/WindowHandler.h"
#include "../widgets/CSelectList.h"
#include "../widgets/CTextField.h"
#include "../widgets/UiTheme.h"

#include "../../lib/GameLibrary.h"
#include "../../lib/texts/CGeneralTextHandler.h"

#include <boost/algorithm/string/predicate.hpp>

namespace
{
constexpr int DIALOG_WIDTH = 440;
constexpr int DIALOG_HEIGHT = 540;
}

CChoiceDialog::CChoiceDialog(const std::string & title, std::vector<Item> items, bool multiple)
	: CDialogWindow(Point(DIALOG_WIDTH, DIALOG_HEIGHT), title)
	, items(std::move(items))
	, checked(this->items.size(), false)
	, multiple(multiple)
{
	OBJECT_CONSTRUCTION_TARGETED(content());

	const Point size = contentSize();
	const int filterHeight = UiTheme::CONTROL_HEIGHT;

	filterField = std::make_shared<CTextField>(Rect(0, 0, size.x, filterHeight), "", [this](const std::string & text) { applyFilter(text); });

	list = std::make_shared<CSelectList>(Rect(0, filterHeight + UiTheme::SPACING, size.x, size.y - filterHeight - UiTheme::SPACING),
		multiple ? CSelectList::Mode::CHECK : CSelectList::Mode::SINGLE, UiTheme::ROW_HEIGHT);

	list->onSelect = [this](int row)
	{
		selected = shown.at(row);
	};
	list->onCheck = [this](int row, bool on)
	{
		checked.at(shown.at(row)) = on;
	};
	list->onActivate = [this](int row)
	{
		selected = shown.at(row);
		if(!this->multiple)
			accept();
	};

	if(multiple)
	{
		addButton(LIBRARY->generaltexth->translate("vcmi.ui.selectAll"), [this]() { setAllShownChecked(true); });
		addButton(LIBRARY->generaltexth->translate("vcmi.ui.selectNone"), [this]() { setAllShownChecked(false); });
	}

	applyFilter("");

#if !defined(VCMI_MOBILE)
	filterField->giveFocus();
#endif
}

void CChoiceDialog::applyFilter(const std::string & text)
{
	filterText = text;
	shown.clear();

	std::vector<CSelectList::Row> rows;
	int selectedRow = -1;

	for(int i = 0; i < static_cast<int>(items.size()); ++i)
	{
		if(!text.empty() && !boost::algorithm::icontains(items[i].text, text))
			continue;

		CSelectList::Row row;
		row.text = items[i].text;
		row.detail = items[i].detail;
		row.icon = items[i].icon;
		row.enabled = items[i].enabled;
		row.checked = checked[i];
		if(i == selected)
			selectedRow = static_cast<int>(rows.size());

		shown.push_back(i);
		rows.push_back(std::move(row));
	}

	list->setRows(std::move(rows));
	if(selectedRow >= 0)
		list->setSelected(selectedRow);
}

void CChoiceDialog::setAllShownChecked(bool on)
{
	for(int index : shown)
		if(items[index].enabled)
			checked[index] = on;

	applyFilter(filterText);
}

int CChoiceDialog::getSelected() const
{
	return selected;
}

std::set<int> CChoiceDialog::getChecked() const
{
	std::set<int> result;
	for(int i = 0; i < static_cast<int>(checked.size()); ++i)
		if(checked[i])
			result.insert(i);
	return result;
}

void CChoiceDialog::setSelected(int index)
{
	selected = index;
	applyFilter(filterText);
}

void CChoiceDialog::setChecked(const std::set<int> & indexes)
{
	for(int i = 0; i < static_cast<int>(checked.size()); ++i)
		checked[i] = indexes.count(i) != 0;

	applyFilter(filterText);
}

void CChoiceDialog::chooseOne(const std::string & title, std::vector<Item> items, int selected, const std::function<void(int)> & onChosen)
{
	auto dialog = std::make_shared<CChoiceDialog>(title, std::move(items), false);
	dialog->setSelected(selected);

	auto * raw = dialog.get();
	dialog->onAccept = [raw, onChosen]()
	{
		if(raw->getSelected() < 0)
			return false;

		if(onChosen)
			onChosen(raw->getSelected());
		return true;
	};
	ENGINE->windows().pushWindow(dialog);
}

void CChoiceDialog::chooseMany(const std::string & title, std::vector<Item> items, const std::set<int> & checked, const std::function<void(const std::set<int> &)> & onChosen)
{
	auto dialog = std::make_shared<CChoiceDialog>(title, std::move(items), true);
	dialog->setChecked(checked);

	auto * raw = dialog.get();
	dialog->onAccept = [raw, onChosen]()
	{
		if(onChosen)
			onChosen(raw->getChecked());
		return true;
	};
	ENGINE->windows().pushWindow(dialog);
}

std::vector<CChoiceDialog::Item> CChoiceDialog::makeItems(const std::vector<std::string> & texts)
{
	std::vector<Item> result;
	result.reserve(texts.size());
	for(const auto & text : texts)
	{
		Item item;
		item.text = text;
		result.push_back(std::move(item));
	}
	return result;
}
