/*
 * FlaggableMapObject.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "FlaggableMapObject.h"

#include "CGHeroInstance.h"

#include "../CPlayerState.h"
#include "../callback/IGameInfoCallback.h"
#include "../callback/IGameEventCallback.h"
#include "../gameState/CGameState.h"
#include "../mapObjectConstructors/FlaggableInstanceConstructor.h"
#include "../networkPacks/PacksForClient.h"

VCMI_LIB_NAMESPACE_BEGIN

FlaggableMapObject::FlaggableMapObject(IGameInfoCallback *cb)
	:CGObjectInstance(cb)
	,CBonusSystemNode(BonusNodeType::UNKNOWN)
{}

const IOwnableObject * FlaggableMapObject::asOwnable() const
{
	return this;
}

ResourceSet FlaggableMapObject::dailyIncome() const
{
	return getFlaggableHandler()->getDailyIncome();
}

std::vector<CreatureID> FlaggableMapObject::providedCreatures() const
{
	return {};
}

void FlaggableMapObject::onHeroVisit(IGameEventCallback & gameEvents, const CGHeroInstance * h) const
{
	if (cb->getPlayerRelations(h->getOwner(), getOwner()) != PlayerRelations::ENEMIES)
		return; // H3 behavior - revisiting owned Lighthouse is a no-op

	gameEvents.setOwner(this, h->getOwner()); //not ours? flag it!

	InfoWindow iw;
	iw.player = h->getOwner();
	iw.text.appendTextID(getFlaggableHandler()->getVisitMessageTextID());
	gameEvents.showInfoDialog(&iw);
}

void FlaggableMapObject::initObj(IGameRandomizer & gameRandomizer)
{
	initBonuses();
}

std::shared_ptr<FlaggableInstanceConstructor> FlaggableMapObject::getFlaggableHandler() const
{
	return std::dynamic_pointer_cast<FlaggableInstanceConstructor>(getObjectHandler());
}

void FlaggableMapObject::initBonuses()
{
	for (auto const & bonus : getFlaggableHandler()->getProvidedBonuses())
		addNewBonus(bonus);
}

void FlaggableMapObject::serializeJsonOptions(JsonSerializeFormat& handler)
{
	serializeJsonOwner(handler);
}

void FlaggableMapObject::attachToBonusSystem(CGameState & gs)
{
	if (getOwner().isValidPlayer())
		attachTo(*gs.getPlayerState(getOwner()));
}

void FlaggableMapObject::detachFromBonusSystem(CGameState & gs)
{
	if (getOwner().isValidPlayer())
		detachFrom(*gs.getPlayerState(getOwner()));
}

void FlaggableMapObject::restoreBonusSystem(CGameState & gs)
{
	attachToBonusSystem(gs);
}

VCMI_LIB_NAMESPACE_END
