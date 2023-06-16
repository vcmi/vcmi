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
#include "MiscObjects.h"

#include "../StringConstants.h"
#include "../NetPacks.h"
#include "../CGeneralTextHandler.h"
#include "../CSoundBase.h"
#include "../CConfigHandler.h"
#include "../CModHandler.h"
#include "../CHeroHandler.h"
#include "../CSkillHandler.h"
#include "../spells/CSpellHandler.h"
#include "../IGameCallback.h"
#include "../CGameState.h"
#include "../mapping/CMap.h"
#include "../CPlayerState.h"
#include "../GameSettings.h"
#include "../serializer/JsonSerializeFormat.h"

VCMI_LIB_NAMESPACE_BEGIN

std::map <si32, std::vector<ObjectInstanceID> > CGMagi::eyelist;
ui8 CGObelisk::obeliskCount = 0; //how many obelisks are on map
std::map<TeamID, ui8> CGObelisk::visited; //map: team_id => how many obelisks has been visited

///helpers
static std::string visitedTxt(const bool visited)
{
	int id = visited ? 352 : 353;
	return VLC->generaltexth->allTexts[id];
}

void CTeamVisited::setPropertyDer(ui8 what, ui32 val)
{
	if(what == CTeamVisited::OBJPROP_VISITED)
		players.insert(PlayerColor(val));
}

bool CTeamVisited::wasVisited(PlayerColor player) const
{
	return wasVisited(cb->getPlayerState(player)->team);
}

bool CTeamVisited::wasVisited(const CGHeroInstance * h) const
{
	return wasVisited(h->tempOwner);
}

bool CTeamVisited::wasVisited(const TeamID & team) const
{
	for(const auto & i : players)
	{
		if(cb->getPlayerState(i)->team == team)
			return true;
	}
	return false;
}

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
		ms << CCreature::getQuantityRangeStringForId(monsterQuantityId);
	else
		ms.addTxt(MetaString::ARRAY_TXT, quantityTextIndex);
	ms << " " ;
	ms.addTxt(MetaString::CRE_PL_NAMES,subID);
	ms.toString(hoverName);
	return hoverName;
}

