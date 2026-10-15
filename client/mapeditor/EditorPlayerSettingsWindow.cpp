/*
 * EditorPlayerSettingsWindow.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "EditorPlayerSettingsWindow.h"

#include "EditorMap.h"
#include "EditorMapHelpers.h"
#include "EditorTexts.h"

#include "../GameEngine.h"
#include "../gui/WindowHandler.h"
#include "../widgets/CCheckBox.h"
#include "../widgets/CDropDown.h"
#include "../widgets/CFormView.h"
#include "../widgets/CNumberInput.h"
#include "../widgets/CSelectList.h"
#include "../widgets/CTabBar.h"
#include "../widgets/CTextButton.h"
#include "../widgets/UiTheme.h"

#include "../../lib/GameLibrary.h"
#include "../../lib/entities/faction/CFaction.h"
#include "../../lib/entities/faction/CTownHandler.h"
#include "../../lib/mapObjects/CGTownInstance.h"
#include "../../lib/mapping/CMap.h"

namespace
{
constexpr int FACTION_ROWS = 6;
}

EditorPlayerSettingsWindow::EditorPlayerSettingsWindow(EditorMap & editor, const std::function<void()> & onClosed)
	: CDialogWindow(Point(640, 560), EditorTexts::get("players.title"))
	, editor(editor)
	, onClosed(onClosed)
{
	OBJECT_CONSTRUCTION_TARGETED(content());

	const Point size = contentSize();
	const CMap & map = *editor.getMap();

	for(int i = 0; i < PlayerColor::PLAYER_LIMIT_I; ++i)
	{
		if(map.players[i].canAnyonePlay())
		{
			originalColors.insert(i);
			entries.push_back(makeEntry(i));
		}
	}

	countInput = std::make_shared<CNumberInput>(Rect(size.x - 120, 0, 120, UiTheme::CONTROL_HEIGHT), static_cast<int>(entries.size()), 1, PlayerColor::PLAYER_LIMIT_I, [this](int count)
	{
		setPlayerCount(count);
	});

	tabs = std::make_shared<CTabBar>(Rect(0, UiTheme::CONTROL_HEIGHT + UiTheme::SPACING, size.x, UiTheme::CONTROL_HEIGHT + 4), std::vector<std::string>{}, 0, [this](int index)
	{
		active = index;
		rebuildForm();
	});

	form = std::make_shared<CFormView>(Rect(0, 2 * UiTheme::CONTROL_HEIGHT + 2 * UiTheme::SPACING + 4, size.x, size.y - 2 * UiTheme::CONTROL_HEIGHT - 2 * UiTheme::SPACING - 4), 180);

	for(int i = 0; i < map.objects.size(); ++i)
		if(dynamic_cast<const CGTownInstance *>(map.objects.at(i).get()))
			townObjects.push_back(i);

	rebuildTabs();
	rebuildForm();

	onAccept = [this]()
	{
		apply();
		return true;
	};
	afterAccept = onClosed;
}

EditorPlayerSettingsWindow::Entry EditorPlayerSettingsWindow::makeEntry(int color) const
{
	const CMap & map = *editor.getMap();

	Entry entry;
	entry.color = color;
	entry.info = map.players[color];
	entry.info.canComputerPlay = true; //computer always can play

	bool foundMainTown = false;
	for(const auto & object : map.objects)
	{
		const auto * town = dynamic_cast<const CGTownInstance *>(object.get());
		if(town && town->getTown() && town->getOwner().getNum() == color && entry.info.hasMainTown && entry.info.posOfMainTown == town->pos)
			foundMainTown = true;
	}

	if(!foundMainTown)
	{
		entry.info.hasMainTown = false;
		entry.info.generateHeroAtMainTown = false;
		entry.info.posOfMainTown = int3(-1, -1, -1);
	}
	return entry;
}

void EditorPlayerSettingsWindow::rebuildTabs()
{
	std::vector<std::string> labels;
	for(const auto & entry : entries)
		labels.push_back(EditorTexts::format("players.tab", {std::to_string(entry.color + 1)}));

	active = std::min(active, static_cast<int>(entries.size()) - 1);
	tabs->setLabels(labels);
	tabs->setSelected(active);
}

void EditorPlayerSettingsWindow::rebuildForm()
{
	form->clear();

	if(entries.empty())
		return;

	auto & entry = entries.at(active);
	const CMap & map = *editor.getMap();

	form->addInfo(EditorTexts::get("players.id"), std::to_string(entry.color + 1));

	// color: the player's own and the ones nobody else uses
	std::vector<std::string> colorNames;
	std::vector<int> colors;
	int selectedColor = 0;
	for(int i = 0; i < PlayerColor::PLAYER_LIMIT_I; ++i)
	{
		const bool usedByOther = std::any_of(entries.begin(), entries.end(), [&](const Entry & other) { return other.color == i && i != entry.color; });
		if(usedByOther)
			continue;

		if(i == entry.color)
			selectedColor = static_cast<int>(colors.size());

		colorNames.push_back(EditorTexts::playerName(PlayerColor(i)));
		colors.push_back(i);
	}
	form->addChoice(EditorTexts::get("players.color"), colorNames, selectedColor, [this, colors](int index)
	{
		entries.at(active).color = colors.at(index);
		rebuildTabs();
		rebuildForm();
	});

	std::vector<std::string> teams{EditorTexts::get("players.noTeam")};
	for(int i = 0; i < PlayerColor::PLAYER_LIMIT_I; ++i)
		teams.push_back(std::to_string(i + 1));
	form->addChoice(EditorTexts::get("players.team"), teams, entry.info.team == TeamID::NO_TEAM ? 0 : entry.info.team.getNum() + 1, [this](int index)
	{
		entries.at(active).info.team = index == 0 ? TeamID::NO_TEAM : TeamID(index - 1);
	});

	form->addChoice(EditorTexts::get("players.control"), {EditorTexts::get("players.controlCpu"), EditorTexts::get("players.controlHuman")}, entry.info.canHumanPlay ? 1 : 0, [this](int index)
	{
		entries.at(active).info.canHumanPlay = index == 1;
	});

	form->addCheck(EditorTexts::get("players.randomFaction"), entry.info.isFactionRandom, [this](bool on)
	{
		entries.at(active).info.isFactionRandom = on;
	});

	// allowed factions
	{
		const auto factions = LIBRARY->townh->getAllowedFactions();
		const int listHeight = FACTION_ROWS * UiTheme::ROW_HEIGHT;
		auto factionList = std::make_shared<CSelectList>(Rect(0, 0, form->contentWidth() - 180, listHeight), CSelectList::Mode::CHECK, UiTheme::ROW_HEIGHT);

		std::vector<CSelectList::Row> rows;
		std::vector<FactionID> ids;
		for(const auto & id : factions)
		{
			CSelectList::Row row;
			row.text = LIBRARY->townh->objects.at(id)->getNameTranslated();
			row.checked = entry.info.allowedFactions.count(id) != 0;
			rows.push_back(std::move(row));
			ids.push_back(id);
		}
		factionList->setRows(std::move(rows));
		factionList->onCheck = [this, ids](int index, bool on)
		{
			if(on)
				entries.at(active).info.allowedFactions.insert(ids.at(index));
			else
				entries.at(active).info.allowedFactions.erase(ids.at(index));
		};
		form->addWidget(EditorTexts::get("players.factions"), factionList);
	}

	// main town
	std::vector<std::string> townNames{EditorTexts::get("players.townDefault")};
	std::vector<int> ownedTowns;
	int selectedTown = 0;
	for(int index : townObjects)
	{
		const auto * town = dynamic_cast<const CGTownInstance *>(map.objects.at(index).get());
		if(!town || !town->getTown() || town->getOwner().getNum() != entry.color)
			continue;

		if(entry.info.hasMainTown && entry.info.posOfMainTown == town->pos)
			selectedTown = static_cast<int>(ownedTowns.size()) + 1;

		townNames.push_back(EditorMapHelpers::getTownName(map, index) + " @ " + town->pos.toString());
		ownedTowns.push_back(index);
	}

	auto heroCheck = std::make_shared<CCheckBox>(Rect(0, 0, form->contentWidth() - 180, UiTheme::CONTROL_HEIGHT), "", entry.info.generateHeroAtMainTown, [this](bool on)
	{
		entries.at(active).info.generateHeroAtMainTown = on;
	});
	heroCheck->block(!entry.info.hasMainTown);

	form->addChoice(EditorTexts::get("players.mainTown"), townNames, selectedTown, [this, ownedTowns, heroCheck](int index)
	{
		auto & info = entries.at(active).info;
		if(index == 0)
		{
			info.hasMainTown = false;
			info.generateHeroAtMainTown = false;
			info.posOfMainTown = int3(-1, -1, -1);
			heroCheck->setChecked(false);
			heroCheck->block(true);
		}
		else
		{
			info.hasMainTown = true;
			info.posOfMainTown = editor.getMap()->objects.at(ownedTowns.at(index - 1))->pos;
			heroCheck->block(false);
		}
	});

	form->addButton("", EditorTexts::get("players.pickTown"), [this]() { pickMainTown(); });
	form->addWidget(EditorTexts::get("players.generateHero"), heroCheck);

	form->finish();
}

void EditorPlayerSettingsWindow::setPlayerCount(int count)
{
	std::set<int> used;
	for(const auto & entry : entries)
		used.insert(entry.color);

	while(static_cast<int>(entries.size()) < count)
	{
		int color = 0;
		while(used.count(color))
			++color;

		used.insert(color);
		entries.push_back(makeEntry(color));
	}

	while(static_cast<int>(entries.size()) > count)
		entries.pop_back();

	rebuildTabs();
	rebuildForm();
}

void EditorPlayerSettingsWindow::pickMainTown()
{
	const int color = entries.at(active).color;
	auto self = ENGINE->windows().topWindow<EditorPlayerSettingsWindow>();
	if(!self || !editor.pickObject)
		return;

	auto filter = [color](const CGObjectInstance * object)
	{
		const auto * town = dynamic_cast<const CGTownInstance *>(object);
		return town && town->getOwner().getNum() == color;
	};

	// the window hides itself, so the map can be clicked
	ENGINE->windows().popWindow(self);

	editor.pickObject(filter, [self](const CGObjectInstance * object)
	{
		ENGINE->windows().pushWindow(self);

		if(!object)
			return;

		auto & info = self->entries.at(self->active).info;
		info.hasMainTown = true;
		info.posOfMainTown = object->pos;
		self->rebuildForm();
	});
}

void EditorPlayerSettingsWindow::apply()
{
	CMap & map = *editor.getMap();

	std::set<int> kept;
	for(const auto & entry : entries)
		kept.insert(entry.color);

	for(int color : originalColors)
	{
		if(!kept.count(color))
		{
			map.players[color].canHumanPlay = false;
			map.players[color].canComputerPlay = false;
		}
	}

	for(const auto & entry : entries)
		map.players[entry.color] = entry.info;

	editor.commitChangeWithoutRedraw();
}

void EditorPlayerSettingsWindow::show(EditorMap & editor, const std::function<void()> & onClosed)
{
	ENGINE->windows().createAndPushWindow<EditorPlayerSettingsWindow>(editor, onClosed);
}
