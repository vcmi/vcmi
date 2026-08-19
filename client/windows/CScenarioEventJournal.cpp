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
#include "../../lib/gameState/QuestInfo.h"
#include "../../lib/texts/CGeneralTextHandler.h"

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

void CScenarioEventJournalMinimap::markerClicked() const
{
	adventureInt->centerOnTile(location);
}

void CScenarioEventJournalMinimap::showAll(Canvas & to)
{
	CIntObject::showAll(to);
}

CScenarioEventJournal::CScenarioEventJournal(const std::vector<ScenarioEventJournalEntry> & journalEntries)
	: CJournalWindow(EJournalMode::EVENTS)
	, entries(journalEntries)
{
	OBJECT_CONSTRUCTION;

	minimap = std::make_shared<CScenarioEventJournalMinimap>(Rect(12, 12, 169, 169));
	initializeItems();
}

size_t CScenarioEventJournal::getItemCount() const
{
	return entries.size();
}

std::string CScenarioEventJournal::getItemText(size_t itemIndex) const
{
	return LIBRARY->generaltexth->translate("core.genrltxt.64") + " " + std::to_string(entries.at(itemIndex).day);
}

void CScenarioEventJournal::onItemSelected(size_t itemIndex)
{
	const auto & entry = entries.at(itemIndex);
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

	setContent(entry.message.toString(), std::move(components));
	minimap->setLocation(entry.location);
}

void CScenarioEventJournal::updateMinimap()
{
	minimap->update();
}
