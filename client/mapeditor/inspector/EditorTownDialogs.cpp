/*
 * EditorTownDialogs.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "EditorTownDialogs.h"

#include "../EditorMap.h"
#include "../EditorMapHelpers.h"
#include "../EditorTexts.h"

#include "../../GameEngine.h"
#include "../../gui/WindowHandler.h"
#include "../../widgets/CCheckBox.h"
#include "../../widgets/CFormView.h"
#include "../../widgets/CListEditor.h"
#include "../../widgets/CMultiLineEdit.h"
#include "../../widgets/CNumberInput.h"
#include "../../widgets/CPlayerSetEditor.h"
#include "../../widgets/CResourceEditor.h"
#include "../../widgets/CSelectList.h"
#include "../../widgets/CTabBar.h"
#include "../../widgets/CTextField.h"
#include "../../widgets/TextControls.h"
#include "../../widgets/UiTheme.h"

#include "../../../lib/CCreatureHandler.h"
#include "../../../lib/GameLibrary.h"
#include "../../../lib/constants/NumericConstants.h"
#include "../../../lib/constants/StringConstants.h"
#include "../../../lib/entities/building/CBuilding.h"
#include "../../../lib/entities/faction/CTown.h"
#include "../../../lib/mapObjects/CGTownInstance.h"
#include "../../../lib/mapping/CMap.h"
#include "../../../lib/spells/CSpellHandler.h"

namespace
{
constexpr int LABEL_WIDTH = 150;
constexpr int BUILDINGS_LIST_HEIGHT = 220;
constexpr int MESSAGE_HEIGHT = 90;
constexpr int MAX_EVENT_DAY = 999;
constexpr int MAX_CREATURES_CHANGE = 999999;
}

EditorBuildingTree::EditorBuildingTree(const CTown & town)
{
	std::map<BuildingID, std::vector<BuildingID>> children;
	std::vector<BuildingID> roots;

	for(const auto & [id, building] : town.buildings)
	{
		if(building->getBase() == id || !vstd::contains(town.buildings, building->upgrade))
			roots.push_back(id);
		else
			children[building->upgrade].push_back(id);
	}

	std::function<void(BuildingID, int, int)> add = [&](BuildingID id, int parent, int indent)
	{
		std::string name = town.buildings.at(id)->getNameTranslated();
		if(name.empty())
			name = EditorMapHelpers::defaultBuildingName(id);

		const int index = static_cast<int>(nodes.size());
		nodes.push_back({id, name, parent, indent});

		if(auto found = children.find(id); found != children.end())
			for(const auto & child : found->second)
				add(child, index, indent + 1);
	};

	for(const auto & root : roots)
		add(root, -1, 0);
}

std::vector<int> EditorBuildingTree::subtree(int index) const
{
	std::vector<int> result{index};
	for(int i = index + 1; i < static_cast<int>(nodes.size()) && nodes[i].indent > nodes[index].indent; ++i)
		result.push_back(i);

	return result;
}

std::vector<int> EditorBuildingTree::ancestors(int index) const
{
	std::vector<int> result;
	for(int i = index; i >= 0; i = nodes[i].parent)
		result.push_back(i);

	return result;
}

// ---------------------------------------------------------------------------------------------

EditorTownBuildingsDialog::EditorTownBuildingsDialog(CGTownInstance & town, const std::function<void()> & onChanged)
	: CDialogWindow(Point(620, 560), EditorTexts::get("townBuildings.title"))
	, town(town)
	, tree(*town.getTown())
{
	OBJECT_CONSTRUCTION_TARGETED(content());

	const Point size = contentSize();

	for(const auto & node : tree.nodes)
	{
		enabled.push_back(!vstd::contains(town.forbiddenBuildings, node.id));
		built.push_back(town.hasBuilt(node.id));
	}

	hintLabel = std::make_shared<CMultiLineLabel>(Rect(0, 0, size.x, UiTheme::CONTROL_HEIGHT), UiTheme::FONT, ETextAlignment::TOPLEFT, UiTheme::TEXT, EditorTexts::get("townBuildings.hint"));

	const int listTop = UiTheme::CONTROL_HEIGHT + UiTheme::SPACING;
	list = std::make_shared<CSelectList>(Rect(0, listTop, size.x, size.y - listTop), CSelectList::Mode::CHECK, UiTheme::ROW_HEIGHT);
	list->setRightChecks(2);
	list->onCheck = [this](int row, bool on) { setEnabledFlag(row, on); };
	list->onCheck2 = [this](int row, bool on) { setBuiltFlag(row, on); };

	auto setAll = [this](bool builtColumn, bool on)
	{
		for(int i = 0; i < static_cast<int>(tree.nodes.size()); ++i)
		{
			if(builtColumn)
				setBuiltFlag(i, on);
			else
				setEnabledFlag(i, on);
		}
	};
	addButton(EditorTexts::get("townBuildings.buildAll"), [setAll]() { setAll(true, true); });
	addButton(EditorTexts::get("townBuildings.demolishAll"), [setAll]() { setAll(true, false); });
	addButton(EditorTexts::get("townBuildings.enableAll"), [setAll]() { setAll(false, true); });
	addButton(EditorTexts::get("townBuildings.disableAll"), [setAll]() { setAll(false, false); });

	refresh();

	onAccept = [this, onChanged]()
	{
		commit();
		if(onChanged)
			onChanged();
		return true;
	};
}

void EditorTownBuildingsDialog::refresh()
{
	std::vector<CSelectList::Row> rows;
	for(size_t i = 0; i < tree.nodes.size(); ++i)
	{
		CSelectList::Row row;
		row.text = tree.nodes[i].name;
		row.indent = tree.nodes[i].indent;
		row.checked = enabled[i];
		row.checked2 = built[i];
		rows.push_back(std::move(row));
	}
	list->setRows(std::move(rows), true);
}

void EditorTownBuildingsDialog::setEnabledFlag(int index, bool on)
{
	if(on)
	{
		for(int i : tree.ancestors(index))
			enabled[i] = true;
	}
	else
	{
		for(int i : tree.subtree(index))
		{
			enabled[i] = false;
			built[i] = false;
		}
	}
	refresh();
}

void EditorTownBuildingsDialog::setBuiltFlag(int index, bool on)
{
	if(on)
	{
		for(int i : tree.ancestors(index))
		{
			built[i] = true;
			enabled[i] = true;
		}
	}
	else
	{
		for(int i : tree.subtree(index))
			built[i] = false;
	}
	refresh();
}

void EditorTownBuildingsDialog::commit()
{
	town.forbiddenBuildings.clear();
	town.removeAllBuildings();

	for(size_t i = 0; i < tree.nodes.size(); ++i)
	{
		if(!enabled[i])
			town.forbiddenBuildings.insert(tree.nodes[i].id);
		if(built[i])
			town.addBuilding(tree.nodes[i].id);
	}
}

// ---------------------------------------------------------------------------------------------

EditorTownSpellsDialog::EditorTownSpellsDialog(CGTownInstance & town, const std::function<void()> & onChanged)
	: CDialogWindow(Point(500, 540), EditorTexts::get("townSpells.title"))
	, town(town)
{
	OBJECT_CONSTRUCTION_TARGETED(content());

	const Point size = contentSize();

	for(const auto & spellID : LIBRARY->spellh->getDefaultAllowed())
	{
		const auto * spell = spellID.toEntity(LIBRARY);
		const int spellLevel = spell->getLevel();
		if(spellLevel >= 1 && spellLevel <= GameConstants::SPELL_LEVELS)
			levels[spellLevel - 1].push_back({spellID, spell->getNameTranslated(), vstd::contains(town.possibleSpells, spellID), vstd::contains(town.obligatorySpells, spellID)});
	}

	const int rowHeight = UiTheme::CONTROL_HEIGHT;
	customizeCheck = std::make_shared<CCheckBox>(Rect(0, 0, size.x, rowHeight), EditorTexts::get("townSpells.customize"), vstd::contains(town.possibleSpells, SpellID::PRESET), [this](bool)
	{
		refresh();
	});

	std::vector<std::string> tabNames;
	for(int i = 0; i < GameConstants::SPELL_LEVELS; ++i)
		tabNames.push_back(EditorTexts::format("townSpells.level", {std::to_string(i + 1)}));
	tabs = std::make_shared<CTabBar>(Rect(0, rowHeight + UiTheme::SPACING, size.x, rowHeight), tabNames, 0, [this](int index)
	{
		level = index;
		refresh();
	});

	const int listTop = 2 * (rowHeight + UiTheme::SPACING);
	list = std::make_shared<CSelectList>(Rect(0, listTop, size.x, size.y - listTop), CSelectList::Mode::CHECK, UiTheme::ROW_HEIGHT);
	list->setRightChecks(2);
	list->onCheck = [this](int row, bool on) { levels[level].at(row).possible = on; };
	list->onCheck2 = [this](int row, bool on) { levels[level].at(row).required = on; };

	refresh();

	onAccept = [this, onChanged]()
	{
		commit();
		if(onChanged)
			onChanged();
		return true;
	};
}

void EditorTownSpellsDialog::refresh()
{
	std::vector<CSelectList::Row> rows;
	for(const auto & entry : levels[level])
	{
		CSelectList::Row row;
		row.text = entry.name;
		row.checked = entry.possible;
		row.checked2 = entry.required;
		row.enabled = customizeCheck->isChecked();
		rows.push_back(std::move(row));
	}
	list->setRows(std::move(rows), true);
}

void EditorTownSpellsDialog::commit()
{
	town.possibleSpells.clear();
	town.obligatorySpells.clear();

	if(!customizeCheck->isChecked())
	{
		for(const auto & spellID : LIBRARY->spellh->getDefaultAllowed())
			town.possibleSpells.push_back(spellID);
		return;
	}

	town.possibleSpells.push_back(SpellID::PRESET);
	for(const auto & spells : levels)
	{
		for(const auto & entry : spells)
		{
			if(entry.possible)
				town.possibleSpells.push_back(entry.id);
			if(entry.required)
				town.obligatorySpells.push_back(entry.id);
		}
	}
}

// ---------------------------------------------------------------------------------------------

EditorTownEventDialog::EditorTownEventDialog(const CGTownInstance & town, const Draft & draft, const std::function<void(const Draft &)> & onSaved)
	: CDialogWindow(Point(700, 640), EditorTexts::get("townEvents.eventTitle"))
	, draft(draft)
	, tree(*town.getTown())
{
	OBJECT_CONSTRUCTION_TARGETED(content());

	const Point size = contentSize();
	const CCastleEvent & event = draft.event;
	const int controlWidth = size.x - LABEL_WIDTH - UiTheme::SLIDER_WIDTH - UiTheme::SPACING;

	for(const auto & node : tree.nodes)
		buildingChecks.push_back(vstd::contains(event.buildings, node.id));

	form = std::make_shared<CFormView>(Rect(0, 0, size.x, size.y), LABEL_WIDTH);

	nameField = form->addText(EditorTexts::get("events.name"), event.name, nullptr);

	messageField = std::make_shared<CMultiLineEdit>(Rect(0, 0, controlWidth, MESSAGE_HEIGHT), draft.messageText);
	form->addWidget(EditorTexts::get("events.message"), messageField);

	humanCheck = form->addCheck(EditorTexts::get("events.affectsHuman"), event.humanAffected, nullptr);
	cpuCheck = form->addCheck(EditorTexts::get("events.affectsAi"), event.computerAffected, nullptr);
	firstInput = form->addNumber(EditorTexts::get("events.firstDay"), static_cast<int>(event.firstOccurrence) + 1, 1, MAX_EVENT_DAY, nullptr);
	repeatInput = form->addNumber(EditorTexts::get("events.repeat"), static_cast<int>(event.nextOccurrence), 0, MAX_EVENT_DAY, nullptr);

	playersEditor = std::make_shared<CPlayerSetEditor>(Rect(0, 0, controlWidth, 0), event.players);
	form->addWidget(EditorTexts::get("events.players"), playersEditor);

	resourcesEditor = std::make_shared<CResourceEditor>(Rect(0, 0, controlWidth, 0), event.resources);
	form->addWidget(EditorTexts::get("events.resources"), resourcesEditor);

	buildingsList = std::make_shared<CSelectList>(Rect(0, 0, controlWidth, BUILDINGS_LIST_HEIGHT), CSelectList::Mode::CHECK, UiTheme::ROW_HEIGHT);
	buildingsList->onCheck = [this](int row, bool on)
	{
		if(on)
		{
			for(int i : tree.ancestors(row))
				buildingChecks[i] = true;
		}
		else
		{
			for(int i : tree.subtree(row))
				buildingChecks[i] = false;
		}
		refreshBuildings();
	};
	form->addWidget(EditorTexts::get("townEvents.buildings"), buildingsList);
	refreshBuildings();

	const auto * ctown = town.getTown();
	for(size_t i = 0; i < ctown->creatures.size(); ++i)
	{
		std::string names;
		for(const auto & creature : ctown->creatures.at(i))
			names += (names.empty() ? "" : " / ") + LIBRARY->creatures()->getById(creature)->getNameSingularTranslated();

		const int amount = i < event.creatures.size() ? event.creatures[i] : 0;
		creatureInputs.push_back(form->addNumber(names, amount, 0, MAX_CREATURES_CHANGE, nullptr));
	}

	form->finish();

	onAccept = [this, onSaved]()
	{
		Draft result = this->draft;
		result.event.name = nameField->getText();
		result.messageText = messageField->getText();
		result.event.humanAffected = humanCheck->isChecked();
		result.event.computerAffected = cpuCheck->isChecked();
		result.event.firstOccurrence = static_cast<ui32>(firstInput->getValue() - 1);
		result.event.nextOccurrence = static_cast<ui32>(repeatInput->getValue());
		result.event.players = playersEditor->getValue();
		result.event.resources = resourcesEditor->getValue();

		result.event.buildings.clear();
		for(size_t i = 0; i < tree.nodes.size(); ++i)
			if(buildingChecks[i])
				result.event.buildings.insert(tree.nodes[i].id);

		result.event.creatures.clear();
		for(const auto & input : creatureInputs)
			result.event.creatures.push_back(input->getValue());

		onSaved(result);
		return true;
	};
}

void EditorTownEventDialog::refreshBuildings()
{
	std::vector<CSelectList::Row> rows;
	for(size_t i = 0; i < tree.nodes.size(); ++i)
	{
		CSelectList::Row row;
		row.text = tree.nodes[i].name;
		row.indent = tree.nodes[i].indent;
		row.checked = buildingChecks[i];
		rows.push_back(std::move(row));
	}
	buildingsList->setRows(std::move(rows), true);
}

// ---------------------------------------------------------------------------------------------

EditorTownEventsDialog::EditorTownEventsDialog(EditorMap & editor, CGTownInstance & town, const std::function<void()> & onChanged)
	: CDialogWindow(Point(520, 420), EditorTexts::get("townEvents.title"))
	, editor(editor)
	, town(town)
{
	OBJECT_CONSTRUCTION_TARGETED(content());

	const Point size = contentSize();

	for(const auto & event : town.events)
		events.push_back({event, EditorTexts::translate(event.message)});

	listEditor = std::make_shared<CListEditor>(Rect(0, 0, size.x, size.y), true);
	listEditor->onAdd = [this]()
	{
		Draft draft;
		draft.event.name = EditorTexts::get("townEvents.newEvent");
		events.push_back(draft);
		refreshList(static_cast<int>(events.size()) - 1);
		edit(static_cast<int>(events.size()) - 1);
	};
	listEditor->onEdit = [this](int index) { edit(index); };
	listEditor->onRemove = [this](int index)
	{
		events.erase(events.begin() + index);
		refreshList(-1);
	};

	refreshList(-1);

	onAccept = [this, onChanged]()
	{
		commit();
		if(onChanged)
			onChanged();
		return true;
	};
}

void EditorTownEventsDialog::refreshList(int selected)
{
	std::vector<std::string> names;
	for(const auto & draft : events)
		names.push_back(EditorTexts::format("townEvents.entry", {std::to_string(draft.event.firstOccurrence + 1), draft.event.name}));

	listEditor->setEntries(names, selected);
}

void EditorTownEventsDialog::edit(int index)
{
	ENGINE->windows().createAndPushWindow<EditorTownEventDialog>(town, events.at(index), [this, index](const EditorTownEventDialog::Draft & draft)
	{
		events.at(index) = draft;
		refreshList(index);
	});
}

void EditorTownEventsDialog::commit()
{
	CMap & map = *editor.getMap();

	town.events.clear();
	for(const auto & draft : events)
	{
		CCastleEvent event = draft.event;
		event.message = MetaString();
		event.message.appendTextID(mapRegisterLocalizedString("map", map, TextIdentifier("town", town.instanceName, "event", event.name, "message"), draft.messageText));
		town.events.push_back(event);
	}
}
