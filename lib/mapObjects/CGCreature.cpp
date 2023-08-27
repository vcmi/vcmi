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

#include "../NetPacks.h"
#include "../CGeneralTextHandler.h"
#include "../CConfigHandler.h"
#include "../GameSettings.h"
#include "../IGameCallback.h"
#include "../serializer/JsonSerializeFormat.h"

VCMI_LIB_NAMESPACE_BEGIN

std::string CGCreature::getHoverText(PlayerColor player) const
{
	if(stacks.empty())
	{
		//should not happen...
		logGlobal->error("Invalid stack at tile %s: subID=%d; id=%d", pos.toString(), subID, id.getNum());
		return "INVALID_STACK";
	}

	std::string hoverName;
	MetaString ms;
	CCreature::CreatureQuantityId monsterQuantityId = stacks.begin()->second->getQuantityID();
	int quantityTextIndex = 172 + 3 * (int)monsterQuantityId;
	if(settings["gameTweaks"]["numericCreaturesQuantities"].Bool())
		ms.appendRawString(CCreature::getQuantityRangeStringForId(monsterQuantityId));
	else
		ms.appendLocalString(EMetaText::ARRAY_TXT, quantityTextIndex);
	ms.appendRawString(" ");
	ms.appendLocalString(EMetaText::CRE_PL_NAMES,subID);
	hoverName = ms.toString();
	return hoverName;
}

std::string CGCreature::getHoverText(const CGHeroInstance * hero) const
{
	std::string hoverName;
	if(hero->hasVisions(this, 0))
	{
		MetaString ms;
		ms.appendNumber(stacks.begin()->second->count);
		ms.appendRawString(" ");
		ms.appendLocalString(EMetaText::CRE_PL_NAMES,subID);

		ms.appendRawString("\n");

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
			ms.appendRawString(boost::str(boost::format(VLC->generaltexth->allTexts[244]) % decision));
			break;
		}

		hoverName = ms.toString();
	}
	else
	{
		hoverName = getHoverText(hero->tempOwner);
	}

	hoverName += VLC->generaltexth->translate("vcmi.adventureMap.monsterThreat.title");

	int choice;
	double ratio = (static_cast<double>(getArmyStrength()) / hero->getTotalStrength());
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

	hoverName += VLC->generaltexth->translate("vcmi.adventureMap.monsterThreat.levels." + std::to_string(choice));
	return hoverName;
}

void CGCreature::onHeroVisit( const CGHeroInstance * h ) const
{
	int action = takenAction(h);
	switch( action ) //decide what we do...
	{
	case FIGHT:
		fight(h);
		break;
	case FLEE:
		{
			flee(h);
			break;
		}
	case JOIN_FOR_FREE: //join for free
		{
			BlockingDialog ynd(true,false);
			ynd.player = h->tempOwner;
			ynd.text.appendLocalString(EMetaText::ADVOB_TXT, 86);
			ynd.text.replaceLocalString(EMetaText::CRE_PL_NAMES, subID);
			cb->showBlockingDialog(&ynd);
			break;
		}
	default: //join for gold
		{
			assert(action > 0);

			//ask if player agrees to pay gold
			BlockingDialog ynd(true,false);
			ynd.player = h->tempOwner;
			std::string tmp = VLC->generaltexth->advobtxt[90];
			boost::algorithm::replace_first(tmp, "%d", std::to_string(getStackCount(SlotID(0))));
			boost::algorithm::replace_first(tmp, "%d", std::to_string(action));
			boost::algorithm::replace_first(tmp,"%s",VLC->creh->objects[subID]->getNamePluralTranslated());
			ynd.text.appendRawString(tmp);
			cb->showBlockingDialog(&ynd);
			break;
		}
	}

}

