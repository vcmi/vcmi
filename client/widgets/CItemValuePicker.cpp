/*
 * CItemValuePicker.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "CItemValuePicker.h"

#include "CDropDown.h"
#include "CListEditor.h"
#include "CNumberInput.h"
#include "CTextButton.h"
#include "UiTheme.h"

#include "../GameEngine.h"
#include "../gui/WindowHandler.h"

#include "../../lib/GameLibrary.h"
#include "../../lib/texts/CGeneralTextHandler.h"

namespace
{
constexpr int DIALOG_WIDTH = 460;
constexpr int DIALOG_HEIGHT = 420;
constexpr size_t HOVER_LIMIT = 30;
}

std::string CItemValueDialog::describe(const ItemValueConfig & config, const ItemValueEntries & entries)
{
	std::string result;
	size_t shown = 0;

	for(const auto & entry : entries)
	{
		if(shown++ >= HOVER_LIMIT)
		{
			result += ", ...";
			break;
		}

		std::string value = std::to_string(entry.second);
		if(!config.valueNames.empty())
			value = config.valueNames.at(entry.second - config.minValue);

		const std::string name = entry.first >= 0 && entry.first < static_cast<int>(config.itemNames.size()) ? config.itemNames[entry.first] : std::to_string(entry.first);
		result += (result.empty() ? "" : ", ") + name + " (" + value + ")";
	}
	return result;
}

std::string CItemValueDialog::describe(const std::pair<int, int> & entry) const
{
	return describe(config, {entry});
}

CItemValueDialog::CItemValueDialog(const ItemValueConfig & config, const ItemValueEntries & entries, const std::function<void(const ItemValueEntries &)> & onSaved)
	: CDialogWindow(Point(DIALOG_WIDTH, DIALOG_HEIGHT), config.title)
	, config(config)
	, entries(entries)
{
	OBJECT_CONSTRUCTION_TARGETED(content());

	const Point size = contentSize();
	const int controlHeight = UiTheme::CONTROL_HEIGHT;
	const int inputTop = size.y - controlHeight;

	list = std::make_shared<CListEditor>(Rect(0, 0, size.x, inputTop - UiTheme::SPACING), false);
	list->onAdd = [this]()
	{
		const int item = itemChoice->getSelected();
		if(item < 0)
			return;

		const int value = this->config.valueNames.empty() ? valueInput->getValue() : valueChoice->getSelected() + this->config.minValue;

		// one record for an item
		for(auto & entry : this->entries)
		{
			if(entry.first == item)
			{
				entry.second = value;
				refresh(static_cast<int>(&entry - this->entries.data()));
				return;
			}
		}

		this->entries.emplace_back(item, value);
		refresh(static_cast<int>(this->entries.size()) - 1);
	};
	list->onRemove = [this](int index)
	{
		this->entries.erase(this->entries.begin() + index);
		refresh(-1);
	};
	list->onSelect = [this](int index)
	{
		const auto & entry = this->entries.at(index);
		itemChoice->setSelected(entry.first);

		if(this->config.valueNames.empty())
			valueInput->setValue(entry.second);
		else
			valueChoice->setSelected(entry.second - this->config.minValue);
	};

	const int valueWidth = 120;
	itemChoice = std::make_shared<CDropDown>(Rect(0, inputTop, size.x - valueWidth - UiTheme::SPACING, controlHeight), config.itemNames, config.itemNames.empty() ? -1 : 0, nullptr, config.title);

	if(config.valueNames.empty())
		valueInput = std::make_shared<CNumberInput>(Rect(size.x - valueWidth, inputTop, valueWidth, controlHeight), config.minValue, config.minValue, config.maxValue);
	else
		valueChoice = std::make_shared<CDropDown>(Rect(size.x - valueWidth, inputTop, valueWidth, controlHeight), config.valueNames, 0, nullptr, config.title);

	refresh(-1);

	onAccept = [this, onSaved]()
	{
		if(onSaved)
			onSaved(this->entries);
		return true;
	};
}

void CItemValueDialog::refresh(int selected)
{
	std::vector<std::string> names;
	for(const auto & entry : entries)
		names.push_back(describe(entry));

	list->setEntries(names, selected);
}

CItemValuePicker::CItemValuePicker(const Rect & area, const ItemValueConfig & config, const ItemValueEntries & entries, const std::function<void(const ItemValueEntries &)> & callback)
	: config(config)
	, entries(entries)
	, callback(callback)
{
	pos += area.topLeft();
	pos.w = area.w;
	pos.h = area.h;

	OBJECT_CONSTRUCTION;
	button = std::make_shared<CTextButton>(Rect(0, 0, area.w, area.h), "", [this]()
	{
		ENGINE->windows().createAndPushWindow<CItemValueDialog>(this->config, this->entries, [this](const ItemValueEntries & edited)
		{
			this->entries = edited;
			updateText();

			if(this->callback)
				this->callback(edited);
		});
	});

	updateText();
}

void CItemValuePicker::updateText()
{
	std::string text = LIBRARY->generaltexth->translate("vcmi.ui.selectedCount");
	const size_t placeholder = text.find("%s");
	if(placeholder != std::string::npos)
		text.replace(placeholder, 2, std::to_string(entries.size()));

	button->setText(text);
	button->setHoverText(CItemValueDialog::describe(config, entries));
}

const ItemValueEntries & CItemValuePicker::getEntries() const
{
	return entries;
}

void CItemValuePicker::setEntries(const ItemValueEntries & newEntries)
{
	entries = newEntries;
	updateText();
}
