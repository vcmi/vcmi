/*
 * CSetPicker.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "CSetPicker.h"

#include "CTextButton.h"

#include "../../lib/GameLibrary.h"
#include "../../lib/texts/CGeneralTextHandler.h"

namespace
{
constexpr size_t HOVER_LIMIT = 30;
}

CSetPicker::CSetPicker(const Rect & area, const std::string & title, std::vector<CChoiceDialog::Item> items, const std::set<int> & selected, const std::function<void(const std::set<int> &)> & callback)
	: title(title)
	, items(std::move(items))
	, selected(selected)
	, callback(callback)
{
	pos += area.topLeft();
	pos.w = area.w;
	pos.h = area.h;

	OBJECT_CONSTRUCTION;
	button = std::make_shared<CTextButton>(Rect(0, 0, area.w, area.h), "", [this]()
	{
		CChoiceDialog::chooseMany(this->title, this->items, this->selected, [this](const std::set<int> & chosen)
		{
			this->selected = chosen;
			updateText();

			if(this->callback)
				this->callback(chosen);
		});
	});

	updateText();
}

void CSetPicker::updateText()
{
	std::string text = LIBRARY->generaltexth->translate("vcmi.ui.selectedCount");
	const size_t placeholder = text.find("%s");
	if(placeholder != std::string::npos)
		text.replace(placeholder, 2, std::to_string(selected.size()));

	std::string names;
	size_t shown = 0;
	for(int index : selected)
	{
		if(index < 0 || index >= static_cast<int>(items.size()))
			continue;

		if(shown++ >= HOVER_LIMIT)
		{
			names += ", ...";
			break;
		}

		names += (names.empty() ? "" : ", ") + items[index].text;
	}

	button->setText(text);
	button->setHoverText(names);
}

const std::set<int> & CSetPicker::getSelected() const
{
	return selected;
}

void CSetPicker::setSelected(const std::set<int> & newSelection)
{
	selected = newSelection;
	updateText();
}
