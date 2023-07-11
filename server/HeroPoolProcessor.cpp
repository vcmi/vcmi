/*
 * HeroPoolProcessor.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "HeroPoolProcessor.h"

#include "CGameHandler.h"

#include "../lib/CHeroHandler.h"
#include "../lib/CPlayerState.h"
#include "../lib/GameSettings.h"
#include "../lib/NetPacks.h"
#include "../lib/StartInfo.h"
#include "../lib/mapObjects/CGTownInstance.h"
#include "../lib/gameState/CGameState.h"
#include "../lib/gameState/TavernHeroesPool.h"

HeroPoolProcessor::HeroPoolProcessor() = default;

HeroPoolProcessor::HeroPoolProcessor(CGameHandler * gameHandler):
	gameHandler(gameHandler)
{
}

void HeroPoolProcessor::onHeroSurrendered(const PlayerColor & color, const CGHeroInstance * hero)
{
	SetAvailableHero sah;
	sah.slotID = 0;
	sah.player = color;
	sah.hid = hero->subID;
	sah.army.clear();
	sah.army.setCreature(SlotID(0), hero->type->initialArmy.at(0).creature, 1);
	gameHandler->sendAndApply(&sah);
}

void HeroPoolProcessor::onHeroEscaped(const PlayerColor & color, const CGHeroInstance * hero)
{
	SetAvailableHero sah;
	sah.slotID = 0;
	sah.player = color;
	sah.hid = hero->subID;

	gameHandler->sendAndApply(&sah);
}

void HeroPoolProcessor::clearHeroFromSlot(const PlayerColor & color, TavernHeroSlot slot)
{
	SetAvailableHero sah;
	sah.player = color;
	sah.slotID = static_cast<int>(slot);
	sah.hid = HeroTypeID::NONE;
	gameHandler->sendAndApply(&sah);
}

void HeroPoolProcessor::selectNewHeroForSlot(const PlayerColor & color, TavernHeroSlot slot)
{
	SetAvailableHero sah;
	sah.player = color;
	sah.slotID = static_cast<int>(slot);

	//first hero - native if possible, second hero -> any other class
	CGHeroInstance *h = gameHandler->gameState()->hpool->pickHeroFor(slot, color, gameHandler->getPlayerSettings(color)->castle, gameHandler->getRandomGenerator());

	if (h)
	{
		sah.hid = h->subID;
		h->initArmy(gameHandler->getRandomGenerator(), &sah.army);
	}
	else
	{
		sah.hid = -1;
	}
	gameHandler->sendAndApply(&sah);
}

void HeroPoolProcessor::onNewWeek(const PlayerColor & color)
{
	clearHeroFromSlot(color, TavernHeroSlot::NATIVE);
	clearHeroFromSlot(color, TavernHeroSlot::RANDOM);
	selectNewHeroForSlot(color, TavernHeroSlot::NATIVE);
	selectNewHeroForSlot(color, TavernHeroSlot::RANDOM);
}

bool HeroPoolProcessor::hireHero(const CGObjectInstance *obj, const HeroTypeID & heroToRecruit, const PlayerColor & player)
{
	const PlayerState * playerState = gameHandler->getPlayerState(player);
	const CGTownInstance * town = gameHandler->getTown(obj->id);

	if (playerState->resources[EGameResID::GOLD] < GameConstants::HERO_GOLD_COST && gameHandler->complain("Not enough gold for buying hero!"))
		return false;

	if (gameHandler->getHeroCount(player, false) >= VLC->settings()->getInteger(EGameSettings::HEROES_PER_PLAYER_ON_MAP_CAP) && gameHandler->complain("Cannot hire hero, too many wandering heroes already!"))
		return false;

	if (gameHandler->getHeroCount(player, true) >= VLC->settings()->getInteger(EGameSettings::HEROES_PER_PLAYER_TOTAL_CAP) && gameHandler->complain("Cannot hire hero, too many heroes garrizoned and wandering already!"))
		return false;

	if (town) //tavern in town
	{
		if (!town->hasBuilt(BuildingID::TAVERN) && gameHandler->complain("No tavern!"))
			return false;

		if (town->visitingHero  && gameHandler->complain("There is visiting hero - no place!"))
			return false;
	}

	if (obj->ID == Obj::TAVERN)
	{
		if (gameHandler->getTile(obj->visitablePos())->visitableObjects.back() != obj && gameHandler->complain("Tavern entry must be unoccupied!"))
			return false;
	}

	auto recruitableHeroes = gameHandler->gameState()->hpool->getHeroesFor(player);

	const CGHeroInstance *recruitedHero = nullptr;;

	for(const auto & hero : recruitableHeroes)
	{
		if (hero->subID == heroToRecruit)
			recruitedHero = hero;
	}

	if (!recruitedHero)
	{
		gameHandler->complain ("Hero is not available for hiring!");
		return false;
	}

	HeroRecruited hr;
	hr.tid = obj->id;
	hr.hid = recruitedHero->subID;
	hr.player = player;
	hr.tile = recruitedHero->convertFromVisitablePos(obj->visitablePos());
	if (gameHandler->getTile(hr.tile)->isWater())
	{
		//Create a new boat for hero
		gameHandler->createObject(obj->visitablePos(), Obj::BOAT, recruitedHero->getBoatType().getNum());

		hr.boatId = gameHandler->getTopObj(hr.tile)->id;
	}
	gameHandler->sendAndApply(&hr);

	if (recruitableHeroes[0] == recruitedHero)
		selectNewHeroForSlot(player, TavernHeroSlot::NATIVE);
	else
		selectNewHeroForSlot(player, TavernHeroSlot::RANDOM);

	gameHandler->giveResource(player, EGameResID::GOLD, -GameConstants::HERO_GOLD_COST);

	if(town)
	{
		gameHandler->visitCastleObjects(town, recruitedHero);
		gameHandler->giveSpells(town, recruitedHero);
	}
	return true;
}
