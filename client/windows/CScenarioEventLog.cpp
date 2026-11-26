/*
 * CScenarioEventLog.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "CScenarioEventLog.h"

#include "../CPlayerInterface.h"
#include "../GameEngine.h"
#include "../GameInstance.h"
#include "../adventureMap/AdventureMapInterface.h"
#include "../gui/Shortcut.h"
#include "render/Canvas.h"
#include "../render/Colors.h"
#include "../widgets/Buttons.h"
#include "../widgets/CComponent.h"
#include "../widgets/Slider.h"

#include "../../lib/GameLibrary.h"
#include "../../lib/callback/CCallback.h"
#include "../../lib/texts/CGeneralTextHandler.h"

ScenarioEventLogMinimap::ScenarioEventLogMinimap(const Rect & position)
	: CMinimap(position)
{
}

void ScenarioEventLogMinimap::setLocation(const int3 & newLocation)
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
	marker->addLClickCallback(std::bind(&ScenarioEventLogMinimap::markerClicked, this));
	redraw();
}

void ScenarioEventLogMinimap::markerClicked() const
{
	adventureInt->centerOnTile(location);
}

void ScenarioEventLogMinimap::showAll(Canvas & to)
{
	CIntObject::showAll(to);
}

CScenarioEventLog::CScenarioEventLog(const std::vector<ScenarioEventJournalEntry> & journalEntries)
	: JournalWindow(EJournalMode::EVENTS)
	, entries(journalEntries)
{
	OBJECT_CONSTRUCTION;

	minimap = std::make_shared<ScenarioEventLogMinimap>(Rect(12, 12, 169, 169));
	initializeItems();
	if(entries.empty())
		setContent(LIBRARY->generaltexth->translate("vcmi.adventureMap.journal.noEvents"), {});
}

size_t CScenarioEventLog::getItemCount() const
{
	return entries.size();
}

std::string CScenarioEventLog::getItemText(size_t itemIndex) const
{
	return LIBRARY->generaltexth->translate("core.genrltxt.64") + " " + std::to_string(entries.at(itemIndex).day);
}

void CScenarioEventLog::onItemSelected(size_t itemIndex)
{
	const auto & entry = entries.at(itemIndex);
	std::vector<std::shared_ptr<CComponent>> components;
	const auto componentSize = entry.components.size() > 4 ? CComponent::small : CComponent::large;
	for(const auto & component : entry.components)
		components.push_back(std::make_shared<CComponent>(component, componentSize));

	setContent(entry.message.toString(&GAME->translator()), std::move(components));
	minimap->setLocation(entry.location);
}

void CScenarioEventLog::updateMinimap()
{
	minimap->update();
}
