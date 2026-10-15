/*
 * EditorRumorsPage.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "EditorRumorsPage.h"

#include "../EditorMap.h"
#include "../EditorTexts.h"

#include "../../widgets/CListEditor.h"
#include "../../widgets/CMultiLineEdit.h"
#include "../../widgets/CTextField.h"
#include "../../widgets/TextControls.h"
#include "../../widgets/UiTheme.h"

#include "../../../lib/mapping/CMap.h"
#include "../../../lib/mapping/CMapEditManager.h"

namespace
{
constexpr int LIST_WIDTH = 240;
}

EditorRumorsPage::EditorRumorsPage(EditorMap & editor, const Rect & area)
	: EditorSettingsPage(editor, area)
{
	OBJECT_CONSTRUCTION;

	for(const auto & rumor : map().rumors)
		rumors.push_back({rumor.name, EditorTexts::translate(rumor.text)});

	listEditor = std::make_shared<CListEditor>(Rect(0, 0, LIST_WIDTH, area.h), false);
	listEditor->onAdd = [this]()
	{
		rumors.push_back({EditorTexts::get("rumors.new"), ""});
		refreshList();
		select(static_cast<int>(rumors.size()) - 1);
	};
	listEditor->onRemove = [this](int index)
	{
		rumors.erase(rumors.begin() + index);
		refreshList();
		select(-1);
	};
	listEditor->onSelect = [this](int index) { select(index); };

	const int detailsLeft = LIST_WIDTH + UiTheme::SPACING;
	const int detailsWidth = area.w - detailsLeft;

	nameLabel = std::make_shared<CLabel>(detailsLeft, UiTheme::CONTROL_HEIGHT / 2, UiTheme::FONT, ETextAlignment::CENTERLEFT, UiTheme::TEXT, EditorTexts::get("rumors.name"));
	nameField = std::make_shared<CTextField>(Rect(detailsLeft + 80, 0, detailsWidth - 80, UiTheme::CONTROL_HEIGHT), "", [this](const std::string & text)
	{
		if(selected >= 0)
		{
			rumors.at(selected).name = text;
			refreshList();
		}
	});

	textLabel = std::make_shared<CLabel>(detailsLeft, UiTheme::CONTROL_HEIGHT * 2 + UiTheme::SPACING, UiTheme::FONT, ETextAlignment::TOPLEFT, UiTheme::TEXT, EditorTexts::get("rumors.text"));
	textField = std::make_shared<CMultiLineEdit>(Rect(detailsLeft, UiTheme::CONTROL_HEIGHT * 3 + UiTheme::SPACING, detailsWidth, area.h - UiTheme::CONTROL_HEIGHT * 3 - UiTheme::SPACING), "", [this](const std::string & text)
	{
		if(selected >= 0)
			rumors.at(selected).text = text;
	});

	refreshList();
	select(-1);
}

void EditorRumorsPage::refreshList()
{
	std::vector<std::string> names;
	for(const auto & rumor : rumors)
		names.push_back(rumor.name);

	listEditor->setEntries(names, selected);
}

void EditorRumorsPage::select(int index)
{
	selected = index;

	const bool has = index >= 0 && index < static_cast<int>(rumors.size());
	nameField->setText(has ? rumors[index].name : "");
	textField->setText(has ? rumors[index].text : "");
	nameField->setReadOnly(!has);

	refreshList();
}

void EditorRumorsPage::apply()
{
	CMap & m = map();

	m.rumors.clear();
	for(size_t i = 0; i < rumors.size(); ++i)
	{
		Rumor rumor;
		rumor.name = rumors[i].name;
		rumor.text.appendTextID(mapRegisterLocalizedString("map", m, TextIdentifier("header", "rumor", static_cast<int>(i), "text"), rumors[i].text));
		m.rumors.push_back(rumor);
	}
}