void CGCreature::initObj(CRandomGenerator & rand)
{
	blockVisit = true;
	switch(character)
	{
	case 0:
		character = -4;
		break;
	case 1:
		character = rand.nextInt(1, 7);
		break;
	case 2:
		character = rand.nextInt(1, 10);
		break;
	case 3:
		character = rand.nextInt(4, 10);
		break;
	case 4:
		character = 10;
		break;
	}

	stacks[SlotID(0)]->setType(CreatureID(subID));
	TQuantity &amount = stacks[SlotID(0)]->count;
	CCreature &c = *VLC->creh->objects[subID];
	if(amount == 0)
	{
		amount = rand.nextInt(c.ammMin, c.ammMax);

		if(amount == 0) //armies with 0 creatures are illegal
		{
			logGlobal->warn("Stack %s cannot have 0 creatures. Check properties of %s", nodeName(), c.nodeName());
			amount = 1;
		}
	}

	temppower = stacks[SlotID(0)]->count * static_cast<ui64>(1000);
	refusedJoining = false;
}

void CGCreature::newTurn(CRandomGenerator & rand) const
{//Works only for stacks of single type of size up to 2 millions
	if (!notGrowingTeam)
	{
		if (stacks.begin()->second->count < VLC->settings()->getInteger(EGameSettings::CREATURES_WEEKLY_GROWTH_CAP) && cb->getDate(Date::DAY_OF_WEEK) == 1 && cb->getDate(Date::DAY) > 1)
		{
			ui32 power = static_cast<ui32>(temppower * (100 + VLC->settings()->getInteger(EGameSettings::CREATURES_WEEKLY_GROWTH_PERCENT)) / 100);
			cb->setObjProperty(id, ObjProperty::MONSTER_COUNT, std::min<uint32_t>(power / 1000, VLC->settings()->getInteger(EGameSettings::CREATURES_WEEKLY_GROWTH_CAP))); //set new amount
			cb->setObjProperty(id, ObjProperty::MONSTER_POWER, power); //increase temppower
		}
	}
	if (VLC->settings()->getBoolean(EGameSettings::MODULE_STACK_EXPERIENCE))
		cb->setObjProperty(id, ObjProperty::MONSTER_EXP, VLC->settings()->getInteger(EGameSettings::CREATURES_DAILY_STACK_EXPERIENCE)); //for testing purpose
}
void CGCreature::setPropertyDer(ui8 what, ui32 val)
{
	switch (what)
	{
		case ObjProperty::MONSTER_COUNT:
			stacks[SlotID(0)]->count = val;
			break;
		case ObjProperty::MONSTER_POWER:
			temppower = val;
			break;
		case ObjProperty::MONSTER_EXP:
			giveStackExp(val);
			break;
		case ObjProperty::MONSTER_RESTORE_TYPE:
			formation.basicType = val;
			break;
		case ObjProperty::MONSTER_REFUSED_JOIN:
			refusedJoining = val;
			break;
	}
}

