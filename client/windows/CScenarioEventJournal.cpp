/*
 * CScenarioEventJournal.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "CScenarioEventJournal.h"

#include "../CPlayerInterface.h"
#include "../GameEngine.h"
#include "../GameInstance.h"
#include "../adventureMap/AdventureMapInterface.h"
#include "../gui/Shortcut.h"
#include "../render/Canvas.h"
#include "../render/Colors.h"
#include "../widgets/Buttons.h"
#include "../widgets/CComponent.h"
#include "../widgets/Slider.h"

#include "../../lib/GameLibrary.h"
#include "../../lib/callback/CCallback.h"
#include "../../lib/entities/ResourceTypeHandler.h"
#include "../../lib/texts/CGeneralTextHandler.h"

CScenarioEventJournalLabel::CScenarioEventJournalLabel(const Rect & position, const std::string & text)
	: CMultiLineLabel(position, FONT_SMALL, ETextAlignment::TOPLEFT, Colors::WHITE, text)
{
}

void CScenarioEventJournalLabel::clickPressed(const Point & cursorPosition)
{
	callback();
}

void CScenarioEventJournalLabel::showAll(Canvas & to)
{
	CMultiLineLabel::showAll(to);
}

CScenarioEventJournalMinimap::CScenarioEventJournalMinimap(const Rect & position)
	: CMinimap(position)
{
}

void CScenarioEventJournalMinimap::setLocation(const int3 & newLocation)
{
	location = newLocation;
	marker.reset();

	const int3 mapSize = GAME->interface()->cb->getMapSize();
	if(location.x < 0 || location.y < 0 || location.z < 0 || location.x >= mapSize.x || location.y >= mapSize.y || location.z >= mapSize.z)
	{
		update();
		return;
	}

	onMapViewMoved(Rect(), location.z);
	update();

	OBJECT_CONSTRUCTION;
	const Point markerPosition = tileToPixels(location);
	marker = std::make_shared<CPicture>(ImagePath::builtin("minimapIcons/generic"), markerPosition);
	marker->moveBy(Point(-marker->pos.w / 2, -marker->pos.h / 2));
	marker->addLClickCallback(std::bind(&CScenarioEventJournalMinimap::markerClicked, this));
	redraw();
}

void CScenarioEventJournalMinimap::markerClicked()
{
	adventureInt->centerOnTile(location);
}

void CScenarioEventJournalMinimap::showAll(Canvas & to)
{
	CIntObject::showAll(to);
}

CScenarioEventJournal::CScenarioEventJournal(const std::vector<ScenarioEventJournalEntry> & journalEntries)
	: CWindowObject(PLAYER_COLORED | BORDERED, ImagePath::builtin("questDialog"))
	, entries(journalEntries)
{
	OBJECT_CONSTRUCTION;

	minimap = std::make_shared<CScenarioEventJournalMinimap>(Rect(12, 12, 169, 169));
	description = std::make_shared<CTextBox>("", Rect(205, 18, 385, DESCRIPTION_HEIGHT), CSlider::BROWN, FONT_MEDIUM, ETextAlignment::TOPLEFT, Colors::WHITE);
	ok = std::make_shared<CButton>(Point(539, 398), AnimationPath::builtin("IOKAY.DEF"), LIBRARY->generaltexth->zelp[445], std::bind(&CScenarioEventJournal::close, this), EShortcut::GLOBAL_RETURN);
	slider = std::make_shared<CSlider>(Point(166, 195), 191, std::bind(&CScenarioEventJournal::sliderMoved, this, _1), VISIBLE_ENTRY_COUNT, static_cast<int>(entries.size()), 0, Orientation::VERTICAL, CSlider::BROWN);
	slider->setPanningStep(32);

	for(size_t i = 0; i < entries.size(); ++i)
	{
		const auto & entry = entries[i];
		const std::string title = entry.title.empty()
			? LIBRARY->generaltexth->translate("vcmi.adventureMap.scenarioEventJournal.event")
			: entry.title;
		const std::string day = LIBRARY->generaltexth->translate("core.genrltxt.64") + " " + std::to_string(entry.day);
		auto label = std::make_shared<CScenarioEventJournalLabel>(Rect(13, 195, 149, 31), title + "\n" + day);
		label->callback = std::bind(&CScenarioEventJournal::selectEntry, this, i, static_cast<int>(i));
		labels.push_back(label);
	}

	const int firstVisible = std::max(0, static_cast<int>(entries.size()) - VISIBLE_ENTRY_COUNT);
	recreateEntryList(firstVisible);
	selectEntry(entries.size() - 1, static_cast<int>(entries.size() - 1));

	if(entries.size() > VISIBLE_ENTRY_COUNT)
	{
		slider->block(false);
		slider->scrollTo(firstVisible);
	}
	else
	{
		slider->block(true);
		slider->scrollToMin();
	}
}

void CScenarioEventJournal::selectEntry(size_t entryIndex, int labelIndex)
{
	selectedLabel = labelIndex;
	const auto & entry = entries.at(entryIndex);

	if(description->slider)
		description->slider->scrollToMin();
	description->setText(entry.message.toString());

	componentsBox.reset();
	std::vector<GameResID> changedResources;
	for(const auto & resource : LIBRARY->resourceTypeHandler->getAllObjects())
	{
		if(entry.resources[resource] != 0)
			changedResources.push_back(resource);
	}

	std::vector<std::shared_ptr<CComponent>> components;
	const auto componentSize = changedResources.size() > 4 ? CComponent::small : CComponent::large;
	for(const auto & resource : changedResources)
	{
		const auto value = entry.resources[resource];
		const std::string subtitle = (value > 0 ? "+" : "") + std::to_string(value);
		components.push_back(std::make_shared<CComponent>(ComponentType::RESOURCE, resource, subtitle, componentSize));
	}

	if(components.empty())
	{
		description->resize(Point(385, DESCRIPTION_HEIGHT));
	}
	else
	{
		const int descriptionHeight = DESCRIPTION_HEIGHT - 130;
		description->resize(Point(385, descriptionHeight));
		OBJECT_CONSTRUCTION;
		componentsBox = std::make_shared<CComponentBox>(components, Rect(205, 18 + descriptionHeight + 15, 385, 115));
	}

	minimap->setLocation(entry.location);
	redraw();
}

void CScenarioEventJournal::recreateEntryList(int firstVisible)
{
	for(size_t i = 0; i < labels.size(); ++i)
	{
		labels[i]->pos = Rect(pos.x + 14, pos.y + 195 + (static_cast<int>(i) - firstVisible) * 32, 151, 31);
		if(static_cast<int>(i) >= firstVisible && static_cast<int>(i) < firstVisible + VISIBLE_ENTRY_COUNT)
			labels[i]->enable();
		else
			labels[i]->disable();
	}
}

void CScenarioEventJournal::sliderMoved(int newPosition)
{
	recreateEntryList(newPosition);
	redraw();
}

void CScenarioEventJournal::showAll(Canvas & to)
{
	CWindowObject::showAll(to);
	if(selectedLabel >= 0 && selectedLabel < labels.size())
	{
		Rect selection = Rect::createAround(labels[selectedLabel]->pos, 1);
		selection.x -= 2;
		selection.w += 2;
		to.drawBorder(selection, Colors::METALLIC_GOLD);
	}
}