std::string CGCreature::getHoverText(const CGHeroInstance * hero) const
{
	std::string hoverName;
	if(hero->hasVisions(this, 0))
	{
		MetaString ms;
		ms << stacks.begin()->second->count;
		ms << " " ;
		ms.addTxt(MetaString::CRE_PL_NAMES,subID);

		ms << "\n";

		int decision = takenAction(hero, true);

		switch (decision)
		{
		case FIGHT:
			ms.addTxt(MetaString::GENERAL_TXT,246);
			break;
		case FLEE:
			ms.addTxt(MetaString::GENERAL_TXT,245);
			break;
		case JOIN_FOR_FREE:
			ms.addTxt(MetaString::GENERAL_TXT,243);
			break;
		default: //decision = cost in gold
			ms << boost::to_string(boost::format(VLC->generaltexth->allTexts[244]) % decision);
			break;
		}

		ms.toString(hoverName);
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
			ynd.text.addTxt(MetaString::ADVOB_TXT, 86);
			ynd.text.addReplacement(MetaString::CRE_PL_NAMES, subID);
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
			ynd.text << tmp;
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
			iw.text << std::pair<ui8,ui32>(1,29);  //You don't have enough gold
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
	ynd.text.addTxt(MetaString::ADVOB_TXT,91);
	ynd.text.addReplacement(MetaString::CRE_PL_NAMES, subID);
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
		iw.text.addTxt(MetaString::ADVOB_TXT, 183); // % has found treasure
		iw.text.addReplacement(h->getNameTranslated());
		cb->showInfoDialog(&iw);
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

//CGMine
void CGMine::onHeroVisit( const CGHeroInstance * h ) const
{
	int relations = cb->gameState()->getPlayerRelations(h->tempOwner, tempOwner);

	if(relations == 2) //we're visiting our mine
	{
		cb->showGarrisonDialog(id,h->id,true);
		return;
	}
	else if (relations == 1)//ally
		return;

	if(stacksCount()) //Mine is guarded
	{
		BlockingDialog ynd(true,false);
		ynd.player = h->tempOwner;
		ynd.text.addTxt(MetaString::ADVOB_TXT, subID == 7 ? 84 : 187);
		cb->showBlockingDialog(&ynd);
		return;
	}

	flagMine(h->tempOwner);

}

void CGMine::newTurn(CRandomGenerator & rand) const
{
	if(cb->getDate() == 1)
		return;

	if (tempOwner == PlayerColor::NEUTRAL)
		return;

	cb->giveResource(tempOwner, producedResource, producedQuantity);
}

void CGMine::initObj(CRandomGenerator & rand)
{
	if(isAbandoned())
	{
		//set guardians
		int howManyTroglodytes = rand.nextInt(100, 199);
		auto * troglodytes = new CStackInstance(CreatureID::TROGLODYTES, howManyTroglodytes);
		putStack(SlotID(0), troglodytes);

		assert(!abandonedMineResources.empty());
		producedResource = *RandomGeneratorUtil::nextItem(abandonedMineResources, rand);
	}
	else
	{
		producedResource = GameResID(subID);
	}
	producedQuantity = defaultResProduction();
}

bool CGMine::isAbandoned() const
{
	return (subID >= 7);
}

std::string CGMine::getObjectName() const
{
	return VLC->generaltexth->translate("core.minename", subID);
}

std::string CGMine::getHoverText(PlayerColor player) const
{
	std::string hoverName = CArmedInstance::getHoverText(player);

	if (tempOwner != PlayerColor::NEUTRAL)
		hoverName += "\n(" + VLC->generaltexth->restypes[producedResource] + ")";

	if(stacksCount())
	{
		hoverName += "\n";
		hoverName += VLC->generaltexth->allTexts[202]; //Guarded by
		hoverName += " ";
		hoverName += getArmyDescription();
	}
	return hoverName;
}

void CGMine::flagMine(const PlayerColor & player) const
{
	assert(tempOwner != player);
	cb->setOwner(this, player); //not ours? flag it!

	InfoWindow iw;
	iw.type = EInfoWindowMode::AUTO;
	iw.soundID = soundBase::FLAGMINE;
	iw.text.addTxt(MetaString::MINE_EVNTS, producedResource); //not use subID, abandoned mines uses default mine texts
	iw.player = player;
	iw.components.emplace_back(Component::EComponentType::RESOURCE, producedResource, producedQuantity, -1);
	cb->showInfoDialog(&iw);
}

ui32 CGMine::defaultResProduction() const
{
	switch(producedResource.toEnum())
	{
	case EGameResID::WOOD:
	case EGameResID::ORE:
		return 2;
	case EGameResID::GOLD:
		return 1000;
	default:
		return 1;
	}
}

void CGMine::battleFinished(const CGHeroInstance *hero, const BattleResult &result) const
{
	if(result.winner == 0) //attacker won
	{
		if(isAbandoned())
		{
			hero->showInfoDialog(85);
		}
		flagMine(hero->tempOwner);
	}
}

void CGMine::blockingDialogAnswered(const CGHeroInstance *hero, ui32 answer) const
{
	if(answer)
		cb->startBattleI(hero, this);
}

void CGMine::serializeJsonOptions(JsonSerializeFormat & handler)
{
	CCreatureSet::serializeJson(handler, "army", 7);

	if(isAbandoned())
	{
		if(handler.saving)
		{
			JsonNode node(JsonNode::JsonType::DATA_VECTOR);
			for(auto const & resID : abandonedMineResources)
			{
				JsonNode one(JsonNode::JsonType::DATA_STRING);
				one.String() = GameConstants::RESOURCE_NAMES[resID];
				node.Vector().push_back(one);
			}
			handler.serializeRaw("possibleResources", node, std::nullopt);
		}
		else
		{
			auto guard = handler.enterArray("possibleResources");
			const JsonNode & node = handler.getCurrent();

			auto names = node.convertTo<std::vector<std::string>>();

			for(const std::string & s : names)
			{
				int raw_res = vstd::find_pos(GameConstants::RESOURCE_NAMES, s);
				if(raw_res < 0)
					logGlobal->error("Invalid resource name: %s", s);
				else
					abandonedMineResources.emplace(raw_res);
			}
		}
	}
	else
	{
		serializeJsonOwner(handler);
	}
}

std::string CGResource::getHoverText(PlayerColor player) const
{
	return VLC->generaltexth->restypes[subID];
}

void CGResource::initObj(CRandomGenerator & rand)
{
	blockVisit = true;

	if(amount == CGResource::RANDOM_AMOUNT)
	{
		switch(static_cast<EGameResID>(subID))
		{
		case EGameResID::GOLD:
			amount = rand.nextInt(5, 10) * 100;
			break;
		case EGameResID::WOOD: case EGameResID::ORE:
			amount = rand.nextInt(6, 10);
			break;
		default:
			amount = rand.nextInt(3, 5);
			break;
		}
	}
}

void CGResource::onHeroVisit( const CGHeroInstance * h ) const
{
	if(stacksCount())
	{
		if(!message.empty())
		{
			BlockingDialog ynd(true,false);
			ynd.player = h->getOwner();
			ynd.text << message;
			cb->showBlockingDialog(&ynd);
		}
		else
		{
			blockingDialogAnswered(h, true); //behave as if player accepted battle
		}
	}
	else
		collectRes(h->getOwner());
}

void CGResource::collectRes(const PlayerColor & player) const
{
	cb->giveResource(player, static_cast<EGameResID>(subID), amount);
	InfoWindow sii;
	sii.player = player;
	if(!message.empty())
	{
		sii.type = EInfoWindowMode::AUTO;
		sii.text << message;
	}
	else
	{
		sii.type = EInfoWindowMode::INFO;
		sii.text.addTxt(MetaString::ADVOB_TXT,113);
		sii.text.addReplacement(MetaString::RES_NAMES, subID);
	}
	sii.components.emplace_back(Component::EComponentType::RESOURCE,subID,amount,0);
	sii.soundID = soundBase::pickup01 + CRandomGenerator::getDefault().nextInt(6);
	cb->showInfoDialog(&sii);
	cb->removeObject(this);
}

void CGResource::battleFinished(const CGHeroInstance *hero, const BattleResult &result) const
{
	if(result.winner == 0) //attacker won
		collectRes(hero->getOwner());
}

void CGResource::blockingDialogAnswered(const CGHeroInstance *hero, ui32 answer) const
{
	if(answer)
		cb->startBattleI(hero, this);
}

void CGResource::serializeJsonOptions(JsonSerializeFormat & handler)
{
	CCreatureSet::serializeJson(handler, "guards", 7);
	handler.serializeInt("amount", amount, 0);
	handler.serializeString("guardMessage", message);
}

bool CGTeleport::isEntrance() const
{
	return type == BOTH || type == ENTRANCE;
}

bool CGTeleport::isExit() const
{
	return type == BOTH || type == EXIT;
}

bool CGTeleport::isChannelEntrance(const ObjectInstanceID & id) const
{
	return vstd::contains(getAllEntrances(), id);
}

bool CGTeleport::isChannelExit(const ObjectInstanceID & id) const
{
	return vstd::contains(getAllExits(), id);
}

std::vector<ObjectInstanceID> CGTeleport::getAllEntrances(bool excludeCurrent) const
{
	auto ret = cb->getTeleportChannelEntraces(channel);
	if(excludeCurrent)
		vstd::erase_if_present(ret, id);

	return ret;
}

std::vector<ObjectInstanceID> CGTeleport::getAllExits(bool excludeCurrent) const
{
	auto ret = cb->getTeleportChannelExits(channel);
	if(excludeCurrent)
		vstd::erase_if_present(ret, id);

	return ret;
}

ObjectInstanceID CGTeleport::getRandomExit(const CGHeroInstance * h) const
{
	auto passableExits = getPassableExits(cb->gameState(), h, getAllExits(true));
	if(!passableExits.empty())
		return *RandomGeneratorUtil::nextItem(passableExits, CRandomGenerator::getDefault());

	return ObjectInstanceID();
}

bool CGTeleport::isTeleport(const CGObjectInstance * obj)
{
	return ((dynamic_cast<const CGTeleport *>(obj)));
}

bool CGTeleport::isConnected(const CGTeleport * src, const CGTeleport * dst)
{
	return src && dst && src->isChannelExit(dst->id);
}

bool CGTeleport::isConnected(const CGObjectInstance * src, const CGObjectInstance * dst)
{
	const auto * srcObj = dynamic_cast<const CGTeleport *>(src);
	const auto * dstObj = dynamic_cast<const CGTeleport *>(dst);
	return isConnected(srcObj, dstObj);
}

bool CGTeleport::isExitPassable(CGameState * gs, const CGHeroInstance * h, const CGObjectInstance * obj)
{
	auto * objTopVisObj = gs->map->getTile(obj->visitablePos()).topVisitableObj();
	if(objTopVisObj->ID == Obj::HERO)
	{
		if(h->id == objTopVisObj->id) // Just to be sure it's won't happen.
			return false;

		// Check if it's friendly hero or not
		if(gs->getPlayerRelations(h->tempOwner, objTopVisObj->tempOwner) != PlayerRelations::ENEMIES)
		{
			// Exchange between heroes only possible via subterranean gates
			if(!dynamic_cast<const CGSubterraneanGate *>(obj))
				return false;
		}
	}
	return true;
}

std::vector<ObjectInstanceID> CGTeleport::getPassableExits(CGameState * gs, const CGHeroInstance * h, std::vector<ObjectInstanceID> exits)
{
	vstd::erase_if(exits, [&](const ObjectInstanceID & exit) -> bool 
	{
		return !isExitPassable(gs, h, gs->getObj(exit));
	});
	return exits;
}

void CGTeleport::addToChannel(std::map<TeleportChannelID, std::shared_ptr<TeleportChannel> > &channelsList, const CGTeleport * obj)
{
	std::shared_ptr<TeleportChannel> tc;
	if(channelsList.find(obj->channel) == channelsList.end())
	{
		tc = std::make_shared<TeleportChannel>();
		channelsList.insert(std::make_pair(obj->channel, tc));
	}
	else
		tc = channelsList[obj->channel];

	if(obj->isEntrance() && !vstd::contains(tc->entrances, obj->id))
		tc->entrances.push_back(obj->id);

	if(obj->isExit() && !vstd::contains(tc->exits, obj->id))
		tc->exits.push_back(obj->id);

	if(!tc->entrances.empty() && !tc->exits.empty()
		&& (tc->entrances.size() != 1 || tc->entrances != tc->exits))
	{
		tc->passability = TeleportChannel::PASSABLE;
	}
}

TeleportChannelID CGMonolith::findMeChannel(const std::vector<Obj> & IDs, int SubID) const
{
	for(auto obj : cb->gameState()->map->objects)
	{
		if(!obj)
			continue;

		const auto * teleportObj = dynamic_cast<const CGTeleport *>(cb->getObj(obj->id));
		if(teleportObj && vstd::contains(IDs, teleportObj->ID) && teleportObj->subID == SubID)
			return teleportObj->channel;
	}
	return TeleportChannelID();
}

void CGMonolith::onHeroVisit( const CGHeroInstance * h ) const
{
	TeleportDialog td(h->tempOwner, channel);
	if(isEntrance())
	{
		if(cb->isTeleportChannelBidirectional(channel) && 1 < cb->getTeleportChannelExits(channel).size())
		{
			auto exits = cb->getTeleportChannelExits(channel);
			for(const auto & exit : exits)
			{
				td.exits.push_back(std::make_pair(exit, h->convertFromVisitablePos(cb->getObj(exit)->visitablePos())));
			}
		}

		if(cb->isTeleportChannelImpassable(channel))
		{
			logGlobal->debug("Cannot find corresponding exit monolith for %d at %s", id.getNum(), pos.toString());
			td.impassable = true;
		}
		else if(getRandomExit(h) == ObjectInstanceID())
			logGlobal->debug("All exits blocked for monolith %d at %s", id.getNum(), pos.toString());
	}
	else
		h->showInfoDialog(70);

	cb->showTeleportDialog(&td);
}

void CGMonolith::teleportDialogAnswered(const CGHeroInstance *hero, ui32 answer, TTeleportExitsList exits) const
{
	int3 dPos;
	auto randomExit = getRandomExit(hero);
	auto realExits = getAllExits(true);
	if(!isEntrance() // Do nothing if hero visited exit only object
		|| (exits.empty() && realExits.empty()) // Do nothing if there no exits on this channel
		|| ObjectInstanceID() == randomExit) // Do nothing if all exits are blocked by friendly hero and it's not subterranean gate
	{
		return;
	}
	else if(vstd::isValidIndex(exits, answer))
		dPos = exits[answer].second;
	else
		dPos = hero->convertFromVisitablePos(cb->getObj(randomExit)->visitablePos());

	cb->moveHero(hero->id, dPos, true);
}

void CGMonolith::initObj(CRandomGenerator & rand)
{
	std::vector<Obj> IDs;
	IDs.push_back(ID);
	switch(ID)
	{
	case Obj::MONOLITH_ONE_WAY_ENTRANCE:
		type = ENTRANCE;
		IDs.emplace_back(Obj::MONOLITH_ONE_WAY_EXIT);
		break;
	case Obj::MONOLITH_ONE_WAY_EXIT:
		type = EXIT;
		IDs.emplace_back(Obj::MONOLITH_ONE_WAY_ENTRANCE);
		break;
	case Obj::MONOLITH_TWO_WAY:
	default:
		type = BOTH;
		break;
	}

	channel = findMeChannel(IDs, subID);
	if(channel == TeleportChannelID())
		channel = TeleportChannelID(static_cast<si32>(cb->gameState()->map->teleportChannels.size()));

	addToChannel(cb->gameState()->map->teleportChannels, this);
}

void CGSubterraneanGate::onHeroVisit( const CGHeroInstance * h ) const
{
	TeleportDialog td(h->tempOwner, channel);
	if(cb->isTeleportChannelImpassable(channel))
	{
		h->showInfoDialog(153);//Just inside the entrance you find a large pile of rubble blocking the tunnel. You leave discouraged.
		logGlobal->debug("Cannot find exit subterranean gate for  %d at %s", id.getNum(), pos.toString());
		td.impassable = true;
	}
	else
	{
		auto exit = getRandomExit(h);
		td.exits.push_back(std::make_pair(exit, h->convertFromVisitablePos(cb->getObj(exit)->visitablePos())));
	}

	cb->showTeleportDialog(&td);
}

void CGSubterraneanGate::initObj(CRandomGenerator & rand)
{
	type = BOTH;
}

void CGSubterraneanGate::postInit() //matches subterranean gates into pairs
{
	//split on underground and surface gates
	std::vector<CGSubterraneanGate *> gatesSplit[2]; //surface and underground gates
	for(auto & obj : cb->gameState()->map->objects)
	{
		if(!obj) // FIXME: Find out why there are nullptr objects right after initialization
			continue;

		auto * hlp = dynamic_cast<CGSubterraneanGate *>(cb->gameState()->getObjInstance(obj->id));
		if(hlp)
			gatesSplit[hlp->pos.z].push_back(hlp);
	}

	//sort by position
	std::sort(gatesSplit[0].begin(), gatesSplit[0].end(), [](const CGObjectInstance * a, const CGObjectInstance * b)
	{
		return a->pos < b->pos;
	});

	auto assignToChannel = [&](CGSubterraneanGate * obj)
	{
		if(obj->channel == TeleportChannelID())
		{ // if object not linked to channel then create new channel
			obj->channel = TeleportChannelID(static_cast<si32>(cb->gameState()->map->teleportChannels.size()));
			addToChannel(cb->gameState()->map->teleportChannels, obj);
		}
	};

	for(size_t i = 0; i < gatesSplit[0].size(); i++)
	{
		CGSubterraneanGate * objCurrent = gatesSplit[0][i];

		//find nearest underground exit
		std::pair<int, si32> best(-1, std::numeric_limits<si32>::max()); //pair<pos_in_vector, distance^2>
		for(int j = 0; j < gatesSplit[1].size(); j++)
		{
			CGSubterraneanGate *checked = gatesSplit[1][j];
			if(checked->channel != TeleportChannelID())
				continue;
			si32 hlp = checked->pos.dist2dSQ(objCurrent->pos);
			if(hlp < best.second)
			{
				best.first = j;
				best.second = hlp;
			}
		}

		assignToChannel(objCurrent);
		if(best.first >= 0) //found pair
		{
			gatesSplit[1][best.first]->channel = objCurrent->channel;
			addToChannel(cb->gameState()->map->teleportChannels, gatesSplit[1][best.first]);
		}
	}

	// we should assign empty channels to underground gates if they don't have matching overground gates
	for(auto & i : gatesSplit[1])
		assignToChannel(i);
}

void CGWhirlpool::onHeroVisit( const CGHeroInstance * h ) const
{
	TeleportDialog td(h->tempOwner, channel);
	if(cb->isTeleportChannelImpassable(channel))
	{
		logGlobal->debug("Cannot find exit whirlpool for %d at %s", id.getNum(), pos.toString());
		td.impassable = true;
	}
	else if(getRandomExit(h) == ObjectInstanceID())
		logGlobal->debug("All exits are blocked for whirlpool  %d at %s", id.getNum(), pos.toString());

	if(!isProtected(h))
	{
		SlotID targetstack = h->Slots().begin()->first; //slot numbers may vary
		for(auto i = h->Slots().rbegin(); i != h->Slots().rend(); i++)
		{
			if(h->getPower(targetstack) > h->getPower(i->first))
				targetstack = (i->first);
		}

		auto countToTake = static_cast<TQuantity>(h->getStackCount(targetstack) * 0.5);
		vstd::amax(countToTake, 1);

		InfoWindow iw;
		iw.type = EInfoWindowMode::AUTO;
		iw.player = h->tempOwner;
		iw.text.addTxt(MetaString::ADVOB_TXT, 168);
		iw.components.emplace_back(CStackBasicDescriptor(h->getCreature(targetstack), -countToTake));
		cb->showInfoDialog(&iw);
		cb->changeStackCount(StackLocation(h, targetstack), -countToTake);
	}
	else
	{
		auto exits = getAllExits();
		for(const auto & exit : exits)
		{
			auto blockedPosList = cb->getObj(exit)->getBlockedPos();
			for(const auto & bPos : blockedPosList)
				td.exits.push_back(std::make_pair(exit, h->convertFromVisitablePos(bPos)));
		}
	}

	cb->showTeleportDialog(&td);
}

void CGWhirlpool::teleportDialogAnswered(const CGHeroInstance *hero, ui32 answer, TTeleportExitsList exits) const
{
	int3 dPos;
	auto realExits = getAllExits();
	if(exits.empty() && realExits.empty())
		return;
	else if(vstd::isValidIndex(exits, answer))
		dPos = exits[answer].second;
	else
	{
		auto exit = getRandomExit(hero);

		if(exit == ObjectInstanceID())
			return;

		const auto * obj = cb->getObj(exit);
		std::set<int3> tiles = obj->getBlockedPos();
		dPos = hero->convertFromVisitablePos(*RandomGeneratorUtil::nextItem(tiles, CRandomGenerator::getDefault()));
	}

	cb->moveHero(hero->id, dPos, true);
}

bool CGWhirlpool::isProtected(const CGHeroInstance * h)
{
	return h->hasBonusOfType(BonusType::WHIRLPOOL_PROTECTION)
	|| (h->stacksCount() == 1 && h->Slots().begin()->second->count == 1);
}

void CGArtifact::initObj(CRandomGenerator & rand)
{
	blockVisit = true;
	if(ID == Obj::ARTIFACT)
	{
		if (!storedArtifact)
		{
			auto * a = new CArtifactInstance();
			cb->gameState()->map->addNewArtifactInstance(a);
			storedArtifact = a;
		}
		if(!storedArtifact->artType)
			storedArtifact->setType(VLC->arth->objects[subID]);
	}
	if(ID == Obj::SPELL_SCROLL)
		subID = 1;

	assert(storedArtifact->artType);
	assert(storedArtifact->getParentNodes().size());

	//assert(storedArtifact->artType->id == subID); //this does not stop desync
}

std::string CGArtifact::getObjectName() const
{
	return VLC->artifacts()->getByIndex(subID)->getNameTranslated();
}

void CGArtifact::onHeroVisit(const CGHeroInstance * h) const
{
	if(!stacksCount())
	{
		InfoWindow iw;
		iw.type = EInfoWindowMode::AUTO;
		iw.player = h->tempOwner;

		if(storedArtifact->artType->canBePutAt(h))
		{
			switch (ID)
			{
			case Obj::ARTIFACT:
			{
				iw.components.emplace_back(Component::EComponentType::ARTIFACT, subID, 0, 0);
				if(message.length())
					iw.text << message;
				else
					iw.text.addTxt(MetaString::ART_EVNTS, subID);
			}
			break;
			case Obj::SPELL_SCROLL:
			{
				int spellID = storedArtifact->getScrollSpellID();
				iw.components.emplace_back(Component::EComponentType::SPELL, spellID, 0, 0);
				if(message.length())
					iw.text << message;
				else
				{
					iw.text.addTxt(MetaString::ADVOB_TXT,135);
					iw.text.addReplacement(MetaString::SPELL_NAME, spellID);
				}
			}
			break;
			}
		}
		else
		{
			iw.text.addTxt(MetaString::ADVOB_TXT, 2);
		}
		cb->showInfoDialog(&iw);
		pick(h);
	}
	else
	{
		switch(ID)
		{
		case Obj::ARTIFACT:
			{
				BlockingDialog ynd(true,false);
				ynd.player = h->getOwner();
				if(message.length())
					ynd.text << message;
				else
				{
					// TODO: Guard text is more complex in H3, see mantis issue 2325 for details
					ynd.text.addTxt(MetaString::GENERAL_TXT, 420);
					ynd.text.addReplacement("");
					ynd.text.addReplacement(getArmyDescription());
					ynd.text.addReplacement(MetaString::GENERAL_TXT, 43); // creatures
				}
				cb->showBlockingDialog(&ynd);
			}
			break;
		case Obj::SPELL_SCROLL:
			{
				if(message.length())
				{
					BlockingDialog ynd(true,false);
					ynd.player = h->getOwner();
					ynd.text << message;
					cb->showBlockingDialog(&ynd);
				}
				else
					blockingDialogAnswered(h, true);
			}
			break;
		}
	}
}

void CGArtifact::pick(const CGHeroInstance * h) const
{
	if(cb->giveHeroArtifact(h, storedArtifact, ArtifactPosition::FIRST_AVAILABLE))
		cb->removeObject(this);
}

BattleField CGArtifact::getBattlefield() const
{
	return BattleField::NONE;
}

void CGArtifact::battleFinished(const CGHeroInstance *hero, const BattleResult &result) const
{
	if(result.winner == 0) //attacker won
		pick(hero);
}

void CGArtifact::blockingDialogAnswered(const CGHeroInstance *hero, ui32 answer) const
{
	if(answer)
		cb->startBattleI(hero, this);
}

void CGArtifact::afterAddToMap(CMap * map)
{
	//Artifacts from map objects are never removed
	//FIXME: This should be revertible in map editor

	if(ID == Obj::SPELL_SCROLL && storedArtifact && storedArtifact->id.getNum() < 0)
        map->addNewArtifactInstance(storedArtifact);
}

void CGArtifact::serializeJsonOptions(JsonSerializeFormat& handler)
{
	handler.serializeString("guardMessage", message);
	CCreatureSet::serializeJson(handler, "guards" ,7);

	if(handler.saving && ID == Obj::SPELL_SCROLL)
	{
		const std::shared_ptr<Bonus> b = storedArtifact->getBonusLocalFirst(Selector::type()(BonusType::SPELL));
		SpellID spellId(b->subtype);

		handler.serializeId("spell", spellId, SpellID::NONE);
	}
}

void CGWitchHut::initObj(CRandomGenerator & rand)
{
	if (allowedAbilities.empty()) //this can happen for RMG and RoE maps.
	{
		auto defaultAllowed = VLC->skillh->getDefaultAllowed();

		// Necromancy and Leadership can't be learned by default
		defaultAllowed[SecondarySkill::NECROMANCY] = false;
		defaultAllowed[SecondarySkill::LEADERSHIP] = false;

		for(int i = 0; i < defaultAllowed.size(); i++)
			if (defaultAllowed[i] && cb->isAllowed(2, i))
				allowedAbilities.insert(SecondarySkill(i));
	}
	ability = *RandomGeneratorUtil::nextItem(allowedAbilities, rand);
}

void CGWitchHut::onHeroVisit( const CGHeroInstance * h ) const
{
	InfoWindow iw;
	iw.type = EInfoWindowMode::AUTO;
	iw.player = h->getOwner();
	if(!wasVisited(h->tempOwner))
		cb->setObjProperty(id, CGWitchHut::OBJPROP_VISITED, h->tempOwner.getNum());
	ui32 txt_id;
	if(h->getSecSkillLevel(SecondarySkill(ability))) //you already know this skill
	{
		txt_id =172;
	}
	else if(!h->canLearnSkill()) //already all skills slots used
	{
		txt_id = 173;
	}
	else //give sec skill
	{
		iw.components.emplace_back(Component::EComponentType::SEC_SKILL, ability, 1, 0);
		txt_id = 171;
		cb->changeSecSkill(h, SecondarySkill(ability), 1, true);
	}

	iw.text.addTxt(MetaString::ADVOB_TXT,txt_id);
	iw.text.addReplacement(MetaString::SEC_SKILL_NAME, ability);
	cb->showInfoDialog(&iw);
}

std::string CGWitchHut::getHoverText(PlayerColor player) const
{
	std::string hoverName = getObjectName();
	if(wasVisited(player))
	{
		hoverName += "\n" + VLC->generaltexth->allTexts[356]; // + (learn %s)
		boost::algorithm::replace_first(hoverName, "%s", VLC->skillh->getByIndex(ability)->getNameTranslated());
	}
	return hoverName;
}

std::string CGWitchHut::getHoverText(const CGHeroInstance * hero) const
{
	std::string hoverName = getHoverText(hero->tempOwner);
	if(wasVisited(hero->tempOwner) && hero->getSecSkillLevel(SecondarySkill(ability))) //hero knows that ability
		hoverName += "\n\n" + VLC->generaltexth->allTexts[357]; // (Already learned)
	return hoverName;
}

void CGWitchHut::serializeJsonOptions(JsonSerializeFormat & handler)
{
	//TODO: unify allowed abilities with others - make them std::vector<bool>

	std::vector<bool> temp;
	size_t skillCount = VLC->skillh->size();
	temp.resize(skillCount, false);

	auto standard = VLC->skillh->getDefaultAllowed(); //todo: for WitchHut default is all except Leadership and Necromancy

	if(handler.saving)
	{
		for(si32 i = 0; i < skillCount; ++i)
			if(vstd::contains(allowedAbilities, i))
				temp[i] = true;
	}

	handler.serializeLIC("allowedSkills", &CSkillHandler::decodeSkill, &CSkillHandler::encodeSkill, standard, temp);

	if(!handler.saving)
	{
		allowedAbilities.clear();
		for(si32 i = 0; i < skillCount; ++i)
			if(temp[i])
				allowedAbilities.insert(SecondarySkill(i));
	}
}

void CGObservatory::onHeroVisit( const CGHeroInstance * h ) const
{
	InfoWindow iw;
	iw.type = EInfoWindowMode::AUTO;
	iw.player = h->tempOwner;
	switch (ID)
	{
	case Obj::REDWOOD_OBSERVATORY:
	case Obj::PILLAR_OF_FIRE:
	{
		iw.text.addTxt(MetaString::ADVOB_TXT,98 + (ID==Obj::PILLAR_OF_FIRE));

		FoWChange fw;
		fw.player = h->tempOwner;
		fw.mode = 1;
		cb->getTilesInRange (fw.tiles, pos, 20, h->tempOwner, 1);
		cb->sendAndApply (&fw);
		break;
	}
	case Obj::COVER_OF_DARKNESS:
	{
		iw.text.addTxt (MetaString::ADVOB_TXT, 31);
		for (auto & player : cb->gameState()->players)
		{
			if (cb->getPlayerStatus(player.first) == EPlayerStatus::INGAME &&
				cb->getPlayerRelations(player.first, h->tempOwner) == PlayerRelations::ENEMIES)
				cb->changeFogOfWar(visitablePos(), 20, player.first, true);
		}
		break;
	}
	}
	cb->showInfoDialog(&iw);
}

void CGShrine::onHeroVisit( const CGHeroInstance * h ) const
{
	if(spell == SpellID::NONE)
	{
		logGlobal->error("Not initialized shrine visited!");
		return;
	}

	if(!wasVisited(h->tempOwner))
		cb->setObjProperty(id, CGShrine::OBJPROP_VISITED, h->tempOwner.getNum());

	InfoWindow iw;
	iw.type = EInfoWindowMode::AUTO;
	iw.player = h->getOwner();
	iw.text.addTxt(MetaString::ADVOB_TXT,127 + ID - 88);
	iw.text.addTxt(MetaString::SPELL_NAME,spell);
	iw.text << ".";

	if(!h->getArt(ArtifactPosition::SPELLBOOK))
	{
		iw.text.addTxt(MetaString::ADVOB_TXT,131);
	}
	else if(h->spellbookContainsSpell(spell))//hero already knows the spell
	{
		iw.text.addTxt(MetaString::ADVOB_TXT,174);
	}
	else if(ID == Obj::SHRINE_OF_MAGIC_THOUGHT  && h->maxSpellLevel() < 3) //it's third level spell and hero doesn't have wisdom
	{
		iw.text.addTxt(MetaString::ADVOB_TXT,130);
	}
	else //give spell
	{
		std::set<SpellID> spells;
		spells.insert(spell);
		cb->changeSpells(h, true, spells);

		iw.components.emplace_back(Component::EComponentType::SPELL, spell, 0, 0);
	}

	cb->showInfoDialog(&iw);
}

void CGShrine::initObj(CRandomGenerator & rand)
{
	if(spell == SpellID::NONE) //spell not set
	{
		int level = ID-87;
		std::vector<SpellID> possibilities;
		cb->getAllowedSpells (possibilities, level);

		if(possibilities.empty())
		{
			logGlobal->error("Error: cannot init shrine, no allowed spells!");
			return;
		}

		spell = *RandomGeneratorUtil::nextItem(possibilities, rand);
	}
}

std::string CGShrine::getHoverText(PlayerColor player) const
{
	std::string hoverName = getObjectName();
	if(wasVisited(player))
	{
		hoverName += "\n" + VLC->generaltexth->allTexts[355]; // + (learn %s)
		boost::algorithm::replace_first(hoverName,"%s", spell.toSpell()->getNameTranslated());
	}
	return hoverName;
}

std::string CGShrine::getHoverText(const CGHeroInstance * hero) const
{
	std::string hoverName = getHoverText(hero->tempOwner);
	if(wasVisited(hero->tempOwner) && hero->spellbookContainsSpell(spell)) //know what spell there is and hero knows that spell
		hoverName += "\n\n" + VLC->generaltexth->allTexts[354]; // (Already learned)
	return hoverName;
}

void CGShrine::serializeJsonOptions(JsonSerializeFormat & handler)
{
	handler.serializeId("spell", spell, SpellID::NONE);
}

void CGSignBottle::initObj(CRandomGenerator & rand)
{
	//if no text is set than we pick random from the predefined ones
	if(message.empty())
	{
		auto vector = VLC->generaltexth->findStringsWithPrefix("core.randsign");
		std::string messageIdentifier = *RandomGeneratorUtil::nextItem(vector, rand);
		message = VLC->generaltexth->translate(messageIdentifier);
	}

	if(ID == Obj::OCEAN_BOTTLE)
	{
		blockVisit = true;
	}
}

void CGSignBottle::onHeroVisit( const CGHeroInstance * h ) const
{
	InfoWindow iw;
	iw.player = h->getOwner();
	iw.text << message;
	cb->showInfoDialog(&iw);

	if(ID == Obj::OCEAN_BOTTLE)
		cb->removeObject(this);
}

void CGSignBottle::serializeJsonOptions(JsonSerializeFormat& handler)
{
	handler.serializeString("text", message);
}

void CGScholar::onHeroVisit( const CGHeroInstance * h ) const
{
	EBonusType type = bonusType;
	int bid = bonusID;
	//check if the bonus if applicable, if not - give primary skill (always possible)
	int ssl = h->getSecSkillLevel(SecondarySkill(bid)); //current sec skill level, used if bonusType == 1
	if((type == SECONDARY_SKILL	&& ((ssl == 3)  ||  (!ssl  &&  !h->canLearnSkill()))) ////hero already has expert level in the skill or (don't know skill and doesn't have free slot)
		|| (type == SPELL && !h->canLearnSpell(SpellID(bid).toSpell())))
	{
		type = PRIM_SKILL;
		bid = CRandomGenerator::getDefault().nextInt(GameConstants::PRIMARY_SKILLS - 1);
	}

	InfoWindow iw;
	iw.type = EInfoWindowMode::AUTO;
	iw.player = h->getOwner();
	iw.text.addTxt(MetaString::ADVOB_TXT,115);

	switch (type)
	{
	case PRIM_SKILL:
		cb->changePrimSkill(h,static_cast<PrimarySkill::PrimarySkill>(bid),+1);
		iw.components.emplace_back(Component::EComponentType::PRIM_SKILL, bid, +1, 0);
		break;
	case SECONDARY_SKILL:
		cb->changeSecSkill(h,SecondarySkill(bid),+1);
		iw.components.emplace_back(Component::EComponentType::SEC_SKILL, bid, ssl + 1, 0);
		break;
	case SPELL:
		{
			std::set<SpellID> hlp;
			hlp.insert(SpellID(bid));
			cb->changeSpells(h,true,hlp);
			iw.components.emplace_back(Component::EComponentType::SPELL, bid, 0, 0);
		}
		break;
	default:
		logGlobal->error("Error: wrong bonus type (%d) for Scholar!\n", static_cast<int>(type));
		return;
	}

	cb->showInfoDialog(&iw);
	cb->removeObject(this);
}

void CGScholar::initObj(CRandomGenerator & rand)
{
	blockVisit = true;
	if(bonusType == RANDOM)
	{
		bonusType = static_cast<EBonusType>(rand.nextInt(2));
		switch(bonusType)
		{
		case PRIM_SKILL:
			bonusID = rand.nextInt(GameConstants::PRIMARY_SKILLS -1);
			break;
		case SECONDARY_SKILL:
			bonusID = rand.nextInt(static_cast<int>(VLC->skillh->size()) - 1);
			break;
		case SPELL:
			std::vector<SpellID> possibilities;
			for (int i = 1; i < 6; ++i)
				cb->getAllowedSpells (possibilities, i);
			bonusID = *RandomGeneratorUtil::nextItem(possibilities, rand);
			break;
		}
	}
}

void CGScholar::serializeJsonOptions(JsonSerializeFormat & handler)
{
	if(handler.saving)
	{
		std::string value;
		switch(bonusType)
		{
		case PRIM_SKILL:
			value = PrimarySkill::names[bonusID];
			handler.serializeString("rewardPrimSkill", value);
			break;
		case SECONDARY_SKILL:
			value = CSkillHandler::encodeSkill(bonusID);
			handler.serializeString("rewardSkill", value);
			break;
		case SPELL:
			value = SpellID::encode(bonusID);
			handler.serializeString("rewardSpell", value);
			break;
		case RANDOM:
			break;
		}
	}
	else
	{
		//TODO: unify
		const JsonNode & json = handler.getCurrent();
		bonusType = RANDOM;
		if(!json["rewardPrimSkill"].String().empty())
		{
			auto raw = VLC->modh->identifiers.getIdentifier(CModHandler::scopeBuiltin(), "primSkill", json["rewardPrimSkill"].String());
			if(raw)
			{
				bonusType = PRIM_SKILL;
				bonusID = raw.value();
			}
		}
		else if(!json["rewardSkill"].String().empty())
		{
			auto raw = VLC->modh->identifiers.getIdentifier(CModHandler::scopeBuiltin(), "skill", json["rewardSkill"].String());
			if(raw)
			{
				bonusType = SECONDARY_SKILL;
				bonusID = raw.value();
			}
		}
		else if(!json["rewardSpell"].String().empty())
		{
			auto raw = VLC->modh->identifiers.getIdentifier(CModHandler::scopeBuiltin(), "spell", json["rewardSpell"].String());
			if(raw)
			{
				bonusType = SPELL;
				bonusID = raw.value();
			}
		}
	}
}

void CGGarrison::onHeroVisit (const CGHeroInstance *h) const
{
	int ally = cb->gameState()->getPlayerRelations(h->tempOwner, tempOwner);
	if (!ally && stacksCount() > 0) {
		//TODO: Find a way to apply magic garrison effects in battle.
		cb->startBattleI(h, this);
		return;
	}

	//New owner.
	if (!ally)
		cb->setOwner(this, h->tempOwner);

	cb->showGarrisonDialog(id, h->id, removableUnits);
}

bool CGGarrison::passableFor(PlayerColor player) const
{
	//FIXME: identical to same method in CGTownInstance

	if ( !stacksCount() )//empty - anyone can visit
		return true;
	if ( tempOwner == PlayerColor::NEUTRAL )//neutral guarded - no one can visit
		return false;

	if (cb->getPlayerRelations(tempOwner, player) != PlayerRelations::ENEMIES)
		return true;
	return false;
}

void CGGarrison::battleFinished(const CGHeroInstance *hero, const BattleResult &result) const
{
	if (result.winner == 0)
		onHeroVisit(hero);
}

void CGGarrison::serializeJsonOptions(JsonSerializeFormat& handler)
{
	handler.serializeBool("removableUnits", removableUnits);
	serializeJsonOwner(handler);
	CCreatureSet::serializeJson(handler, "army", 7);
}

void CGMagi::reset()
{
	eyelist.clear();
}

void CGMagi::initObj(CRandomGenerator & rand)
{
	if (ID == Obj::EYE_OF_MAGI)
	{
		blockVisit = true;
		eyelist[subID].push_back(id);
	}
}
void CGMagi::onHeroVisit(const CGHeroInstance * h) const
{
	if (ID == Obj::HUT_OF_MAGI)
	{
		h->showInfoDialog(61);

		if (!eyelist[subID].empty())
		{
			CenterView cv;
			cv.player = h->tempOwner;
			cv.focusTime = 2000;

			FoWChange fw;
			fw.player = h->tempOwner;
			fw.mode = 1;
			fw.waitForDialogs = true;

			for(const auto & it : eyelist[subID])
			{
				const CGObjectInstance *eye = cb->getObj(it);

				cb->getTilesInRange (fw.tiles, eye->pos, 10, h->tempOwner, 1);
				cb->sendAndApply(&fw);
				cv.pos = eye->pos;

				cb->sendAndApply(&cv);
			}
			cv.pos = h->visitablePos();
			cv.focusTime = 0;
			cb->sendAndApply(&cv);
		}
	}
	else if (ID == Obj::EYE_OF_MAGI)
	{
		h->showInfoDialog(48);
	}

}
void CGBoat::initObj(CRandomGenerator & rand)
{
	hero = nullptr;
}

int3 CGBoat::translatePos(const int3& pos, bool reverse /* = false */)
{
	//pos - offset we want to place the boat at the map
	//returned value - actual position as seen by game mechanics

	//If reverse = true, then it's the opposite.
	if (!reverse)
	{
		return pos + int3(1, 0, 0);
	}
	else
	{
		return pos - int3(1, 0, 0);
	}
}

void CGSirens::initObj(CRandomGenerator & rand)
{
	blockVisit = true;
}

std::string CGSirens::getHoverText(const CGHeroInstance * hero) const
{
	return getObjectName() + " " + visitedTxt(hero->hasBonusFrom(BonusSource::OBJECT,ID));
}

void CGSirens::onHeroVisit( const CGHeroInstance * h ) const
{
	InfoWindow iw;
	iw.player = h->tempOwner;
	if(h->hasBonusFrom(BonusSource::OBJECT,ID)) //has already visited Sirens
	{
		iw.type = EInfoWindowMode::AUTO;
		iw.text.addTxt(MetaString::ADVOB_TXT,133);
	}
	else
	{
		giveDummyBonus(h->id, BonusDuration::ONE_BATTLE);
		TExpType xp = 0;

		for (auto i = h->Slots().begin(); i != h->Slots().end(); i++)
		{
			// 1-sized stacks are not affected by sirens
			if (i->second->count == 1)
				continue;

			// tested H3 behavior: 30% (rounded up) of stack drowns
			TQuantity drown = std::ceil(i->second->count * 0.3);

			if(drown)
			{
				cb->changeStackCount(StackLocation(h, i->first), -drown);
				xp += drown * i->second->type->getMaxHealth();
			}
		}

		if(xp)
		{
			xp = h->calculateXp(static_cast<int>(xp));
			iw.text.addTxt(MetaString::ADVOB_TXT,132);
			iw.text.addReplacement(static_cast<int>(xp));
			cb->changePrimSkill(h, PrimarySkill::EXPERIENCE, xp, false);
		}
		else
		{
			iw.text.addTxt(MetaString::ADVOB_TXT,134);
		}
	}
	cb->showInfoDialog(&iw);

}

CGShipyard::CGShipyard()
	:IShipyard(this)
{
}

void CGShipyard::getOutOffsets( std::vector<int3> &offsets ) const
{
	// H J L K I
	// A x S x B
	// C E G F D
	offsets = {
		int3(-3,0,0), int3(1,0,0), //AB
		int3(-3,1,0), int3(1,1,0), int3(-2,1,0), int3(0,1,0), int3(-1,1,0), //CDEFG
		int3(-3,-1,0), int3(1,-1,0), int3(-2,-1,0), int3(0,-1,0), int3(-1,-1,0) //HIJKL
	};
}

void CGShipyard::onHeroVisit( const CGHeroInstance * h ) const
{
	if(!cb->gameState()->getPlayerRelations(tempOwner, h->tempOwner))
		cb->setOwner(this, h->tempOwner);

	auto s = shipyardStatus();
	if(s != IBoatGenerator::GOOD)
	{
		InfoWindow iw;
		iw.type = EInfoWindowMode::AUTO;
		iw.player = tempOwner;
		getProblemText(iw.text, h);
		cb->showInfoDialog(&iw);
	}
	else
	{
		openWindow(EOpenWindowMode::SHIPYARD_WINDOW,id.getNum(),h->id.getNum());
	}
}

void CGShipyard::serializeJsonOptions(JsonSerializeFormat& handler)
{
	serializeJsonOwner(handler);
}

void CCartographer::onHeroVisit( const CGHeroInstance * h ) const
{
	//if player has not bought map of this subtype yet and underground exist for stalagmite cartographer
	if (!wasVisited(h->getOwner()) && (subID != 2 || cb->gameState()->map->twoLevel))
	{
		if (cb->getResource(h->tempOwner, EGameResID::GOLD) >= 1000) //if he can afford a map
		{
			//ask if he wants to buy one
			int text=0;
			switch (subID)
			{
				case 0:
					text = 25;
					break;
				case 1:
					text = 26;
					break;
				case 2:
					text = 27;
					break;
				default:
					logGlobal->warn("Unrecognized subtype of cartographer");
			}
			assert(text);
			BlockingDialog bd (true, false);
			bd.player = h->getOwner();
			bd.text.addTxt (MetaString::ADVOB_TXT, text);
			cb->showBlockingDialog (&bd);
		}
		else //if he cannot afford
		{
			h->showInfoDialog(28);
		}
	}
	else //if he already visited carographer
	{
		h->showInfoDialog(24);
	}
}

void CCartographer::blockingDialogAnswered(const CGHeroInstance *hero, ui32 answer) const
{
	if(answer) //if hero wants to buy map
	{
		cb->giveResource(hero->tempOwner, EGameResID::GOLD, -1000);
		FoWChange fw;
		fw.mode = 1;
		fw.player = hero->tempOwner;

		//subIDs of different types of cartographers:
		//water = 0; land = 1; underground = 2;

		IGameCallback::MapTerrainFilterMode tileFilterMode = IGameCallback::MapTerrainFilterMode::NONE;

		switch(subID)
		{
			case 0:
				tileFilterMode = CPrivilegedInfoCallback::MapTerrainFilterMode::WATER;
				break;
			case 1:
				tileFilterMode = CPrivilegedInfoCallback::MapTerrainFilterMode::LAND_CARTOGRAPHER;
				break;
			case 2:
				tileFilterMode = CPrivilegedInfoCallback::MapTerrainFilterMode::UNDERGROUND_CARTOGRAPHER;
				break;
		}

		cb->getAllTiles(fw.tiles, hero->tempOwner, -1, tileFilterMode); //reveal appropriate tiles
		cb->sendAndApply(&fw);
		cb->setObjProperty(id, CCartographer::OBJPROP_VISITED, hero->tempOwner.getNum());
	}
}

void CGDenOfthieves::onHeroVisit (const CGHeroInstance * h) const
{
	cb->showThievesGuildWindow(h->tempOwner, id);
}

void CGObelisk::onHeroVisit( const CGHeroInstance * h ) const
{
	InfoWindow iw;
	iw.type = EInfoWindowMode::AUTO;
	iw.player = h->tempOwner;
	TeamState *ts = cb->gameState()->getPlayerTeam(h->tempOwner);
	assert(ts);
	TeamID team = ts->id;

	if(!wasVisited(team))
	{
		iw.text.addTxt(MetaString::ADVOB_TXT, 96);
		cb->sendAndApply(&iw);

		// increment general visited obelisks counter
		cb->setObjProperty(id, CGObelisk::OBJPROP_INC, team.getNum());

		openWindow(EOpenWindowMode::PUZZLE_MAP, h->tempOwner.getNum());

		// mark that particular obelisk as visited for all players in the team
		for(const auto & color : ts->players)
		{
			cb->setObjProperty(id, CGObelisk::OBJPROP_VISITED, color.getNum());
		}
	}
	else
	{
		iw.text.addTxt(MetaString::ADVOB_TXT, 97);
		cb->sendAndApply(&iw);
	}

}

void CGObelisk::initObj(CRandomGenerator & rand)
{
	obeliskCount++;
}

void CGObelisk::reset()
{
	obeliskCount = 0;
	visited.clear();
}

std::string CGObelisk::getHoverText(PlayerColor player) const
{
	return getObjectName() + " " + visitedTxt(wasVisited(player));
}

void CGObelisk::setPropertyDer( ui8 what, ui32 val )
{
	switch(what)
	{
		case CGObelisk::OBJPROP_INC:
			{
				auto progress = ++visited[TeamID(val)];
				logGlobal->debug("Player %d: obelisk progress %d / %d", val, static_cast<int>(progress) , static_cast<int>(obeliskCount));

				if(progress > obeliskCount)
				{
					logGlobal->error("Visited %d of %d", static_cast<int>(progress), obeliskCount);
					throw std::runtime_error("internal error");
				}

				break;
			}
		default:
			CTeamVisited::setPropertyDer(what, val);
			break;
	}
}

void CGLighthouse::onHeroVisit( const CGHeroInstance * h ) const
{
	if(h->tempOwner != tempOwner)
	{
		PlayerColor oldOwner = tempOwner;
		cb->setOwner(this,h->tempOwner); //not ours? flag it!
		h->showInfoDialog(69);
		giveBonusTo(h->tempOwner);

		if(oldOwner < PlayerColor::PLAYER_LIMIT) //remove bonus from old owner
		{
			RemoveBonus rb(GiveBonus::ETarget::PLAYER);
			rb.whoID = oldOwner.getNum();
			rb.source = vstd::to_underlying(BonusSource::OBJECT);
			rb.id = id.getNum();
			cb->sendAndApply(&rb);
		}
	}
}

void CGLighthouse::initObj(CRandomGenerator & rand)
{
	if(tempOwner < PlayerColor::PLAYER_LIMIT)
	{
		// FIXME: This is dirty hack
		giveBonusTo(tempOwner, true);
	}
}

void CGLighthouse::giveBonusTo(const PlayerColor & player, bool onInit) const
{
	GiveBonus gb(GiveBonus::ETarget::PLAYER);
	gb.bonus.type = BonusType::MOVEMENT;
	gb.bonus.val = 500;
	gb.id = player.getNum();
	gb.bonus.duration = BonusDuration::PERMANENT;
	gb.bonus.source = BonusSource::OBJECT;
	gb.bonus.sid = id.getNum();
	gb.bonus.subtype = 0;

	// FIXME: This is really dirty hack
	// Proper fix would be to make CGLighthouse into bonus system node
	// Unfortunately this will cause saves breakage
	if(onInit)
		gb.applyGs(cb->gameState());
	else
		cb->sendAndApply(&gb);
}

void CGLighthouse::serializeJsonOptions(JsonSerializeFormat& handler)
{
	serializeJsonOwner(handler);
}

VCMI_LIB_NAMESPACE_END