int CGCreature::takenAction(const CGHeroInstance *h, bool allowJoin) const
{
	//calculate relative strength of hero and creatures armies
	double relStrength = static_cast<double>(h->getTotalStrength()) / getArmyStrength();

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

	std::set<CreatureID> myKindCres; //what creatures are the same kind as we
	const CCreature * myCreature = VLC->creh->objects[subID];
	myKindCres.insert(myCreature->getId()); //we
	myKindCres.insert(myCreature->upgrades.begin(), myCreature->upgrades.end()); //our upgrades

	for(ConstTransitivePtr<CCreature> &crea : VLC->creh->objects)
	{
		if(vstd::contains(crea->upgrades, myCreature->getId())) //it's our base creatures
			myKindCres.insert(crea->getId());
	}

	int count = 0; //how many creatures of similar kind has hero
	int totalCount = 0;

	for(const auto & elem : h->Slots())
	{
		if(vstd::contains(myKindCres,elem.second->type->getId()))
			count += elem.second->count;
		totalCount += elem.second->count;
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

	if (allowJoin)
	{
		if(diplomacy + sympathy + 1 >= character)
			return JOIN_FOR_FREE;

		else if(diplomacy * 2  +  sympathy  +  1 >= character)
			return VLC->creatures()->getByIndex(subID)->getRecruitCost(EGameResID::GOLD) * getStackCount(SlotID(0)); //join for gold
	}

	//we are still here - creatures have not joined hero, flee or fight

	if (charisma > character && !neverFlees)
		return FLEE;
	else
		return FIGHT;
}

void CGCreature::fleeDecision(const CGHeroInstance *h, ui32 pursue) const
{
	if(refusedJoining)
		cb->setObjProperty(id, ObjProperty::MONSTER_REFUSED_JOIN, false);

	if(pursue)
	{
		fight(h);
	}
	else
	{
		cb->removeObject(this);
	}
}

void CGCreature::joinDecision(const CGHeroInstance *h, int cost, ui32 accept) const
{
	if(!accept)
	{
		if(takenAction(h,false) == FLEE)
		{
			cb->setObjProperty(id, ObjProperty::MONSTER_REFUSED_JOIN, true);
			flee(h);
		}
		else //they fight
		{
			h->showInfoDialog(87, 0, EInfoWindowMode::MODAL);//Insulted by your refusal of their offer, the monsters attack!
			fight(h);
		}
	}
	else //accepted
	{
		if (cb->getResource(h->tempOwner, EGameResID::GOLD) < cost) //player don't have enough gold!
		{
			InfoWindow iw;
			iw.player = h->tempOwner;
			iw.text.appendLocalString(EMetaText::GENERAL_TXT,29);  //You don't have enough gold
			cb->showInfoDialog(&iw);

			//act as if player refused
			joinDecision(h,cost,false);
			return;
		}

		//take gold
		if(cost)
			cb->giveResource(h->tempOwner,EGameResID::GOLD,-cost);

		giveReward(h);
		cb->tryJoiningArmy(this, h, true, true);
	}
}

void CGCreature::fight( const CGHeroInstance *h ) const
{
	//split stacks
	//TODO: multiple creature types in a stack?
	int basicType = stacks.begin()->second->type->getId();
	cb->setObjProperty(id, ObjProperty::MONSTER_RESTORE_TYPE, basicType); //store info about creature stack

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
		cb->moveStack(StackLocation(this, sourceSlot), StackLocation(this, SlotID(slotID)), stackSize);
	}
	for (int slotID = a; slotID < stacksCount; ++slotID)
	{
		int stackSize = m;
		if (slotID) //don't do this when a = 0 -> stack is single
			cb->moveStack(StackLocation(this, sourceSlot), StackLocation(this, SlotID(slotID)), stackSize);
	}
	if (stacksCount > 1)
	{
		if (containsUpgradedStack()) //upgrade
		{
			SlotID slotID = SlotID(static_cast<si32>(std::floor(static_cast<float>(stacks.size()) / 2.0f)));
			const auto & upgrades = getStack(slotID).type->upgrades;
			if(!upgrades.empty())
			{
				auto it = RandomGeneratorUtil::nextItem(upgrades, CRandomGenerator::getDefault());
				cb->changeStackType(StackLocation(this, slotID), VLC->creh->objects[*it]);
			}
		}
	}

	cb->startBattleI(h, this);

}

void CGCreature::flee( const CGHeroInstance * h ) const
{
	BlockingDialog ynd(true,false);
	ynd.player = h->tempOwner;
	ynd.text.appendLocalString(EMetaText::ADVOB_TXT,91);
	ynd.text.replaceLocalString(EMetaText::CRE_PL_NAMES, subID);
	cb->showBlockingDialog(&ynd);
}

void CGCreature::battleFinished(const CGHeroInstance *hero, const BattleResult &result) const
{
	if(result.winner == 0)
	{
		giveReward(hero);
		cb->removeObject(this);
	}
	else if(result.winner > 1) // draw
	{
		// guarded reward is lost forever on draw
		cb->removeObject(this);
	}
	else
	{
		//merge stacks into one
		TSlots::const_iterator i;
		CCreature * cre = VLC->creh->objects[formation.basicType];
		for(i = stacks.begin(); i != stacks.end(); i++)
		{
			if(cre->isMyUpgrade(i->second->type))
			{
				cb->changeStackType(StackLocation(this, i->first), cre); //un-upgrade creatures
			}
		}

		//first stack has to be at slot 0 -> if original one got killed, move there first remaining stack
		if(!hasStackAtSlot(SlotID(0)))
			cb->moveStack(StackLocation(this, stacks.begin()->first), StackLocation(this, SlotID(0)), stacks.begin()->second->count);

		while(stacks.size() > 1) //hopefully that's enough
		{
			// TODO it's either overcomplicated (if we assume there'll be only one stack) or buggy (if we allow multiple stacks... but that'll also cause troubles elsewhere)
			i = stacks.end();
			i--;
			SlotID slot = getSlotFor(i->second->type);
			if(slot == i->first) //no reason to move stack to its own slot
				break;
			else
				cb->moveStack(StackLocation(this, i->first), StackLocation(this, slot), i->second->count);
		}

		cb->setObjProperty(id, ObjProperty::MONSTER_POWER, stacks.begin()->second->count * 1000); //remember casualties
	}
}

