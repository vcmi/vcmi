/*
 * MiscObjects.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "CGResource.h"

#include "../callback/IGameInfoCallback.h"
#include "../callback/IGameEventCallback.h"
#include "../callback/IGameRandomizer.h"
#include "../mapObjectConstructors/CommonConstructors.h"
#include "../texts/CGeneralTextHandler.h"
#include "../networkPacks/PacksForClient.h"
#include "../networkPacks/PacksForClientBattle.h"
#include "../gameState/CGameState.h"
#include "../serializer/JsonSerializeFormat.h"
#include "../CSoundBase.h"

#include <vstd/RNG.h>

VCMI_LIB_NAMESPACE_BEGIN

std::shared_ptr<ResourceInstanceConstructor> CGResource::getResourceHandler() const
{
	const auto & baseHandler = getObjectHandler();
	const auto & ourHandler = std::dynamic_pointer_cast<ResourceInstanceConstructor>(baseHandler);
	return ourHandler;
}

int CGResource::getAmountMultiplier() const
{
	return getResourceHandler()->getAmountMultiplier();
}

uint32_t CGResource::getAmount() const
{
	return amount;
}

GameResID CGResource::resourceID() const
{
	return getResourceHandler()->getResourceType();
}

std::string CGResource::getHoverText(PlayerColor player) const
{
	return LIBRARY->generaltexth->restypes[resourceID().getNum()];
}

void CGResource::pickRandomObject(IGameRandomizer & gameRandomizer)
{
	assert(ID == Obj::RESOURCE || ID == Obj::RANDOM_RESOURCE);

	if (ID == Obj::RANDOM_RESOURCE)
	{
		ID = Obj::RESOURCE;
		subID = gameRandomizer.getDefault().nextInt(EGameResID::WOOD, EGameResID::GOLD);
		setType(ID, subID);

		amount *= getAmountMultiplier();
	}
}

void CGResource::initObj(IGameRandomizer & gameRandomizer)
{
	blockVisit = true;
	getResourceHandler()->randomizeObject(this, gameRandomizer);
}

void CGResource::onHeroVisit(IGameEventCallback & gameEvents, const CGHeroInstance * h) const
{
	if(stacksCount())
	{
		if(!message.empty())
		{
			BlockingDialog ynd(true,false);
			ynd.player = h->getOwner();
			ynd.text = message;
			gameEvents.showBlockingDialog(this, &ynd);
		}
		else
		{
			blockingDialogAnswered(gameEvents, h, true); //behave as if player accepted battle
		}
	}
	else
		collectRes(gameEvents, h->getOwner());
}

void CGResource::collectRes(IGameEventCallback & gameEvents, const PlayerColor & player) const
{
	gameEvents.giveResource(player, resourceID(), amount);
	InfoWindow sii;
	sii.player = player;
	if(!message.empty())
	{
		sii.type = EInfoWindowMode::AUTO;
		sii.text = message;
	}
	else
	{
		sii.type = EInfoWindowMode::INFO;
		sii.text.appendLocalString(EMetaText::ADVOB_TXT,113);
		sii.text.replaceName(resourceID());
	}
	sii.components.emplace_back(ComponentType::RESOURCE, resourceID(), amount);
	sii.soundID = soundBase::pickup01 + gameEvents.getRandomGenerator().nextInt(6);
	gameEvents.showInfoDialog(&sii);
	gameEvents.removeObject(this, player);
}

void CGResource::battleFinished(IGameEventCallback & gameEvents, const CGHeroInstance *hero, const BattleResult &result) const
{
	if(result.winner == BattleSide::ATTACKER) //attacker won
		collectRes(gameEvents, hero->getOwner());
}

void CGResource::blockingDialogAnswered(IGameEventCallback & gameEvents, const CGHeroInstance *hero, int32_t answer) const
{
	if(answer)
		gameEvents.startBattle(hero, this);
}

void CGResource::serializeJsonOptions(JsonSerializeFormat & handler)
{
	CArmedInstance::serializeJsonOptions(handler);
	if(!handler.saving && !handler.getCurrent()["guards"].Vector().empty())
		CCreatureSet::serializeJson(handler, "guards", 7);
	handler.serializeInt("amount", amount, 0);
	handler.serializeStruct("guardMessage", message);
}


VCMI_LIB_NAMESPACE_END
