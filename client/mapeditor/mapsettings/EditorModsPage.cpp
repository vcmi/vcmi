/*
 * EditorModsPage.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "EditorModsPage.h"

#include "../EditorTexts.h"

#include "../../widgets/CSelectList.h"
#include "../../widgets/CTextButton.h"
#include "../../widgets/UiTheme.h"

#include "../../../lib/GameLibrary.h"
#include "../../../lib/mapping/CMap.h"
#include "../../../lib/modding/CModHandler.h"
#include "../../../lib/modding/ModDescription.h"

EditorModsPage::EditorModsPage(EditorMap & editor, const Rect & area)
	: EditorSettingsPage(editor, area)
{
	OBJECT_CONSTRUCTION;

	const CMap & m = map();

	// mods go after their parent mod
	const auto active = LIBRARY->modh->getActiveMods();
	std::set<std::string> added;
	std::function<void(const std::string &, int)> addWithChildren = [&](const std::string & modName, int depth)
	{
		const auto & info = LIBRARY->modh->getModInfo(modName);
		entries.push_back({modName, depth, m.mods.count(modName) != 0, info.getName(), info.getVersion().toString()});
		added.insert(modName);

		for(const auto & other : active)
		{
			const size_t dot = other.rfind('.');
			if(dot != std::string::npos && other.substr(0, dot) == modName && !added.count(other))
				addWithChildren(other, depth + 1);
		}
	};

	for(const auto & modName : active)
	{
		if(modName == "core" || added.count(modName))
			continue;

		const size_t dot = modName.rfind('.');
		const bool hasParent = dot != std::string::npos && vstd::contains(active, modName.substr(0, dot));
		if(!hasParent)
			addWithChildren(modName, 0);
	}

	// a mod is required together with all its parents
	for(size_t i = 0; i < entries.size(); ++i)
		if(entries[i].required)
			requireParents(i);

	const int buttonWidth = (area.w - UiTheme::SPACING) / 2;
	mapModsButton = std::make_shared<CTextButton>(Rect(0, 0, buttonWidth, UiTheme::CONTROL_HEIGHT), EditorTexts::get("mods.fromMap"), [this]()
	{
		selectMods(EditorMap::modAssessmentMap(map()));
	});
	allModsButton = std::make_shared<CTextButton>(Rect(buttonWidth + UiTheme::SPACING, 0, buttonWidth, UiTheme::CONTROL_HEIGHT), EditorTexts::get("mods.all"), [this]()
	{
		selectMods(EditorMap::modAssessmentAll());
	});

	const int listTop = UiTheme::CONTROL_HEIGHT + UiTheme::SPACING;
	list = std::make_shared<CSelectList>(Rect(0, listTop, area.w, area.h - listTop), CSelectList::Mode::CHECK, UiTheme::ROW_HEIGHT);
	list->onCheck = [this](int row, bool on)
	{
		setRequired(static_cast<size_t>(row), on);
		refreshList();
	};

	refreshList();
}

void EditorModsPage::refreshList()
{
	std::vector<CSelectList::Row> rows;
	for(const auto & entry : entries)
	{
		CSelectList::Row row;
		row.text = entry.name;
		row.detail = entry.version;
		row.indent = entry.depth;
		row.checked = entry.required;
		rows.push_back(std::move(row));
	}
	list->setRows(std::move(rows), true);
}

void EditorModsPage::setRequired(size_t index, bool required)
{
	entries[index].required = required;

	// children follow the state of the mod
	for(size_t i = index + 1; i < entries.size() && entries[i].depth > entries[index].depth; ++i)
		entries[i].required = required;

	if(required)
		requireParents(index);
}

void EditorModsPage::requireParents(size_t index)
{
	int depth = entries[index].depth;
	for(size_t i = index; i-- > 0 && depth > 0;)
	{
		if(entries[i].depth < depth)
		{
			entries[i].required = true;
			depth = entries[i].depth;
		}
	}
}

void EditorModsPage::selectMods(const ModCompatibilityInfo & mods)
{
	for(auto & entry : entries)
		entry.required = mods.count(entry.id) != 0;

	// parents of the selected mods are selected as well
	for(size_t i = 0; i < entries.size(); ++i)
		if(entries[i].required)
			setRequired(i, true);

	refreshList();
}

void EditorModsPage::apply()
{
	CMap & m = map();

	m.mods.clear();
	for(const auto & entry : entries)
		if(entry.required)
			m.mods[entry.id] = LIBRARY->modh->getModInfo(entry.id).getVerificationInfo();
}