void CGCreature::blockingDialogAnswered(const CGHeroInstance *hero, ui32 answer) const
{
	auto action = takenAction(hero);
	if(!refusedJoining && action >= JOIN_FOR_FREE) //higher means price
		joinDecision(hero, action, answer);
	else if(action != FIGHT)
		fleeDecision(hero, answer);
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

	int val = static_cast<int>(std::floor(a * pos.x + b * pos.y + c * pos.z + d));
	return ((val % 32768) % 100) < 50;
}

int CGCreature::getNumberOfStacks(const CGHeroInstance *hero) const
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

	ui32 a = 1550811371u;
	ui32 b = 3359066809u;
	ui32 c = 1943276003u;
	ui32 d = 3174620878u;

	ui32 R1 = a * static_cast<ui32>(pos.x) + b * static_cast<ui32>(pos.y) + c * static_cast<ui32>(pos.z) + d;
	ui32 R2 = (R1 >> 16) & 0x7fff;

	int R4 = R2 % 100 + 1;

	if (R4 <= 20)
		split -= 1;
	else if (R4 >= 80)
		split += 1;

	vstd::amin(split, getStack(SlotID(0)).count); //can't divide into more stacks than creatures total
	vstd::amin(split, 7); //can't have more than 7 stacks

	return split;
}

void CGCreature::giveReward(const CGHeroInstance * h) const
{
	InfoWindow iw;
	iw.player = h->tempOwner;

	if(!resources.empty())
	{
		cb->giveResources(h->tempOwner, resources);
		for(int i = 0; i < resources.size(); i++)
		{
			if(resources[i] > 0)
				iw.components.emplace_back(Component::EComponentType::RESOURCE, i, resources[i], 0);
		}
	}

	if(gainedArtifact != ArtifactID::NONE)
	{
		cb->giveHeroNewArtifact(h, VLC->arth->objects[gainedArtifact], ArtifactPosition::FIRST_AVAILABLE);
		iw.components.emplace_back(Component::EComponentType::ARTIFACT, gainedArtifact, 0, 0);
	}

	if(!iw.components.empty())
	{
		iw.type = EInfoWindowMode::AUTO;
		iw.text.appendLocalString(EMetaText::ADVOB_TXT, 183); // % has found treasure
		iw.text.replaceRawString(h->getNameTranslated());
		cb->showInfoDialog(&iw);
	}
}

static const std::vector<std::string> CHARACTER_JSON  =
{
	"compliant", "friendly", "aggressive", "hostile", "savage"
};

void CGCreature::serializeJsonOptions(JsonSerializeFormat & handler)
{
	if(cb && cb->getDate() > 1 && handler.saving)
	{
		auto oldCharacter = 0;
		if(character == 10)
			oldCharacter = 4;
		else if(character > 7)
			oldCharacter = 3;
		else
			oldCharacter = character < 3 ? 1 : 2;

		handler.serializeEnum("character", oldCharacter, CHARACTER_JSON);
	}
	else
		handler.serializeEnum("character", character, CHARACTER_JSON);

	if(handler.saving)
	{
		if(hasStackAtSlot(SlotID(0)))
		{
			si32 amount = getStack(SlotID(0)).count;
			handler.serializeInt("amount", amount, 0);
		}
	}
	else
	{
		si32 amount = 0;
		handler.serializeInt("amount", amount);
		auto * hlp = new CStackInstance();
		hlp->count = amount;
		//type will be set during initialization
		putStack(SlotID(0), hlp);
	}

	resources.serializeJson(handler, "rewardResources");

	handler.serializeId("rewardArtifact", gainedArtifact, ArtifactID(ArtifactID::NONE));

	handler.serializeBool("noGrowing", notGrowingTeam);
	handler.serializeBool("neverFlees", neverFlees);
	handler.serializeString("rewardMessage", message);
}

VCMI_LIB_NAMESPACE_END
