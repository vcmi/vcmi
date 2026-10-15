/*
 * EditorAllowedPage.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "EditorAllowedPage.h"

#include "../EditorMap.h"

#include "../../widgets/CSelectList.h"
#include "../../widgets/CTextButton.h"
#include "../../widgets/CTextField.h"
#include "../../widgets/UiTheme.h"

#include "../../../lib/CSkillHandler.h"
#include "../../../lib/GameLibrary.h"
#include "../../../lib/entities/artifact/CArtHandler.h"
#include "../../../lib/mapping/CMap.h"
#include "../../../lib/spells/CSpellHandler.h"
#include "../../../lib/texts/CGeneralTextHandler.h"

#include <boost/algorithm/string/predicate.hpp>

EditorAllowedPage::EditorAllowedPage(EditorMap & editor, const Rect & area, Kind kind)
	: EditorSettingsPage(editor, area)
	, kind(kind)
{
	OBJECT_CONSTRUCTION;

	const CMap & m = map();

	switch(kind)
	{
		case Kind::ABILITIES:
			for(auto const & object : LIBRARY->skillh->objects)
				entries.push_back({object->getIndex(), object->getNameTranslated(), m.allowedAbilities.count(object->getId()) != 0});
			break;

		case Kind::SPELLS:
			for(auto const & object : LIBRARY->spellh->objects)
				if(object->isCommonHeroSpell())
					entries.push_back({object->getIndex(), object->getNameTranslated(), m.allowedSpells.count(object->getId()) != 0});
			break;

		case Kind::ARTIFACTS:
			for(auto const & object : LIBRARY->arth->objects)
				entries.push_back({object->getIndex(), object->getNameTranslated(), m.allowedArtifact.count(object->getId()) != 0});
			break;
	}

	const int buttonWidth = 100;
	filterField = std::make_shared<CTextField>(Rect(0, 0, area.w - 2 * buttonWidth - 2 * UiTheme::SPACING, UiTheme::CONTROL_HEIGHT), "", [this](const std::string &) { applyFilter(); });
	allButton = std::make_shared<CTextButton>(Rect(area.w - 2 * buttonWidth - UiTheme::SPACING, 0, buttonWidth, UiTheme::CONTROL_HEIGHT),
		LIBRARY->generaltexth->translate("vcmi.ui.selectAll"), [this]() { setShownChecked(true); });
	noneButton = std::make_shared<CTextButton>(Rect(area.w - buttonWidth, 0, buttonWidth, UiTheme::CONTROL_HEIGHT),
		LIBRARY->generaltexth->translate("vcmi.ui.selectNone"), [this]() { setShownChecked(false); });

	const int listTop = UiTheme::CONTROL_HEIGHT + UiTheme::SPACING;
	list = std::make_shared<CSelectList>(Rect(0, listTop, area.w, area.h - listTop), CSelectList::Mode::CHECK, UiTheme::ROW_HEIGHT);
	list->onCheck = [this](int row, bool on)
	{
		entries.at(shown.at(row)).allowed = on;
	};

	applyFilter();
}

void EditorAllowedPage::applyFilter()
{
	const std::string filter = filterField->getText();

	shown.clear();
	std::vector<CSelectList::Row> rows;
	for(size_t i = 0; i < entries.size(); ++i)
	{
		if(!filter.empty() && !boost::algorithm::icontains(entries[i].name, filter))
			continue;

		CSelectList::Row row;
		row.text = entries[i].name;
		row.checked = entries[i].allowed;
		rows.push_back(std::move(row));
		shown.push_back(static_cast<int>(i));
	}
	list->setRows(std::move(rows));
}

void EditorAllowedPage::setShownChecked(bool on)
{
	for(int index : shown)
		entries[index].allowed = on;

	applyFilter();
}

void EditorAllowedPage::apply()
{
	CMap & m = map();

	switch(kind)
	{
		case Kind::ABILITIES:
			m.allowedAbilities.clear();
			for(const auto & entry : entries)
				if(entry.allowed)
					m.allowedAbilities.emplace(entry.id);
			break;

		case Kind::SPELLS:
			m.allowedSpells.clear();
			for(const auto & entry : entries)
				if(entry.allowed)
					m.allowedSpells.emplace(entry.id);
			break;

		case Kind::ARTIFACTS:
			m.allowedArtifact.clear();
			for(const auto & entry : entries)
				if(entry.allowed)
					m.allowedArtifact.emplace(entry.id);
			break;
	}
}
