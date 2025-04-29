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

#include "../IGameCallback.h"
#include "CGHeroInstance.h"
#include "../networkPacks/PacksForClient.h"
#include "../mapObjectConstructors/FlaggableInstanceConstructor.h"

VCMI_LIB_NAMESPACE_BEGIN

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

void FlaggableMapObject::onHeroVisit( const CGHeroInstance * h ) const
{
	if (cb->getPlayerRelations(h->getOwner(), getOwner()) != PlayerRelations::ENEMIES)
		return; // H3 behavior - revisiting owned Lighthouse is a no-op

	if (getOwner().isValidPlayer())
		takeBonusFrom(getOwner());

	cb->setOwner(this, h->getOwner()); //not ours? flag it!

	InfoWindow iw;
	iw.player = h->getOwner();
	iw.text.appendTextID(getFlaggableHandler()->getVisitMessageTextID());
	cb->showInfoDialog(&iw);

	giveBonusTo(h->getOwner());
}

void FlaggableMapObject::markAsDeleted() const
{
	if(getOwner().isValidPlayer())
		takeBonusFrom(getOwner());
}

void FlaggableMapObject::initObj(vstd::RNG & rand)
{
	if(getOwner().isValidPlayer())
	{
		// FIXME: This is dirty hack
		giveBonusTo(getOwner(), true);
	}
}

std::shared_ptr<FlaggableInstanceConstructor> FlaggableMapObject::getFlaggableHandler() const
{
	return std::dynamic_pointer_cast<FlaggableInstanceConstructor>(getObjectHandler());
}

void FlaggableMapObject::giveBonusTo(const PlayerColor & player, bool onInit) const
{
	for (auto const & bonus : getFlaggableHandler()->getProvidedBonuses())
	{
		GiveBonus gb(GiveBonus::ETarget::PLAYER);
		gb.id = player;
		gb.bonus = *bonus;

		// FIXME: better place for this code?
		gb.bonus.duration = BonusDuration::PERMANENT;
		gb.bonus.source = BonusSource::OBJECT_INSTANCE;
		gb.bonus.sid = BonusSourceID(id);

		// FIXME: This is really dirty hack
		// Proper fix would be to make FlaggableMapObject into bonus system node
		// Unfortunately this will cause saves breakage
		if(onInit)
			gb.applyGs(&cb->gameState());
		else
			cb->sendAndApply(gb);
	}
}

void FlaggableMapObject::takeBonusFrom(const PlayerColor & player) const
{
	RemoveBonus rb(GiveBonus::ETarget::PLAYER);
	rb.whoID = player;
	rb.source = BonusSource::OBJECT_INSTANCE;
	rb.id = BonusSourceID(id);
	cb->sendAndApply(rb);
}

void FlaggableMapObject::serializeJsonOptions(JsonSerializeFormat& handler)
{
	serializeJsonOwner(handler);
}

VCMI_LIB_NAMESPACE_END
