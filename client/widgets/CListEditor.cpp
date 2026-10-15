/*
 * CListEditor.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "CListEditor.h"

#include "CSelectList.h"
#include "CTextButton.h"
#include "UiTheme.h"

#include "../../lib/GameLibrary.h"
#include "../../lib/texts/CGeneralTextHandler.h"

CListEditor::CListEditor(const Rect & area, bool editable)
{
	pos += area.topLeft();
	pos.w = area.w;
	pos.h = area.h;

	OBJECT_CONSTRUCTION;

	list = std::make_shared<CSelectList>(Rect(0, 0, area.w, area.h - BUTTON_HEIGHT - UiTheme::SPACING), CSelectList::Mode::SINGLE, UiTheme::ROW_HEIGHT);
	list->onSelect = [this](int index)
	{
		removeButton->block(false);
		if(editButton)
			editButton->block(false);

		if(onSelect)
			onSelect(index);
	};
	list->onActivate = [this](int index)
	{
		if(onEdit)
			onEdit(index);
	};

	const int buttons = editable ? 3 : 2;
	const int buttonWidth = (area.w - (buttons - 1) * UiTheme::SPACING) / buttons;
	const int top = area.h - BUTTON_HEIGHT;
	int x = 0;

	addButton = std::make_shared<CTextButton>(Rect(x, top, buttonWidth, BUTTON_HEIGHT), LIBRARY->generaltexth->translate("vcmi.ui.add"), [this]()
	{
		if(onAdd)
			onAdd();
	});
	x += buttonWidth + UiTheme::SPACING;

	if(editable)
	{
		editButton = std::make_shared<CTextButton>(Rect(x, top, buttonWidth, BUTTON_HEIGHT), LIBRARY->generaltexth->translate("vcmi.ui.edit"), [this]()
		{
			if(onEdit && list->getSelected() >= 0)
				onEdit(list->getSelected());
		});
		editButton->block(true);
		x += buttonWidth + UiTheme::SPACING;
	}

	removeButton = std::make_shared<CTextButton>(Rect(x, top, buttonWidth, BUTTON_HEIGHT), LIBRARY->generaltexth->translate("vcmi.ui.remove"), [this]()
	{
		if(onRemove && list->getSelected() >= 0)
			onRemove(list->getSelected());
	});
	removeButton->block(true);
}

void CListEditor::setEntries(const std::vector<std::string> & entries, int selected)
{
	std::vector<CSelectList::Row> rows;
	for(const auto & entry : entries)
	{
		CSelectList::Row row;
		row.text = entry;
		rows.push_back(std::move(row));
	}

	list->setRows(std::move(rows), true);
	list->setSelected(selected);

	const bool hasSelection = selected >= 0 && selected < static_cast<int>(entries.size());
	removeButton->block(!hasSelection);
	if(editButton)
		editButton->block(!hasSelection);
}

int CListEditor::getSelected() const
{
	return list->getSelected();
}

size_t CListEditor::size() const
{
	return list->size();
}
