/*
 * CGCreature.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "CGCreature.h"
#include "CGHeroInstance.h"

#include "../texts/CGeneralTextHandler.h"
#include "../CConfigHandler.h"
#include "../IGameSettings.h"
#include "../callback/IGameInfoCallback.h"
#include "../callback/IGameEventCallback.h"
#include "../callback/IGameRandomizer.h"
#include "../gameState/CGameState.h"
#include "../mapObjectConstructors/CObjectClassesHandler.h"
#include "../networkPacks/PacksForClient.h"
#include "../networkPacks/PacksForClientBattle.h"
#include "../networkPacks/StackLocation.h"
#include "../serializer/JsonSerializeFormat.h"
#include "../entities/faction/CTownHandler.h"
#include "../entities/ResourceTypeHandler.h"

#include <vstd/RNG.h>

VCMI_LIB_NAMESPACE_BEGIN

std::string CGCreature::getHoverText(PlayerColor player) const
{
	if(stacks.empty())
	{
		//should not happen...
		logGlobal->error("Invalid stack at tile %s: subID=%d; id=%d", anchorPos().toString(), getCreature(), id.getNum());
		return "INVALID_STACK";
	}

	MetaString ms;
	CCreature::CreatureQuantityId monsterQuantityId = stacks.begin()->second->getQuantityID();
	int quantityTextIndex = 172 + 3 * (int)monsterQuantityId;
	if(settings["gameTweaks"]["numericCreaturesQuantities"].Bool())
		ms.appendRawString(CCreature::getQuantityRangeStringForId(monsterQuantityId));
	else
		ms.appendLocalString(EMetaText::ARRAY_TXT, quantityTextIndex);
	ms.appendRawString(" ");
	ms.appendNamePlural(getCreatureID());

	return ms.toString();
}

std::string CGCreature::getHoverText(const CGHeroInstance * hero) const
{
	if(hero->hasVisions(this, BonusCustomSubtype::visionsMonsters))
	{
		MetaString ms;
		ms.appendNumber(stacks.begin()->second->getCount());
		ms.appendRawString(" ");
		ms.appendName(getCreatureID(), stacks.begin()->second->getCount());
		return ms.toString();
	}
	else
	{
		return getHoverText(hero->tempOwner);
	}
}

std::string CGCreature::getMonsterLevelText() const
{
	std::string monsterLevel = LIBRARY->generaltexth->translate("vcmi.adventureMap.monsterLevel");
	bool isRanged = getCreature()->getBonusBearer()->hasBonusOfType(BonusType::SHOOTER);
	std::string attackTypeKey = isRanged ? "vcmi.adventureMap.monsterRangedType" : "vcmi.adventureMap.monsterMeleeType";
	std::string attackType = LIBRARY->generaltexth->translate(attackTypeKey);
	boost::replace_first(monsterLevel, "%TOWN", getCreature()->getFactionID().toEntity(LIBRARY)->getNameTranslated());
	boost::replace_first(monsterLevel, "%LEVEL", std::to_string(getCreature()->getLevel()));
	boost::replace_first(monsterLevel, "%ATTACK_TYPE", attackType);
	return monsterLevel;
}

std::string CGCreature::getPopupText(const CGHeroInstance * hero) const
{
	std::string hoverName;
	if(hero->hasVisions(this, BonusCustomSubtype::visionsMonsters))
	{
		MetaString ms;
		ms.appendRawString(getHoverText(hero));
		ms.appendRawString("\n\n");

		int decision = takenAction(hero, true);

		switch (decision)
		{
		case FIGHT:
			ms.appendLocalString(EMetaText::GENERAL_TXT,246);
			break;
		case FLEE:
			ms.appendLocalString(EMetaText::GENERAL_TXT,245);
			break;
		case JOIN_FOR_FREE:
			ms.appendLocalString(EMetaText::GENERAL_TXT,243);
			break;
		default: //decision = cost in gold
			ms.appendLocalString(EMetaText::GENERAL_TXT,244);
			ms.replaceNumber(decision);
			break;
		}
		hoverName = ms.toString();
	}
	else
	{
		hoverName = getHoverText(hero->tempOwner);
	}

	if (settings["general"]["enableUiEnhancements"].Bool())
	{
		hoverName += getMonsterLevelText();
		hoverName += LIBRARY->generaltexth->translate("vcmi.adventureMap.monsterThreat.title");

		int choice;
		uint64_t armyStrength = getArmyStrength();
		uint64_t heroStrength = hero->getTotalStrength();
		double ratio = static_cast<double>(armyStrength) / heroStrength;
		if (ratio < 0.1)  choice = 0;
		else if (ratio < 0.25) choice = 1;
		else if (ratio < 0.6)  choice = 2;
		else if (ratio < 0.9)  choice = 3;
		else if (ratio < 1.1)  choice = 4;
		else if (ratio < 1.3)  choice = 5;
		else if (ratio < 1.8)  choice = 6;
		else if (ratio < 2.5)  choice = 7;
		else if (ratio < 4)    choice = 8;
		else if (ratio < 8)    choice = 9;
		else if (ratio < 20)   choice = 10;
		else                   choice = 11;

		hoverName += LIBRARY->generaltexth->translate("vcmi.adventureMap.monsterThreat.levels." + std::to_string(choice));
	}
	return hoverName;
}

std::string CGCreature::getPopupText(PlayerColor player) const
{
	std::string hoverName = getHoverText(player);
	if (settings["general"]["enableUiEnhancements"].Bool())
		hoverName += getMonsterLevelText();
	return hoverName;
}

std::vector<Component> CGCreature::getPopupComponents(PlayerColor player) const
{
	return {
		Component(ComponentType::CREATURE, getCreatureID())
	};
}

void CGCreature::onHeroVisit(IGameEventCallback & gameEvents, const CGHeroInstance * h) const
{
	//show message
	if(!message.empty())
	{
		InfoWindow iw;
		iw.player = h->tempOwner;
		iw.text = message;
		iw.type = EInfoWindowMode::MODAL;
		gameEvents.showInfoDialog(&iw);
	}
	
	int action = takenAction(h);
	switch( action ) //decide what we do...
	{
	case FIGHT:
		fight(gameEvents, h);
		break;
	case FLEE:
		{
			flee(gameEvents, h);
			break;
		}
	case JOIN_FOR_FREE: //join for free
		{
			BlockingDialog ynd(true,false);
			ynd.player = h->tempOwner;
			ynd.text.appendLocalString(EMetaText::ADVOB_TXT, 86);
			ynd.text.replaceName(getCreatureID(), getJoiningAmount());
			gameEvents.showBlockingDialog(this, &ynd);
			break;
		}
	default: //join for gold
		{
			assert(action > 0);

			//ask if player agrees to pay gold
			BlockingDialog ynd(true,false);
			ynd.player = h->tempOwner;
			ynd.components.emplace_back(ComponentType::RESOURCE, GameResID(GameResID::GOLD), action);
			std::string tmp = LIBRARY->generaltexth->advobtxt[90];
			boost::algorithm::replace_first(tmp, "%d", std::to_string(getJoiningAmount()));
			boost::algorithm::replace_first(tmp, "%d", std::to_string(action));
			boost::algorithm::replace_first(tmp,"%s",getCreature()->getNamePluralTranslated());
			ynd.text.appendRawString(tmp);
			gameEvents.showBlockingDialog(this, &ynd);
			break;
		}
	}
}

CreatureID CGCreature::getCreatureID() const
{
	return CreatureID(getObjTypeIndex().getNum());
}

const CCreature * CGCreature::getCreature() const
{
	return getCreatureID().toCreature();
}

TQuantity CGCreature::getJoiningAmount() const
{
	return std::max(static_cast<int64_t>(1), getStackCount(SlotID(0)) * cb->getSettings().getInteger(EGameSettings::CREATURES_JOINING_PERCENTAGE) / 100);
}

void CGCreature::pickRandomObject(IGameRandomizer & gameRandomizer)
{
	switch(ID.toEnum())
	{
		case MapObjectID::RANDOM_MONSTER:
			subID = gameRandomizer.rollCreature();
			break;
		case MapObjectID::RANDOM_MONSTER_L1:
			subID = gameRandomizer.rollCreature(1);
			break;
		case MapObjectID::RANDOM_MONSTER_L2:
			subID = gameRandomizer.rollCreature(2);
			break;
		case MapObjectID::RANDOM_MONSTER_L3:
			subID = gameRandomizer.rollCreature(3);
			break;
		case MapObjectID::RANDOM_MONSTER_L4:
			subID = gameRandomizer.rollCreature(4);
			break;
		case MapObjectID::RANDOM_MONSTER_L5:
			subID = gameRandomizer.rollCreature(5);
			break;
		case MapObjectID::RANDOM_MONSTER_L6:
			subID = gameRandomizer.rollCreature(6);
			break;
		case MapObjectID::RANDOM_MONSTER_L7:
			subID = gameRandomizer.rollCreature(7);
			break;
	}

	try {
		// sanity check
		LIBRARY->objtypeh->getHandlerFor(MapObjectID::MONSTER, subID);
	}
	catch (const std::out_of_range & )
	{
		// Try to generate some debug information if sanity check failed
		CreatureID creatureID(subID.getNum());
		throw std::out_of_range("Failed to find handler for creature " + std::to_string(creatureID.getNum()) + ", identifier:" + creatureID.toEntity(LIBRARY)->getJsonKey());
	}

	ID = MapObjectID::MONSTER;
	setType(ID, subID);
}

void CGCreature::initObj(IGameRandomizer & gameRandomizer)
{
	blockVisit = true;
	switch(character)
	{
	case 0:
		character = -4;
		break;
	case 1:
		character = gameRandomizer.getDefault().nextInt(1, 7);
		break;
	case 2:
		character = gameRandomizer.getDefault().nextInt(1, 10);
		break;
	case 3:
		character = gameRandomizer.getDefault().nextInt(4, 10);
		break;
	case 4:
		character = 10;
		break;
	}

	stacks[SlotID(0)]->setType(getCreature());
	const Creature * c = getCreature();
	if(stacks[SlotID(0)]->getCount() == 0)
	{
		stacks[SlotID(0)]->setCount(gameRandomizer.getDefault().nextInt(c->getAdvMapAmountMin(), c->getAdvMapAmountMax()));

		if(stacks[SlotID(0)]->getCount() == 0) //armies with 0 creatures are illegal
		{
			logGlobal->warn("Stack cannot have 0 creatures. Check properties of %s", c->getJsonKey());
			stacks[SlotID(0)]->setCount(1);
		}
	}

	temppower = stacks[SlotID(0)]->getCount() * static_cast<int64_t>(1000);
	refusedJoining = false;
}

void CGCreature::newTurn(IGameEventCallback & gameEvents, IGameRandomizer & gameRandomizer) const
{//Works only for stacks of single type of size up to 2 millions
	if (!notGrowingTeam)
	{
		if (stacks.begin()->second->getCount() < cb->getSettings().getInteger(EGameSettings::CREATURES_WEEKLY_GROWTH_CAP) && cb->getDate(Date::DAY_OF_WEEK) == 1 && cb->getDate(Date::DAY) > 1)
		{
			ui32 power = static_cast<ui32>(temppower * (100 + cb->getSettings().getInteger(EGameSettings::CREATURES_WEEKLY_GROWTH_PERCENT)) / 100);
			gameEvents.setObjPropertyValue(id, ObjProperty::MONSTER_COUNT, std::min<uint32_t>(power / 1000, cb->getSettings().getInteger(EGameSettings::CREATURES_WEEKLY_GROWTH_CAP))); //set new amount
			gameEvents.setObjPropertyValue(id, ObjProperty::MONSTER_POWER, power); //increase temppower
		}
	}
	if (cb->getSettings().getBoolean(EGameSettings::MODULE_STACK_EXPERIENCE))
		gameEvents.setObjPropertyValue(id, ObjProperty::MONSTER_EXP, cb->getSettings().getInteger(EGameSettings::CREATURES_DAILY_STACK_EXPERIENCE)); //for testing purpose
}
void CGCreature::setPropertyDer(ObjProperty what, ObjPropertyID identifier)
{
	switch (what)
	{
		case ObjProperty::MONSTER_COUNT:
			stacks[SlotID(0)]->setCount(identifier.getNum());
			break;
		case ObjProperty::MONSTER_POWER:
			temppower = identifier.getNum();
			break;
		case ObjProperty::MONSTER_EXP:
			giveAverageStackExperience(identifier.getNum());
			break;
		case ObjProperty::MONSTER_REFUSED_JOIN:
			refusedJoining = identifier.getNum();
			break;
	}
}

int CGCreature::takenAction(const CGHeroInstance *h, bool allowJoin) const
{
	//calculate relative strength of hero and creatures armies
	double relStrength = static_cast<double>(h->getValueForDiplomacy()) / getArmyStrength();

	int powerFactor;
	if(relStrength >= 7)
		powerFactor = 11;

	else if(relStrength >= 1)
		powerFactor = static_cast<int>(2 * (relStrength - 1));

	else if(relStrength >= 0.5)
		powerFactor = -1;

	else if(relStrength >= 0.333)
		powerFactor = -2;
	else
		powerFactor = -3;

	int count = 0; //how many creatures of similar kind has hero
	int totalCount = 0;

	for(const auto & elem : h->Slots())
	{
		bool isOurUpgrade = vstd::contains(getCreature()->upgrades, elem.second->getCreatureID());
		bool isOurDowngrade = vstd::contains(elem.second->getCreature()->upgrades, getCreatureID());

		if(isOurUpgrade || isOurDowngrade)
			count += elem.second->getCount();
		totalCount += elem.second->getCount();
	}

	int sympathy = 0; // 0 if hero have no similar creatures
	if(count)
		sympathy++; // 1 - if hero have at least 1 similar creature
	if(count*2 > totalCount)
		sympathy++; // 2 - hero have similar creatures more that 50%

	int diplomacy = h->valOfBonuses(BonusType::WANDERING_CREATURES_JOIN_BONUS);
	int charisma = powerFactor + diplomacy + sympathy;

	if(charisma < character)
		return FIGHT;

	if (allowJoin && cb->getSettings().getInteger(EGameSettings::CREATURES_JOINING_PERCENTAGE) > 0)
	{
		if((cb->getSettings().getBoolean(EGameSettings::CREATURES_ALLOW_JOINING_FOR_FREE) || character == Character::COMPLIANT) && diplomacy + sympathy + 1 >= character)
			return JOIN_FOR_FREE;

		if(diplomacy * 2 + sympathy + 1 >= character)
		{
			int32_t recruitCost = getCreature()->getRecruitCost(EGameResID::GOLD);
			int32_t stackCount = getStackCount(SlotID(0));
			return recruitCost * stackCount; //join for gold
		}
	}

	//we are still here - creatures have not joined hero, flee or fight

	if (charisma > character && !neverFlees)
		return FLEE;
	else
		return FIGHT;
}

void CGCreature::fleeDecision(IGameEventCallback & gameEvents, const CGHeroInstance *h, ui32 pursue) const
{
	if(refusedJoining)
		gameEvents.setObjPropertyValue(id, ObjProperty::MONSTER_REFUSED_JOIN, false);

	if(pursue)
	{
		fight(gameEvents, h);
	}
	else
	{
		gameEvents.removeObject(this, h->getOwner());
	}
}

void CGCreature::joinDecision(IGameEventCallback & gameEvents, const CGHeroInstance *h, int cost, ui32 accept) const
{
	if(!accept)
	{
		if(takenAction(h,false) == FLEE)
		{
			gameEvents.setObjPropertyValue(id, ObjProperty::MONSTER_REFUSED_JOIN, true);
			flee(gameEvents, h);
		}
		else //they fight
		{
			h->showInfoDialog(gameEvents, 87, 0, EInfoWindowMode::MODAL);//Insulted by your refusal of their offer, the monsters attack!
			fight(gameEvents, h);
		}
	}
	else //accepted
	{
		if (cb->getResource(h->tempOwner, EGameResID::GOLD) < cost) //player don't have enough gold!
		{
			InfoWindow iw;
			iw.player = h->tempOwner;
			iw.text.appendLocalString(EMetaText::GENERAL_TXT,29);  //You don't have enough gold
			gameEvents.showInfoDialog(&iw);

			//act as if player refused
			joinDecision(gameEvents, h, cost, false);
			return;
		}

		//take gold
		if(cost)
			gameEvents.giveResource(h->tempOwner,EGameResID::GOLD,-cost);

		giveReward(gameEvents, h);

		for(auto & stack : this->stacks)
			stack.second->setCount(getJoiningAmount());

		gameEvents.tryJoiningArmy(this, h, true, true);
	}
}

void CGCreature::fight(IGameEventCallback & gameEvents, const CGHeroInstance * h) const
{
	//split stacks
	int stacksCount = getNumberOfStacks(h);
	//source: http://heroescommunity.com/viewthread.php3?TID=27539&PID=1266335#focus

	int amount = getStackCount(SlotID(0));
	int m = amount / stacksCount;
	int b = stacksCount * (m + 1) - amount;
	int a = stacksCount - b;

	SlotID sourceSlot = stacks.begin()->first;
	for (int slotID = 1; slotID < a; ++slotID)
	{
		int stackSize = m + 1;
		gameEvents.moveStack(StackLocation(id, sourceSlot), StackLocation(id, SlotID(slotID)), stackSize);
	}
	for (int slotID = a; slotID < stacksCount; ++slotID)
	{
		int stackSize = m;
		if (slotID) //don't do this when a = 0 -> stack is single
			gameEvents.moveStack(StackLocation(id, sourceSlot), StackLocation(id, SlotID(slotID)), stackSize);
	}
	if (stacksCount > 1)
	{
		if (containsUpgradedStack()) //upgrade
		{
			SlotID slotID = SlotID(static_cast<si32>(std::floor(static_cast<float>(stacks.size()) / 2.0f)));
			const auto & upgrades = getStack(slotID).getCreature()->upgrades;
			if(!upgrades.empty())
			{
				auto it = RandomGeneratorUtil::nextItem(upgrades, gameEvents.getRandomGenerator());
				gameEvents.changeStackType(StackLocation(id, slotID), it->toCreature());
			}
		}
	}

	gameEvents.startBattle(h, this);

}

void CGCreature::flee(IGameEventCallback & gameEvents, const CGHeroInstance * h) const
{
	BlockingDialog ynd(true,false);
	ynd.player = h->tempOwner;
	ynd.text.appendLocalString(EMetaText::ADVOB_TXT,91);
	ynd.text.replaceName(getCreatureID(), getStackCount(SlotID(0)));
	gameEvents.showBlockingDialog(this, &ynd);
}

void CGCreature::battleFinished(IGameEventCallback & gameEvents, const CGHeroInstance *hero, const BattleResult &result) const
{
	if(result.winner == BattleSide::ATTACKER)
	{
		giveReward(gameEvents, hero);
		gameEvents.removeObject(this, hero->getOwner());
	}
	else if(result.winner == BattleSide::NONE) // draw
	{
		// guarded reward is lost forever on draw
		gameEvents.removeObject(this, result.attacker);
	}
	else
	{
		//merge stacks into one
		TSlots::const_iterator i;
		const CCreature * cre = getCreature();
		for(i = stacks.begin(); i != stacks.end(); i++)
		{
			if(cre->isMyDirectUpgrade(i->second->getCreature()))
			{
				gameEvents.changeStackType(StackLocation(id, i->first), cre); //un-upgrade creatures
			}
		}

		//first stack has to be at slot 0 -> if original one got killed, move there first remaining stack
		if(!hasStackAtSlot(SlotID(0)))
			gameEvents.moveStack(StackLocation(id, stacks.begin()->first), StackLocation(id, SlotID(0)), stacks.begin()->second->getCount());

		while(stacks.size() > 1) //hopefully that's enough
		{
			// TODO it's either overcomplicated (if we assume there'll be only one stack) or buggy (if we allow multiple stacks... but that'll also cause troubles elsewhere)
			i = stacks.end();
			i--;
			SlotID slot = getSlotFor(i->second->getCreature());
			if(slot == i->first) //no reason to move stack to its own slot
				break;
			else
				gameEvents.moveStack(StackLocation(id, i->first), StackLocation(id, slot), i->second->getCount());
		}

		gameEvents.setObjPropertyValue(id, ObjProperty::MONSTER_POWER, stacks.begin()->second->getCount() * 1000); //remember casualties
	}
}

void CGCreature::blockingDialogAnswered(IGameEventCallback & gameEvents, const CGHeroInstance *hero, int32_t answer) const
{
	auto action = takenAction(hero);
	if(!refusedJoining && action >= JOIN_FOR_FREE) //higher means price
		joinDecision(gameEvents, hero, action, answer);
	else if(action != FIGHT)
		fleeDecision(gameEvents, hero, answer);
	else
		assert(0);
}

bool CGCreature::containsUpgradedStack() const
{
	//source http://heroescommunity.com/viewthread.php3?TID=27539&PID=830557#focus

	float a = 2992.911117f;
	float b = 14174.264968f;
	float c = 5325.181015f;
	float d = 32788.727920f;

	int val = static_cast<int>(std::floor(a * visitablePos().x + b * visitablePos().y + c * visitablePos().z + d));
	return ((val % 32768) % 100) < 50;
}

int CGCreature::getNumberOfStacks(const CGHeroInstance * hero) const
{
	if(stacksCount > 0)
		return stacksCount;

	int split = 0;
	if (hero->hasBonusOfType(BonusType::FORCE_NEUTRAL_ENCOUNTER_STACK_COUNT))
		split = getNumberOfStacksFromBonus(hero);

	if(split == 0)
	 split = getDefaultNumberOfStacks(hero);

	vstd::amin(split, getStack(SlotID(0)).getCount()); //can't divide into more stacks than creatures total
	vstd::amin(split, 7);   
	vstd::amax(split, 1);
	return split;
}

int CGCreature::getNumberOfStacksFromBonus(const CGHeroInstance * hero) const
{
	auto bonus = hero->getBonus(Selector::type()(BonusType::FORCE_NEUTRAL_ENCOUNTER_STACK_COUNT));
	if(bonus->val > 0)
		return bonus->val;

	auto addInfo = bonus->additionalInfo;
	if(addInfo.empty())
		return 0;
	const size_t maxEntries = std::min<size_t>(addInfo.size(), 7);
	int total = 0;
	for(size_t i = 0; i < maxEntries; i++)
		total += std::max<int>(0, addInfo[i]);

	if(total <= 0)
		return 0;

	ui32 R2 = hashByPosition();
	int R4 = R2 % total + 1;

	int acc = 0;
	for(size_t i = 0; i < maxEntries; i++)
	{
		acc += std::max<int>(0, addInfo[i]);
		if(R4 <= acc)
			return static_cast<int>(i + 1);
	}
	return 0;
}

int CGCreature::getDefaultNumberOfStacks(const CGHeroInstance *hero) const
{
	//source http://heroescommunity.com/viewthread.php3?TID=27539&PID=1266094#focus

	double strengthRatio = static_cast<double>(hero->getArmyStrength()) / getArmyStrength();
	int split = 1;

	if (strengthRatio < 0.5f)
		split = 7;
	else if (strengthRatio < 0.67f)
		split = 6;
	else if (strengthRatio < 1)
		split = 5;
	else if (strengthRatio < 1.5f)
		split = 4;
	else if (strengthRatio < 2)
		split = 3;
	else
		split = 2;

	ui32 R2 = hashByPosition();

	int R4 = R2 % 100 + 1;

	if(stacksCount == -3)
		;
	else if (stacksCount == -2 || R4 <= 20)
		split -= 1;
	else if(stacksCount == 0 || R4 >= 80)
		split += 1;

	vstd::amin(split, getStack(SlotID(0)).getCount()); //can't divide into more stacks than creatures total
	vstd::amin(split, 7); //can't have more than 7 stacks

	return split;
}

ui32 CGCreature::hashByPosition() const
{
	ui32 a = 1550811371u;
	ui32 b = 3359066809u;
	ui32 c = 1943276003u;
	ui32 d = 3174620878u;

	ui32 R1 = a * static_cast<ui32>(visitablePos().x) + b * static_cast<ui32>(visitablePos().y) + c * static_cast<ui32>(visitablePos().z) + d;
	ui32 R2 = (R1 >> 16) & 0x7fff;

	return R2;
}

void CGCreature::giveReward(IGameEventCallback & gameEvents, const CGHeroInstance * h) const
{
	InfoWindow iw;
	iw.player = h->tempOwner;

	if(!resources.empty())
	{
		gameEvents.giveResources(h->tempOwner, resources);
		for(const auto & res : LIBRARY->resourceTypeHandler->getAllObjects())
		{
			if(resources[res] > 0)
				iw.components.emplace_back(ComponentType::RESOURCE, res, resources[res]);
		}
	}

	if(gainedArtifact != ArtifactID::NONE)
	{
		gameEvents.giveHeroNewArtifact(h, gainedArtifact, ArtifactPosition::FIRST_AVAILABLE);
		iw.components.emplace_back(ComponentType::ARTIFACT, gainedArtifact);
	}

	if(!iw.components.empty())
	{
		iw.type = EInfoWindowMode::AUTO;
		iw.text.appendLocalString(EMetaText::ADVOB_TXT, 183); // % has found treasure
		iw.text.replaceRawString(h->getNameTranslated());
		gameEvents.showInfoDialog(&iw);
	}
}

static const std::vector<std::string> CHARACTER_JSON  =
{
	"compliant", "friendly", "aggressive", "hostile", "savage"
};

void CGCreature::serializeJsonOptions(JsonSerializeFormat & handler)
{
	handler.serializeEnum("character", character, CHARACTER_JSON);

	if(handler.saving)
	{
		if(hasStackAtSlot(SlotID(0)))
		{
			si32 amount = getStack(SlotID(0)).getCount();
			handler.serializeInt("amount", amount, 0);
		}
	}
	else
	{
		si32 amount = 0;
		handler.serializeInt("amount", amount);
		auto hlp = std::make_unique<CStackInstance>(cb);
		hlp->setCount(amount);
		//type will be set during initialization
		putStack(SlotID(0), std::move(hlp));
	}

	resources.serializeJson(handler, "rewardResources");

	handler.serializeId("rewardArtifact", gainedArtifact, ArtifactID(ArtifactID::NONE));

	handler.serializeBool("noGrowing", notGrowingTeam);
	handler.serializeBool("neverFlees", neverFlees);
	handler.serializeStruct("rewardMessage", message);
}

VCMI_LIB_NAMESPACE_END
