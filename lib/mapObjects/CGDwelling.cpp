/*
 * CGDwelling.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "CGDwelling.h"
#include "../callback/IGameInfoCallback.h"
#include "../callback/IGameEventCallback.h"
#include "../callback/IGameRandomizer.h"
#include "../serializer/JsonSerializeFormat.h"
#include "../entities/faction/CTownHandler.h"
#include "../mapping/CMap.h"
#include "../mapObjectConstructors/AObjectTypeHandler.h"
#include "../mapObjectConstructors/CObjectClassesHandler.h"
#include "../mapObjectConstructors/DwellingInstanceConstructor.h"
#include "../mapObjects/CGHeroInstance.h"
#include "../mapObjects/CGTownInstance.h"
#include "../networkPacks/StackLocation.h"
#include "../networkPacks/PacksForClient.h"
#include "../networkPacks/PacksForClientBattle.h"
#include "../gameState/CGameState.h"
#include "../CPlayerState.h"
#include "../IGameSettings.h"
#include "../CConfigHandler.h"
#include "../texts/CGeneralTextHandler.h"

#include <vstd/RNG.h>

VCMI_LIB_NAMESPACE_BEGIN

void CGDwellingRandomizationInfo::serializeJson(JsonSerializeFormat & handler)
{
	handler.serializeString("sameAsTown", instanceId);
	handler.serializeIdArray("allowedFactions", allowedFactions);
	handler.serializeInt("minLevel", minLevel, static_cast<uint8_t>(1));
	handler.serializeInt("maxLevel", maxLevel, static_cast<uint8_t>(7));

	if(!handler.saving)
	{
		//todo: safely allow any level > 7
		vstd::abetween<uint8_t>(minLevel, 1, 7);
		vstd::abetween<uint8_t>(maxLevel, minLevel, 7);
	}
}

CGDwelling::CGDwelling(IGameInfoCallback *cb):
	CGDwelling(cb, BonusNodeType::ARMY)
{}

CGDwelling::CGDwelling(IGameInfoCallback *cb, BonusNodeType nodeType):
	CArmedInstance(cb, nodeType, false)
{}

CGDwelling::~CGDwelling() = default;

FactionID CGDwelling::randomizeFaction(IGameRandomizer & gameRandomizer)
{
	if (ID == Obj::RANDOM_DWELLING_FACTION)
		return FactionID(subID.getNum());

	assert(ID == Obj::RANDOM_DWELLING || ID == Obj::RANDOM_DWELLING_LVL);
	assert(randomizationInfo.has_value());
	if (!randomizationInfo)
		return FactionID::CASTLE;

	CGTownInstance * linkedTown = nullptr;

	if (!randomizationInfo->instanceId.empty())
	{
		auto iter = cb->gameState().getMap().instanceNames.find(randomizationInfo->instanceId);

		if(iter == cb->gameState().getMap().instanceNames.end())
			logGlobal->error("Map object not found: %s", randomizationInfo->instanceId);
		linkedTown = dynamic_cast<CGTownInstance *>(iter->second.get());
	}

	if (randomizationInfo->identifier != 0)
	{
		for(auto & townID : cb->gameState().getMap().getAllTowns())
		{
			auto town = cb->gameState().getTown(townID);
			if(town && town->identifier == randomizationInfo->identifier)
			{
				linkedTown = town;
				break;
			}
		}
	}

	if (linkedTown)
	{
		if(linkedTown->ID==Obj::RANDOM_TOWN)
			linkedTown->pickRandomObject(gameRandomizer); //we have to randomize the castle first

		assert(linkedTown->ID == Obj::TOWN);
		if(linkedTown->ID==Obj::TOWN)
			return linkedTown->getFactionID();
	}

	if(!randomizationInfo->allowedFactions.empty())
		return *RandomGeneratorUtil::nextItem(randomizationInfo->allowedFactions, gameRandomizer.getDefault());


	std::vector<FactionID> potentialPicks;

	for (FactionID faction(0); faction < FactionID(LIBRARY->townh->size()); ++faction)
		if (LIBRARY->factions()->getById(faction)->hasTown())
			potentialPicks.push_back(faction);

	assert(!potentialPicks.empty());
	return *RandomGeneratorUtil::nextItem(potentialPicks, gameRandomizer.getDefault());
}

int CGDwelling::randomizeLevel(vstd::RNG & rand)
{
	if (ID == Obj::RANDOM_DWELLING_LVL)
		return subID.getNum();

	assert(ID == Obj::RANDOM_DWELLING || ID == Obj::RANDOM_DWELLING_FACTION);
	assert(randomizationInfo.has_value());

	if (!randomizationInfo)
		return rand.nextInt(1, 7) - 1;

	if(randomizationInfo->minLevel == randomizationInfo->maxLevel)
		return randomizationInfo->minLevel - 1;

	return rand.nextInt(randomizationInfo->minLevel, randomizationInfo->maxLevel) - 1;
}

void CGDwelling::pickRandomObject(IGameRandomizer & gameRandomizer)
{
	if (ID == Obj::RANDOM_DWELLING || ID == Obj::RANDOM_DWELLING_LVL || ID == Obj::RANDOM_DWELLING_FACTION)
	{
		FactionID faction = randomizeFaction(gameRandomizer);
		int level = randomizeLevel(gameRandomizer.getDefault());
		assert(faction != FactionID::NONE && faction != FactionID::NEUTRAL);
		assert(level >= 0 && level <= 6);
		randomizationInfo.reset();

		CreatureID cid = (*LIBRARY->townh)[faction]->town->creatures[level][0];

		//NOTE: this will pick last dwelling with this creature (Mantis #900)
		//check for block map equality is better but more complex solution
		auto testID = [&](const Obj & primaryID) -> MapObjectSubID
		{
			auto dwellingIDs = LIBRARY->objtypeh->knownSubObjects(primaryID);
			for (MapObjectSubID entry : dwellingIDs)
			{
				const auto * handler = dynamic_cast<const DwellingInstanceConstructor *>(LIBRARY->objtypeh->getHandlerFor(primaryID, entry).get());

				if (!handler->isBannedForRandomDwelling() && handler->producesCreature(cid.toCreature()))
					return MapObjectSubID(entry);
			}
			return MapObjectSubID();
		};

		ID = Obj::CREATURE_GENERATOR1;
		subID = testID(Obj::CREATURE_GENERATOR1);

		if (subID == MapObjectSubID())
		{
			ID = Obj::CREATURE_GENERATOR4;
			subID = testID(Obj::CREATURE_GENERATOR4);
		}

		if (subID == MapObjectSubID())
		{
			logGlobal->error("Error: failed to find dwelling for %s of level %d", (*LIBRARY->townh)[faction]->getNameTranslated(), int(level));
			ID = Obj::CREATURE_GENERATOR1;
			subID = *RandomGeneratorUtil::nextItem(LIBRARY->objtypeh->knownSubObjects(Obj::CREATURE_GENERATOR1), gameRandomizer.getDefault());
		}

		setType(ID, subID);
	}
}

void CGDwelling::initObj(IGameRandomizer & gameRandomizer)
{
	switch(ID.toEnum())
	{
	case Obj::CREATURE_GENERATOR1:
	case Obj::CREATURE_GENERATOR4:
	case Obj::WAR_MACHINE_FACTORY:
		{
			getObjectHandler()->configureObject(this, gameRandomizer);
			assert(!creatures.empty());
			assert(!creatures[0].second.empty());
			break;
		}
	case Obj::REFUGEE_CAMP:
		//is handled within newturn func
		break;

	default:
		assert(0);
		break;
	}
}

void CGDwelling::setPropertyDer(ObjProperty what, ObjPropertyID identifier)
{
	switch (what)
	{
		case ObjProperty::AVAILABLE_CREATURE:
			creatures.resize(1);
			creatures[0].second.resize(1);
			creatures[0].second[0] = identifier.as<CreatureID>();
			break;
	}
}

void CGDwelling::onHeroVisit(IGameEventCallback & gameEvents, const CGHeroInstance * h) const
{
	if(ID == Obj::REFUGEE_CAMP)
	{
		ChangeObjectVisitors cow;
		cow.mode = ChangeObjectVisitors::VISITOR_CLEAR;
		cow.object = id;
		gameEvents.sendAndApply(cow);

		cow.mode = ChangeObjectVisitors::VISITOR_ADD_PLAYER;
		cow.hero = h->id;
		gameEvents.sendAndApply(cow);

		if (!creatures[0].first) //Refugee Camp, no available cres
		{
			InfoWindow iw;
			iw.type = EInfoWindowMode::AUTO;
			iw.player = h->tempOwner;
			iw.text.appendLocalString(EMetaText::ADVOB_TXT, 44); //{%s} \n\n The camp is deserted.  Perhaps you should try next week.
			iw.text.replaceName(ID, subID);
			gameEvents.sendAndApply(iw);
			return;
		}
	}

	PlayerRelations relations = cb->getPlayerRelations( h->tempOwner, tempOwner );

	if ( relations == PlayerRelations::ALLIES )
		return;//do not allow recruiting or capturing

	if(relations == PlayerRelations::ENEMIES && stacksCount() > 0) //object is guarded, owned by enemy
	{
		BlockingDialog bd(true,false);
		bd.player = h->tempOwner;
		bd.text.appendLocalString(EMetaText::GENERAL_TXT, 421); //Much to your dismay, the %s is guarded by %s %s. Do you wish to fight the guards?
		bd.text.replaceTextID(getObjectHandler()->getNameTextID());
		if(settings["gameTweaks"]["numericCreaturesQuantities"].Bool())
			bd.text.replaceRawString(CCreature::getQuantityRangeStringForId(Slots().begin()->second->getQuantityID()));
		else
			bd.text.replaceLocalString(EMetaText::ARRAY_TXT, 173 + (int)Slots().begin()->second->getQuantityID()*3);
		bd.text.replaceName(*Slots().begin()->second);
		gameEvents.showBlockingDialog(this, &bd);
		return;
	}

	// TODO this shouldn't be hardcoded
	if(relations == PlayerRelations::ENEMIES && ID != Obj::WAR_MACHINE_FACTORY && ID != Obj::REFUGEE_CAMP)
	{
		gameEvents.setOwner(this, h->tempOwner);
	}

	BlockingDialog bd (true,false);
	bd.player = h->tempOwner;
	if(ID == Obj::CREATURE_GENERATOR1 || ID == Obj::CREATURE_GENERATOR4)
	{
		const size_t count = std::min<size_t>(creatures.size(), 4);
		constexpr std::array dwellingVisitTextID = {
			"core.advevent.35", // 0 creatures, should not happen
			"core.advevent.35",
			"vcmi.adventureMap.dwelling2",
			"vcmi.adventureMap.dwelling3",
			"core.advevent.36"
		};
		bd.text.appendTextID(dwellingVisitTextID[count]);	
		bd.text.replaceTextID(getObjectHandler()->getNameTextID());
		for(const auto & elem : creatures)
			bd.text.replaceNamePlural(elem.second[0]);
	}
	else if(ID == Obj::REFUGEE_CAMP)
	{
		bd.text.appendLocalString(EMetaText::ADVOB_TXT, 35); //{%s} Would you like to recruit %s?
		bd.text.replaceName(ID, subID);
		for(const auto & elem : creatures)
			bd.text.replaceNamePlural(elem.second[0]);
	}
	else if(ID == Obj::WAR_MACHINE_FACTORY)
		bd.text.appendLocalString(EMetaText::ADVOB_TXT, 157); //{War Machine Factory} Would you like to purchase War Machines?
	else
		throw std::runtime_error("Illegal dwelling!");

	if(ID == Obj::REFUGEE_CAMP || (ID == Obj::CREATURE_GENERATOR1 && LIBRARY->creatures()->getById(creatures[0].second[0])->getLevel() != 1))
	{
		bd.flags |= BlockingDialog::SAFE_TO_AUTOACCEPT;
	}

	gameEvents.showBlockingDialog(this, &bd);
}

void CGDwelling::newTurn(IGameEventCallback & gameEvents, IGameRandomizer & gameRandomizer) const
{
	if(cb->getDate(Date::DAY_OF_WEEK) != 1) //not first day of week
		return;

	//town growths and War Machines Factories are handled separately
	if(ID == Obj::TOWN  ||  ID == Obj::WAR_MACHINE_FACTORY)
		return;

	if(ID == Obj::REFUGEE_CAMP) //if it's a refugee camp, we need to pick an available creature
	{
		ChangeObjectVisitors cow;
		cow.mode = ChangeObjectVisitors::VISITOR_CLEAR;
		cow.object = id;
		gameEvents.sendAndApply(cow);

		gameEvents.setObjPropertyID(id, ObjProperty::AVAILABLE_CREATURE, gameRandomizer.rollCreature());
	}

	bool change = false;

	SetAvailableCreatures sac;
	sac.creatures = creatures;
	sac.tid = id;
	for (size_t i = 0; i < creatures.size(); i++)
	{
		if(!creatures[i].second.empty())
		{
			bool creaturesAccumulate = false;

			if (tempOwner.isValidPlayer())
				creaturesAccumulate = cb->getSettings().getBoolean(EGameSettings::DWELLINGS_ACCUMULATE_WHEN_OWNED);
			else
				creaturesAccumulate = cb->getSettings().getBoolean(EGameSettings::DWELLINGS_ACCUMULATE_WHEN_NEUTRAL);

			const CCreature * cre =creatures[i].second[0].toCreature();
			TQuantity amount = cre->getGrowth() * (1 + cre->valOfBonuses(BonusType::CREATURE_GROWTH_PERCENT)/100) + cre->valOfBonuses(BonusType::CREATURE_GROWTH, BonusCustomSubtype::creatureLevel(cre->getLevel()));
			if (creaturesAccumulate && ID != Obj::REFUGEE_CAMP) //camp should not try to accumulate different kinds of creatures
				sac.creatures[i].first += amount;
			else
				sac.creatures[i].first = amount;
			change = true;
		}
	}

	if(change)
		gameEvents.sendAndApply(sac);

	updateGuards(gameEvents);
}

bool CGDwelling::wasVisited (PlayerColor player) const
{
	return cb->getPlayerState(player)->visitedObjects.count(id) != 0;
}

std::vector<Component> CGDwelling::getPopupComponents(PlayerColor player) const
{
	bool visitedByOwner = getOwner() == player;
	bool isDwelling = ID == Obj::CREATURE_GENERATOR1 || ID == Obj::CREATURE_GENERATOR4;
	bool isRefugeeCamp = ID == Obj::REFUGEE_CAMP;
	bool canSeeUnitTypes = isDwelling || (isRefugeeCamp && wasVisited(player));
	bool canSeeUnitAmount = (isDwelling && visitedByOwner) || (isRefugeeCamp && wasVisited(player));

	std::vector<Component> result;

	if (creatures.empty())
		return result;

	if (canSeeUnitTypes)
	{
		for (auto const & creatureLevel : creatures)
		{
			for (auto const & creature : creatureLevel.second)
			{
				if (canSeeUnitAmount)
					result.emplace_back(ComponentType::CREATURE, creature, creatures.front().first);
				else
					result.emplace_back(ComponentType::CREATURE, creature);
			}
		}
	}
	return result;
}

void CGDwelling::updateGuards(IGameEventCallback & gameEvents) const
{
	//TODO: store custom guard config and use it
	//TODO: store boolean flag for guards

	bool guarded = false;
	//default condition - creatures are of level 5 or higher
	for (auto creatureEntry : creatures)
	{
		if (LIBRARY->creatures()->getById(creatureEntry.second.at(0))->getLevel() >= 5 && ID != Obj::REFUGEE_CAMP)
		{
			guarded = true;
			break;
		}
	}

	if (guarded)
	{
		for (auto creatureEntry : creatures)
		{
			const CCreature * crea = creatureEntry.second.at(0).toCreature();
			SlotID slot = getSlotFor(crea->getId());

			if (hasStackAtSlot(slot)) //stack already exists, overwrite it
			{
				ChangeStackCount csc;
				csc.army = this->id;
				csc.slot = slot;
				csc.count = crea->getGrowth() * 3;
				csc.mode = ChangeValueMode::ABSOLUTE;
				gameEvents.sendAndApply(csc);
			}
			else //slot is empty, create whole new stack
			{
				InsertNewStack ns;
				ns.army = this->id;
				ns.slot = slot;
				ns.type = crea->getId();
				ns.count = crea->getGrowth() * 3;
				gameEvents.sendAndApply(ns);
			}
		}
	}
}

void CGDwelling::heroAcceptsCreatures(IGameEventCallback & gameEvents, const CGHeroInstance *h) const
{
	CreatureID crid = creatures[0].second[0];
	auto *crs = crid.toCreature();
	TQuantity count = creatures[0].first;

	if(crs->getLevel() == 1  &&  ID != Obj::REFUGEE_CAMP) //first level - creatures are for free
	{
		if(count) //there are available creatures
		{

			if (cb->getSettings().getBoolean(EGameSettings::DWELLINGS_MERGE_ON_RECRUIT))
			{
				SlotID testSlot = h->getSlotFor(crid);
				if(!testSlot.validSlot()) //no available slot - try merging army of visiting hero
				{
					std::pair<SlotID, SlotID> toMerge;
					if (h->mergeableStacks(toMerge))
					{
						gameEvents.moveStack(StackLocation(h->id, toMerge.first), StackLocation(h->id, toMerge.second), -1); //merge toMerge.first into toMerge.second
						assert(!h->hasStackAtSlot(toMerge.first)); //we have now a new free slot
					}
				}
			}

			SlotID slot = h->getSlotFor(crid);
			if(!slot.validSlot()) //no available slot
			{
				InfoWindow iw;
				iw.type = EInfoWindowMode::AUTO;
				iw.player = h->tempOwner;
				iw.text.appendLocalString(EMetaText::GENERAL_TXT, 425);//The %s would join your hero, but there aren't enough provisions to support them.
				iw.text.replaceNamePlural(crid);
				gameEvents.showInfoDialog(&iw);
			}
			else //give creatures
			{
				SetAvailableCreatures sac;
				sac.tid = id;
				sac.creatures = creatures;
				sac.creatures[0].first = 0;


				InfoWindow iw;
				iw.type = EInfoWindowMode::AUTO;
				iw.player = h->tempOwner;
				iw.text.appendLocalString(EMetaText::GENERAL_TXT, 423); //%d %s join your army.
				iw.text.replaceNumber(count);
				iw.text.replaceNamePlural(crid);

				gameEvents.showInfoDialog(&iw);
				gameEvents.sendAndApply(sac);
				gameEvents.addToSlot(StackLocation(h->id, slot), crs, count);
			}
		}
		else //there no creatures
		{
			InfoWindow iw;
			iw.type = EInfoWindowMode::AUTO;
			iw.text.appendLocalString(EMetaText::GENERAL_TXT, 422); //There are no %s here to recruit.
			iw.text.replaceNamePlural(crid);
			iw.player = h->tempOwner;
			gameEvents.sendAndApply(iw);
		}
	}
	else
	{
		if(ID == Obj::WAR_MACHINE_FACTORY) //pick available War Machines
		{
			//there is 1 war machine available to recruit if hero doesn't have one
			SetAvailableCreatures sac;
			sac.tid = id;
			sac.creatures = creatures;

			for (auto & entry : sac.creatures)
			{
				CreatureID creature = entry.second.at(0);
				ArtifactID warMachine = creature.toCreature()->warMachine;

				if (h->hasArt(warMachine, true, false))
					entry.first = 0;
				else
					entry.first = 1;
			}
			gameEvents.sendAndApply(sac);
		}

		auto windowMode = (ID == Obj::CREATURE_GENERATOR1 || ID == Obj::REFUGEE_CAMP) ? EOpenWindowMode::RECRUITMENT_FIRST : EOpenWindowMode::RECRUITMENT_ALL;
		gameEvents.showObjectWindow(this, windowMode, h, true);
	}
}

void CGDwelling::battleFinished(IGameEventCallback & gameEvents, const CGHeroInstance *hero, const BattleResult &result) const
{
	if (result.winner == BattleSide::ATTACKER)
	{
		onHeroVisit(gameEvents, hero);
	}
}

void CGDwelling::blockingDialogAnswered(IGameEventCallback & gameEvents, const CGHeroInstance *hero, int32_t answer) const
{
	auto relations = cb->getPlayerRelations(getOwner(), hero->getOwner());
	if(stacksCount() > 0  && relations == PlayerRelations::ENEMIES) //guards present
	{
		if(answer)
			gameEvents.startBattle(hero, this);
	}
	else if(answer)
	{
		heroAcceptsCreatures(gameEvents, hero);
	}
}

void CGDwelling::serializeJsonOptions(JsonSerializeFormat & handler)
{
	switch (ID.toEnum())
	{
	case Obj::WAR_MACHINE_FACTORY:
	case Obj::REFUGEE_CAMP:
		//do nothing
		break;
	case Obj::RANDOM_DWELLING:
	case Obj::RANDOM_DWELLING_LVL:
	case Obj::RANDOM_DWELLING_FACTION:
		if (!handler.saving)
			randomizationInfo = CGDwellingRandomizationInfo();
		randomizationInfo->serializeJson(handler);
		[[fallthrough]];
	default:
		serializeJsonOwner(handler);
		break;
	}
}

const IOwnableObject * CGDwelling::asOwnable() const
{
	switch (ID.toEnum())
	{
		case Obj::WAR_MACHINE_FACTORY:
		case Obj::REFUGEE_CAMP:
			return nullptr; // can't be owned
		default:
			return this;
	}
}

ResourceSet CGDwelling::dailyIncome() const
{
	return {};
}

std::vector<CreatureID> CGDwelling::providedCreatures() const
{
	if (ID == Obj::WAR_MACHINE_FACTORY || ID == Obj::REFUGEE_CAMP)
		return {};

	std::vector<CreatureID> result;
	for (const auto & level : creatures)
		result.insert(result.end(), level.second.begin(), level.second.end());

	return result;
}

VCMI_LIB_NAMESPACE_END
