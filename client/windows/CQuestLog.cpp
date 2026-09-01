/*
 * CQuestLog.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "CQuestLog.h"

#include "../CPlayerInterface.h"

#include "../GameEngine.h"
#include "../GameInstance.h"
#include "../CPlayerInterface.h"
#include "../gui/Shortcut.h"
#include "../widgets/Buttons.h"
#include "../widgets/CComponent.h"
#include "../widgets/Slider.h"
#include "../adventureMap/AdventureMapInterface.h"
#include "../adventureMap/CMinimap.h"
#include "../render/Canvas.h"

#include "../../lib/CConfigHandler.h"
#include "../../lib/GameLibrary.h"
#include "../../lib/callback/CCallback.h"
#include "../../lib/gameState/ScenarioEventJournalEntry.h"
#include "../../lib/gameState/QuestInfo.h"
#include "../../lib/mapObjects/Quest.h"
#include "../../lib/texts/CGeneralTextHandler.h"

struct QuestInfo;

class CAdvmapInterface;

CQuestIcon::CQuestIcon (const AnimationPath &defname, int index, int x, int y) :
	CAnimImage(defname, index, 0, x, y)
{
	addUsedEvents(LCLICK);
}

void CQuestIcon::clickPressed(const Point & cursorPosition)
{
	callback();
}

void CQuestIcon::showAll(Canvas & to)
{
	CanvasClipRectGuard guard(to, parent->pos);
	CAnimImage::showAll(to);
}

CQuestMinimap::CQuestMinimap(const Rect & position)
	: CMinimap(position),
	currentQuest(nullptr)
{
}

void CQuestMinimap::setQuest(const QuestInfo * q)
{
	currentQuest = q;
	markerTiles = q ? q->getMarkerTiles(GAME->interface()->cb.get()) : std::vector<int3>{};
	update();
}

void CQuestMinimap::placeMarks()
{
	OBJECT_CONSTRUCTION;
	icons.clear();

	if(markerTiles.empty())
		return;

	// The minimap shows a single level; follow the source object's level and draw
	// only the markers on it.
	const int level = markerTiles.front().z;
	onMapViewMoved(Rect(), level);

	for(const int3 & tile : markerTiles)
	{
		if(tile.z != level)
			continue;

		Point offset = tileToPixels(tile);
		auto pic = std::make_shared<CQuestIcon>(AnimationPath::builtin("VwSymbol.def"), 3, offset.x, offset.y);
		pic->moveBy(Point(-pic->pos.w/2, -pic->pos.h/2));
		pic->callback = std::bind(&CQuestMinimap::iconClicked, this);
		icons.push_back(pic);
	}
}

void CQuestMinimap::update()
{
	CMinimap::update();
	placeMarks();
}

void CQuestMinimap::iconClicked()
{
	if(currentQuest->hasObjectInstance())
		adventureInt->centerOnTile(currentQuest->getObject(GAME->interface()->cb.get())->visitablePos());
	//moveAdvMapSelection();
}

void CQuestMinimap::showAll(Canvas & to)
{
	CIntObject::showAll(to); // blitting IntObject directly to hide radar
//	for (auto pic : icons)
//		pic->showAll(to);
}

CQuestLog::CQuestLog (const std::vector<QuestInfo> & Quests)
	: JournalWindow(EJournalMode::QUESTS)
	, currentQuest(nullptr)
	, quests(Quests)
{
	OBJECT_CONSTRUCTION;

	minimap = std::make_shared<CQuestMinimap>(Rect(12, 12, 169, 169));
	initializeItems();
}

size_t CQuestLog::getItemCount() const
{
	return quests.size();
}

std::string CQuestLog::getItemText(size_t itemIndex) const
{
	const auto & questInfo = quests.at(itemIndex);
	const auto * quest = questInfo.getQuest(GAME->interface()->cb.get());
	const auto * questObject = questInfo.getObject(GAME->interface()->cb.get());

	MetaString text;
	quest->getQuestlogText(GAME->interface()->cb.get(), text, false);
	if(questInfo.hasObjectInstance())
	{
		const auto * source = questObject ? questObject->asQuestSource() : nullptr;
		const std::string giver = source ? source->getQuestGiverName() : "";
		if(!giver.empty())
		{
			MetaString toSeer;
			toSeer.appendTextID("core.genrltxt.347");
			toSeer.replaceRawString(giver);
			text.replaceRawString(toSeer.toString(&GAME->translator()));
		}
		else if(questObject)
			text.replaceRawString(questObject->getObjectName().toString(&GAME->translator()));
	}
	return text.toString(&GAME->translator());
}

void CQuestLog::onItemSelected(size_t itemIndex)
{
	currentQuest = &quests.at(itemIndex);
	minimap->setQuest(currentQuest);

	MetaString text;
	std::vector<Component> components;
	currentQuest->getQuest(GAME->interface()->cb.get())->getVisitText(GAME->interface()->cb.get(), text, components, true);
	const auto imageSize = components.size() > 4 ? CComponent::small : CComponent::large;
	std::vector<std::shared_ptr<CComponent>> componentWidgets;
	for(const auto & component : components)
		componentWidgets.push_back(std::make_shared<CComponent>(component, imageSize));
	setContent(text.toString(&GAME->translator()), std::move(componentWidgets), components.size() > 4 ? 155 : 130);

	minimap->update();
	redraw();
}

void CQuestLog::updateMinimap()
{
	minimap->update();
}
