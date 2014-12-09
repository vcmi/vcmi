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

#include "../NetPacks.h"
#include "../CGeneralTextHandler.h"
#include "../CSoundBase.h"
#include "../CModHandler.h"

#include "CObjectClassesHandler.h"
#include "../CSpellHandler.h"
#include "../IGameCallback.h"
#include "../CGameState.h"

std::map<Obj, std::map<int, std::vector<ObjectInstanceID> > > CGTeleport::objs;
std::vector<std::pair<ObjectInstanceID, ObjectInstanceID> > CGTeleport::gates;
std::map <si32, std::vector<ObjectInstanceID> > CGMagi::eyelist;
ui8 CGObelisk::obeliskCount; //how many obelisks are on map
std::map<TeamID, ui8> CGObelisk::visited; //map: team_id => how many obelisks has been visited

///helpers
static void openWindow(const OpenWindow::EWindow type, const int id1, const int id2 = -1)
{
	OpenWindow ow;
	ow.window = type;
	ow.id1 = id1;
	ow.id2 = id2;
	IObjectInterface::cb->sendAndApply(&ow);
}

static void showInfoDialog(const PlayerColor playerID, const ui32 txtID, const ui16 soundID)
{
	InfoWindow iw;
	iw.soundID = soundID;
	iw.player = playerID;
	iw.text.addTxt(MetaString::ADVOB_TXT,txtID);
	IObjectInterface::cb->sendAndApply(&iw);
}

static void showInfoDialog(const CGHeroInstance* h, const ui32 txtID, const ui16 soundID)
{
	const PlayerColor playerID = h->getOwner();
	showInfoDialog(playerID,txtID,soundID);
}

static std::string & visitedTxt(const bool visited)
{
	int id = visited ? 352 : 353;
	return VLC->generaltexth->allTexts[id];
}

void CPlayersVisited::setPropertyDer( ui8 what, ui32 val )
{
	if(what == 10)
		players.insert(PlayerColor(val));
}

bool CPlayersVisited::wasVisited( PlayerColor player ) const
{
	return vstd::contains(players,player);
}

bool CPlayersVisited::wasVisited( TeamID team ) const
{
	for(auto i : players)
	{
		if(cb->getPlayer(i)->team == team)
			return true;
	}
	return false;
}

std::string CGCreature::getHoverText(PlayerColor player) const
{
	if(stacks.empty())
	{
		//should not happen...
		logGlobal->errorStream() << "Invalid stack at tile " << pos << ": subID=" << subID << "; id=" << id;
		return "!!!INVALID_STACK!!!";
	}

	std::string hoverName;
	MetaString ms;
	int pom = stacks.begin()->second->getQuantityID();
	pom = 172 + 3*pom;
	ms.addTxt(MetaString::ARRAY_TXT,pom);
	ms << " " ;
	ms.addTxt(MetaString::CRE_PL_NAMES,subID);
	ms.toString(hoverName);
	return hoverName;
}

std::string CGCreature::getHoverText(const CGHeroInstance * hero) const
{
	std::string hoverName = getHoverText(hero->tempOwner);

	const JsonNode & texts = VLC->generaltexth->localizedTexts["adventureMap"]["monsterThreat"];

	hoverName += texts["title"].String();
	int choice;
	double ratio = ((double)getArmyStrength() / hero->getTotalStrength());
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
	hoverName += texts["levels"].Vector()[choice].String();
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
	case FLEE: //flee
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
			boost::algorithm::replace_first(tmp,"%d",boost::lexical_cast<std::string>(getStackCount(SlotID(0))));
			boost::algorithm::replace_first(tmp,"%d",boost::lexical_cast<std::string>(action));
			boost::algorithm::replace_first(tmp,"%s",VLC->creh->creatures[subID]->namePl);
			ynd.text << tmp;
			cb->showBlockingDialog(&ynd);
			break;
		}
	}

}

void CGCreature::initObj()
{
	blockVisit = true;
	switch(character)
	{
	case 0:
		character = -4;
		break;
	case 1:
		character = cb->gameState()->getRandomGenerator().nextInt(1, 7);
		break;
	case 2:
		character = cb->gameState()->getRandomGenerator().nextInt(1, 10);
		break;
	case 3:
		character = cb->gameState()->getRandomGenerator().nextInt(4, 10);
		break;
	case 4:
		character = 10;
		break;
	}

	stacks[SlotID(0)]->setType(CreatureID(subID));
	TQuantity &amount = stacks[SlotID(0)]->count;
	CCreature &c = *VLC->creh->creatures[subID];
	if(amount == 0)
	{
		amount = cb->gameState()->getRandomGenerator().nextInt(c.ammMin, c.ammMax);

		if(amount == 0) //armies with 0 creatures are illegal
		{
            logGlobal->warnStream() << "Problem: stack " << nodeName() << " cannot have 0 creatures. Check properties of " << c.nodeName();
			amount = 1;
		}
	}
	formation.randomFormation = cb->gameState()->getRandomGenerator().nextInt();

	temppower = stacks[SlotID(0)]->count * 1000;
	refusedJoining = false;
}

void CGCreature::newTurn() const
{//Works only for stacks of single type of size up to 2 millions
	if (stacks.begin()->second->count < VLC->modh->settings.CREEP_SIZE && cb->getDate(Date::DAY_OF_WEEK) == 1 && cb->getDate(Date::DAY) > 1)
	{
		ui32 power = temppower * (100 +  VLC->modh->settings.WEEKLY_GROWTH)/100;
		cb->setObjProperty(id, ObjProperty::MONSTER_COUNT, std::min (power/1000 , (ui32)VLC->modh->settings.CREEP_SIZE)); //set new amount
		cb->setObjProperty(id, ObjProperty::MONSTER_POWER, power); //increase temppower
	}
	if (VLC->modh->modules.STACK_EXP)
		cb->setObjProperty(id, ObjProperty::MONSTER_EXP, VLC->modh->settings.NEUTRAL_STACK_EXP); //for testing purpose
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
	double relStrength = double(h->getTotalStrength()) / getArmyStrength();

	int powerFactor;
	if(relStrength >= 7)
		powerFactor = 11;

	else if(relStrength >= 1)
		powerFactor = (int)(2*(relStrength-1));

	else if(relStrength >= 0.5)
		powerFactor = -1;

	else if(relStrength >= 0.333)
		powerFactor = -2;
	else
		powerFactor = -3;

	std::set<CreatureID> myKindCres; //what creatures are the same kind as we
	const CCreature * myCreature = VLC->creh->creatures[subID];
	myKindCres.insert(myCreature->idNumber); //we
	myKindCres.insert(myCreature->upgrades.begin(), myCreature->upgrades.end()); //our upgrades

	for(ConstTransitivePtr<CCreature> &crea : VLC->creh->creatures)
	{
		if(vstd::contains(crea->upgrades, myCreature->idNumber)) //it's our base creatures
			myKindCres.insert(crea->idNumber);
	}

	int count = 0, //how many creatures of similar kind has hero
		totalCount = 0;

	for (auto & elem : h->Slots())
	{
		if(vstd::contains(myKindCres,elem.second->type->idNumber))
			count += elem.second->count;
		totalCount += elem.second->count;
	}

	int sympathy = 0; // 0 if hero have no similar creatures
	if(count)
		sympathy++; // 1 - if hero have at least 1 similar creature
	if(count*2 > totalCount)
		sympathy++; // 2 - hero have similar creatures more that 50%

	int charisma = powerFactor + h->getSecSkillLevel(SecondarySkill::DIPLOMACY) + sympathy;

	if(charisma < character) //creatures will fight
		return -2;

	if (allowJoin)
	{
		if(h->getSecSkillLevel(SecondarySkill::DIPLOMACY) + sympathy + 1 >= character)
			return 0; //join for free

		else if(h->getSecSkillLevel(SecondarySkill::DIPLOMACY) * 2  +  sympathy  +  1 >= character)
			return VLC->creh->creatures[subID]->cost[6] * getStackCount(SlotID(0)); //join for gold
	}

	//we are still here - creatures have not joined hero, flee or fight

	if (charisma > character)
		return -1; //flee
	else
		return -2; //fight
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
		if(takenAction(h,false) == -1) //they flee
		{
			cb->setObjProperty(id, ObjProperty::MONSTER_REFUSED_JOIN, true);
			flee(h);
		}
		else //they fight
		{
			showInfoDialog(h,87,0);//Insulted by your refusal of their offer, the monsters attack!
			fight(h);
		}
	}
	else //accepted
	{
		if (cb->getResource(h->tempOwner, Res::GOLD) < cost) //player don't have enough gold!
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
			cb->giveResource(h->tempOwner,Res::GOLD,-cost);

		cb->tryJoiningArmy(this, h, true, true);
	}
}

void CGCreature::fight( const CGHeroInstance *h ) const
{
	//split stacks
	//TODO: multiple creature types in a stack?
	int basicType = stacks.begin()->second->type->idNumber;
	cb->setObjProperty(id, ObjProperty::MONSTER_RESTORE_TYPE, basicType); //store info about creature stack

	double relativePower = static_cast<double>(h->getTotalStrength()) / getArmyStrength();
	int stacksCount;
	//TODO: number depends on tile type
	if (relativePower < 0.5)
	{
		stacksCount = 7;
	}
	else if (relativePower < 0.67)
	{
		stacksCount = 7;
	}
	else if (relativePower < 1)
	{
		stacksCount = 6;
	}
	else if (relativePower < 1.5)
	{
		stacksCount = 5;
	}
	else if (relativePower < 2)
	{
		stacksCount = 4;
	}
	else
	{
		stacksCount = 3;
	}
	SlotID sourceSlot = stacks.begin()->first;
	SlotID destSlot;
	for (int stacksLeft = stacksCount; stacksLeft > 1; --stacksLeft)
	{
		int stackSize = stacks.begin()->second->count / stacksLeft;
		if (stackSize)
		{
			if ((destSlot = getFreeSlot()).validSlot())
				cb->moveStack(StackLocation(this, sourceSlot), StackLocation(this, destSlot), stackSize);
			else
			{
                logGlobal->warnStream() <<"Warning! Not enough empty slots to split stack!";
				break;
			}
		}
		else break;
	}
	if (stacksCount > 1)
	{
		if (formation.randomFormation % 100 < 50) //upgrade
		{
			SlotID slotId = SlotID(stacks.size() / 2);
			const auto & upgrades = getStack(slotId).type->upgrades;
			if(!upgrades.empty())
			{
				auto it = RandomGeneratorUtil::nextItem(upgrades, cb->gameState()->getRandomGenerator());
				cb->changeStackType(StackLocation(this, slotId), VLC->creh->creatures[*it]);
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

	if(result.winner==0)
	{
		cb->removeObject(this);
	}
	else
	{
		//merge stacks into one
		TSlots::const_iterator i;
		CCreature * cre = VLC->creh->creatures[formation.basicType];
		for (i = stacks.begin(); i != stacks.end(); i++)
		{
			if (cre->isMyUpgrade(i->second->type))
			{
				cb->changeStackType (StackLocation(this, i->first), cre); //un-upgrade creatures
			}
		}

		//first stack has to be at slot 0 -> if original one got killed, move there first remaining stack
		if(!hasStackAtSlot(SlotID(0)))
			cb->moveStack(StackLocation(this, stacks.begin()->first), StackLocation(this, SlotID(0)), stacks.begin()->second->count);

		while (stacks.size() > 1) //hopefully that's enough
		{
			// TODO it's either overcomplicated (if we assume there'll be only one stack) or buggy (if we allow multiple stacks... but that'll also cause troubles elsewhere)
			i = stacks.end();
			i--;
			SlotID slot = getSlotFor(i->second->type);
			if (slot == i->first) //no reason to move stack to its own slot
				break;
			else
				cb->moveStack (StackLocation(this, i->first), StackLocation(this, slot), i->second->count);
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

void CGMine::newTurn() const
{
	if(cb->getDate() == 1)
		return;

	if (tempOwner == PlayerColor::NEUTRAL)
		return;

	cb->giveResource(tempOwner, producedResource, producedQuantity);
}

void CGMine::initObj()
{
	if(subID >= 7) //Abandoned Mine
	{
		//set guardians
		int howManyTroglodytes = cb->gameState()->getRandomGenerator().nextInt(100, 199);
		auto troglodytes = new CStackInstance(CreatureID::TROGLODYTES, howManyTroglodytes);
		putStack(SlotID(0), troglodytes);

		//after map reading tempOwner placeholds bitmask for allowed resources
		std::vector<Res::ERes> possibleResources;
		for (int i = 0; i < PlayerColor::PLAYER_LIMIT_I; i++)
			if(tempOwner.getNum() & 1<<i) //NOTE: reuse of tempOwner
				possibleResources.push_back(static_cast<Res::ERes>(i));

		assert(!possibleResources.empty());
		producedResource = *RandomGeneratorUtil::nextItem(possibleResources, cb->gameState()->getRandomGenerator());
		tempOwner = PlayerColor::NEUTRAL;
	}
	else
	{
		producedResource = static_cast<Res::ERes>(subID);
		if(tempOwner >= PlayerColor::PLAYER_LIMIT)
			tempOwner = PlayerColor::NEUTRAL;
	}

	producedQuantity = defaultResProduction();
}

std::string CGMine::getObjectName() const
{
	return VLC->generaltexth->mines.at(subID).first;
}

std::string CGMine::getHoverText(PlayerColor player) const
{

	std::string hoverName = getObjectName(); // Sawmill

	if (tempOwner != PlayerColor::NEUTRAL)
	{
		hoverName += "\n";
		hoverName += VLC->generaltexth->arraytxt[23 + tempOwner.getNum()]; // owned by Red Player
		hoverName += "\n(" + VLC->generaltexth->restypes[producedResource] + ")";
	}

	for (auto & slot : Slots()) // guarded by a few Pikeman
	{
		hoverName += "\n";
		hoverName += getRoughAmount(slot.first);
		hoverName += getCreature(slot.first)->namePl;
	}
	return hoverName;
}

void CGMine::flagMine(PlayerColor player) const
{
	assert(tempOwner != player);
	cb->setOwner(this, player); //not ours? flag it!

	InfoWindow iw;
	iw.soundID = soundBase::FLAGMINE;
	iw.text.addTxt(MetaString::MINE_EVNTS,producedResource); //not use subID, abandoned mines uses default mine texts
	iw.player = player;
	iw.components.push_back(Component(Component::RESOURCE,producedResource,producedQuantity,-1));
	cb->showInfoDialog(&iw);
}

ui32 CGMine::defaultResProduction()
{
	switch(producedResource)
	{
	case Res::WOOD:
	case Res::ORE:
		return 2;
	case Res::GOLD:
		return 1000;
	default:
		return 1;
	}
}

void CGMine::battleFinished(const CGHeroInstance *hero, const BattleResult &result) const
{
	if(result.winner == 0) //attacker won
	{
		if(subID == 7)
		{
			showInfoDialog(hero->tempOwner, 85, 0);
		}
		flagMine(hero->tempOwner);
	}
}

void CGMine::blockingDialogAnswered(const CGHeroInstance *hero, ui32 answer) const
{
	if(answer)
		cb->startBattleI(hero, this);
}

std::string CGResource::getHoverText(PlayerColor player) const
{
	return VLC->generaltexth->restypes[subID];
}

CGResource::CGResource()
{
	amount = 0;
}

void CGResource::initObj()
{
	blockVisit = true;

	if(!amount)
	{
		switch(subID)
		{
		case 6:
			amount = cb->gameState()->getRandomGenerator().nextInt(500, 1000);
			break;
		case 0: case 2:
			amount = cb->gameState()->getRandomGenerator().nextInt(6, 10);
			break;
		default:
			amount = cb->gameState()->getRandomGenerator().nextInt(3, 5);
			break;
		}
	}
}

void CGResource::onHeroVisit( const CGHeroInstance * h ) const
{
	if(stacksCount())
	{
		if(message.size())
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
	{
		if(message.length())
		{
			InfoWindow iw;
			iw.player = h->tempOwner;
			iw.text << message;
			cb->showInfoDialog(&iw);
		}
		collectRes(h->getOwner());
	}
}

void CGResource::collectRes( PlayerColor player ) const
{
	cb->giveResource(player, static_cast<Res::ERes>(subID), amount);
	ShowInInfobox sii;
	sii.player = player;
	sii.c = Component(Component::RESOURCE,subID,amount,0);
	sii.text.addTxt(MetaString::ADVOB_TXT,113);
	sii.text.addReplacement(MetaString::RES_NAMES, subID);
	cb->showCompInfo(&sii);
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

void CGTeleport::onHeroVisit( const CGHeroInstance * h ) const
{
	ObjectInstanceID destinationid;
	switch(ID)
	{
	case Obj::MONOLITH_ONE_WAY_ENTRANCE: //one way - find corresponding exit monolith
	{
		if(vstd::contains(objs,Obj::MONOLITH_ONE_WAY_EXIT) && vstd::contains(objs[Obj::MONOLITH_ONE_WAY_EXIT],subID) && objs[Obj::MONOLITH_ONE_WAY_EXIT][subID].size())
		{
			destinationid = *RandomGeneratorUtil::nextItem(objs[Obj::MONOLITH_ONE_WAY_EXIT][subID], cb->gameState()->getRandomGenerator());
		}
		else
		{
			logGlobal->warnStream() << "Cannot find corresponding exit monolith for "<< id;
		}
		break;
	}
	case Obj::MONOLITH_TWO_WAY://two way monolith - pick any other one
	case Obj::WHIRLPOOL: //Whirlpool
		if(vstd::contains(objs,ID) && vstd::contains(objs[ID],subID) && objs[ID][subID].size()>1)
		{
			//choose another exit
			do
			{
				destinationid = *RandomGeneratorUtil::nextItem(objs[ID][subID], cb->gameState()->getRandomGenerator());
			} while(destinationid == id);

			if (ID == Obj::WHIRLPOOL)
			{
				if (!h->hasBonusOfType(Bonus::WHIRLPOOL_PROTECTION))
				{
					if (h->Slots().size() > 1 || h->Slots().begin()->second->count > 1)
					{ //we can't remove last unit
						SlotID targetstack = h->Slots().begin()->first; //slot numbers may vary
						for(auto i = h->Slots().rbegin(); i != h->Slots().rend(); i++)
						{
							if (h->getPower(targetstack) > h->getPower(i->first))
							{
								targetstack = (i->first);
							}
						}

						TQuantity countToTake = h->getStackCount(targetstack) * 0.5;
						vstd::amax(countToTake, 1);


						InfoWindow iw;
						iw.player = h->tempOwner;
						iw.text.addTxt (MetaString::ADVOB_TXT, 168);
						iw.components.push_back (Component(CStackBasicDescriptor(h->getCreature(targetstack), countToTake)));
						cb->showInfoDialog(&iw);
						cb->changeStackCount(StackLocation(h, targetstack), -countToTake);
					}
				}
			}
		}
		else
			logGlobal->warnStream() << "Cannot find corresponding exit monolith for "<< id;
		break;
	case Obj::SUBTERRANEAN_GATE: //find nearest subterranean gate on the other level
		{
			destinationid = getMatchingGate(id);
			if(destinationid == ObjectInstanceID()) //no exit
			{
				showInfoDialog(h,153,0);//Just inside the entrance you find a large pile of rubble blocking the tunnel. You leave discouraged.
			}
			break;
		}
	}
	if(destinationid == ObjectInstanceID())
	{
		logGlobal->warnStream() << "Cannot find exit... (obj at " << pos << ") :( ";
		return;
	}
	if (ID == Obj::WHIRLPOOL)
	{
		std::set<int3> tiles = cb->getObj(destinationid)->getBlockedPos();
		auto & tile = *RandomGeneratorUtil::nextItem(tiles, cb->gameState()->getRandomGenerator());
		cb->moveHero(h->id, tile + int3(1,0,0), true);
	}
	else
		cb->moveHero (h->id,CGHeroInstance::convertPosition(cb->getObj(destinationid)->pos,true) - getVisitableOffset(), true);
}

void CGTeleport::initObj()
{
	int si = subID;
	switch (ID)
	{
	case Obj::SUBTERRANEAN_GATE://ignore subterranean gates subid
	case Obj::WHIRLPOOL:
		{
			si = 0;
			break;
		}
	default:
		break;
	}
	objs[ID][si].push_back(id);
}

void CGTeleport::postInit() //matches subterranean gates into pairs
{
	//split on underground and surface gates
	std::vector<const CGObjectInstance *> gatesSplit[2]; //surface and underground gates
	for(auto & elem : objs[Obj::SUBTERRANEAN_GATE][0])
	{
		const CGObjectInstance *hlp = cb->getObj(elem);
		gatesSplit[hlp->pos.z].push_back(hlp);
	}

	//sort by position
	std::sort(gatesSplit[0].begin(), gatesSplit[0].end(), [](const CGObjectInstance * a, const CGObjectInstance * b)
	{
		return a->pos < b->pos;
	});

	for(size_t i = 0; i < gatesSplit[0].size(); i++)
	{
		const CGObjectInstance *cur = gatesSplit[0][i];

		//find nearest underground exit
		std::pair<int, si32> best(-1, std::numeric_limits<si32>::max()); //pair<pos_in_vector, distance^2>
		for(int j = 0; j < gatesSplit[1].size(); j++)
		{
			const CGObjectInstance *checked = gatesSplit[1][j];
			if(!checked)
				continue;
			si32 hlp = checked->pos.dist2dSQ(cur->pos);
			if(hlp < best.second)
			{
				best.first = j;
				best.second = hlp;
			}
		}

		if(best.first >= 0) //found pair
		{
			gates.push_back(std::make_pair(cur->id, gatesSplit[1][best.first]->id));
			gatesSplit[1][best.first] = nullptr;
		}
		else
			gates.push_back(std::make_pair(cur->id, ObjectInstanceID()));
	}
	objs.erase(Obj::SUBTERRANEAN_GATE);
}

ObjectInstanceID CGTeleport::getMatchingGate(ObjectInstanceID id)
{
	for(auto & gate : gates)
	{
		if(gate.first == id)
			return gate.second;
		if(gate.second == id)
			return gate.first;
	}

	return ObjectInstanceID();
}

void CGArtifact::initObj()
{
	blockVisit = true;
	if(ID == Obj::ARTIFACT)
	{
		if (!storedArtifact)
		{
			auto a = new CArtifactInstance();
			cb->gameState()->map->addNewArtifactInstance(a);
			storedArtifact = a;
		}
		if(!storedArtifact->artType)
			storedArtifact->setType(VLC->arth->artifacts[subID]);
	}
	if(ID == Obj::SPELL_SCROLL)
		subID = 1;

	assert(storedArtifact->artType);
	assert(storedArtifact->getParentNodes().size());

	//assert(storedArtifact->artType->id == subID); //this does not stop desync
}

std::string CGArtifact::getObjectName() const
{
	return VLC->arth->artifacts[subID]->Name();
}

void CGArtifact::onHeroVisit( const CGHeroInstance * h ) const
{
	if(!stacksCount())
	{
		InfoWindow iw;
		iw.player = h->tempOwner;
		switch(ID)
		{
		case Obj::ARTIFACT:
			{
				iw.soundID = soundBase::treasure; //play sound only for non-scroll arts
				iw.components.push_back(Component(Component::ARTIFACT,subID,0,0));
				if(message.length())
					iw.text <<  message;
				else
				{
					if (VLC->arth->artifacts[subID]->EventText().size())
						iw.text << std::pair<ui8, ui32> (MetaString::ART_EVNTS, subID);
					else //fix for mod artifacts with no event text
					{
						iw.text.addTxt (MetaString::ADVOB_TXT, 183); //% has found treasure
						iw.text.addReplacement (h->name);
					}

				}
			}
			break;
		case Obj::SPELL_SCROLL:
			{
				int spellID = storedArtifact->getGivenSpellID();
				iw.components.push_back (Component(Component::SPELL, spellID,0,0));
				iw.text.addTxt (MetaString::ADVOB_TXT,135);
				iw.text.addReplacement(MetaString::SPELL_NAME, spellID);
			}
			break;
		}
		cb->showInfoDialog(&iw);
		pick(h);
	}
	else
	{
		if(message.size())
		{
			BlockingDialog ynd(true,false);
			ynd.player = h->getOwner();
			ynd.text << message;
			cb->showBlockingDialog(&ynd);
		}
		else
		{
			blockingDialogAnswered(h, true);
		}
	}
}

void CGArtifact::pick(const CGHeroInstance * h) const
{
	cb->giveHeroArtifact(h, storedArtifact, ArtifactPosition::FIRST_AVAILABLE);
	cb->removeObject(this);
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

void CGWitchHut::initObj()
{
	if (allowedAbilities.empty()) //this can happen for RMG. regular maps load abilities from map file
	{
		for (int i = 0; i < GameConstants::SKILL_QUANTITY; i++)
			allowedAbilities.push_back(i);
	}
	ability = *RandomGeneratorUtil::nextItem(allowedAbilities, cb->gameState()->getRandomGenerator());
}

void CGWitchHut::onHeroVisit( const CGHeroInstance * h ) const
{
	InfoWindow iw;
	iw.soundID = soundBase::gazebo;
	iw.player = h->getOwner();
	if(!wasVisited(h->tempOwner))
		cb->setObjProperty(id, 10, h->tempOwner.getNum());
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
		iw.components.push_back(Component(Component::SEC_SKILL, ability, 1, 0));
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
		boost::algorithm::replace_first(hoverName,"%s",VLC->generaltexth->skillName[ability]);
	}
	return hoverName;
}

std::string CGWitchHut::getHoverText(const CGHeroInstance * hero) const
{
	std::string hoverName = getHoverText(hero->tempOwner);
	if(hero->getSecSkillLevel(SecondarySkill(ability))) //hero knows that ability
		hoverName += "\n\n" + VLC->generaltexth->allTexts[357]; // (Already learned)
	return hoverName;
}

void CGMagicWell::onHeroVisit( const CGHeroInstance * h ) const
{
	int message;

	if(h->hasBonusFrom(Bonus::OBJECT,ID)) //has already visited Well today
	{
		message = 78;//"A second drink at the well in one day will not help you."
	}
	else if(h->mana < h->manaLimit())
	{
		giveDummyBonus(h->id);
		cb->setManaPoints(h->id,h->manaLimit());
		message = 77;
	}
	else
	{
		message = 79;
	}
	showInfoDialog(h,message,soundBase::faerie);
}

std::string CGMagicWell::getHoverText(const CGHeroInstance * hero) const
{
	return getObjectName() + " " + visitedTxt(hero->hasBonusFrom(Bonus::OBJECT,ID));
}

void CGObservatory::onHeroVisit( const CGHeroInstance * h ) const
{
	InfoWindow iw;
	iw.player = h->tempOwner;
	switch (ID)
	{
	case Obj::REDWOOD_OBSERVATORY:
	case Obj::PILLAR_OF_FIRE:
	{
		iw.soundID = soundBase::LIGHTHOUSE;
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
        logGlobal->errorStream() << "Not initialized shrine visited!";
		return;
	}

	if(!wasVisited(h->tempOwner))
		cb->setObjProperty(id, 10, h->tempOwner.getNum());

	InfoWindow iw;
	iw.soundID = soundBase::temple;
	iw.player = h->getOwner();
	iw.text.addTxt(MetaString::ADVOB_TXT,127 + ID - 88);
	iw.text.addTxt(MetaString::SPELL_NAME,spell);
	iw.text << ".";

	if(!h->getArt(ArtifactPosition::SPELLBOOK))
	{
		iw.text.addTxt(MetaString::ADVOB_TXT,131);
	}
	else if(ID == Obj::SHRINE_OF_MAGIC_THOUGHT  && !h->getSecSkillLevel(SecondarySkill::WISDOM)) //it's third level spell and hero doesn't have wisdom
	{
		iw.text.addTxt(MetaString::ADVOB_TXT,130);
	}
	else if(vstd::contains(h->spells,spell))//hero already knows the spell
	{
		iw.text.addTxt(MetaString::ADVOB_TXT,174);
	}
	else //give spell
	{
		std::set<SpellID> spells;
		spells.insert(spell);
		cb->changeSpells(h, true, spells);

		iw.components.push_back(Component(Component::SPELL,spell,0,0));
	}

	cb->showInfoDialog(&iw);
}

void CGShrine::initObj()
{
	if(spell == SpellID::NONE) //spell not set
	{
		int level = ID-87;
		std::vector<SpellID> possibilities;
		cb->getAllowedSpells (possibilities, level);

		if(possibilities.empty())
		{
            logGlobal->errorStream() << "Error: cannot init shrine, no allowed spells!";
			return;
		}

		spell = *RandomGeneratorUtil::nextItem(possibilities, cb->gameState()->getRandomGenerator());
	}
}

std::string CGShrine::getHoverText(PlayerColor player) const
{
	std::string hoverName = getObjectName();
	if(wasVisited(player))
	{
		hoverName += "\n" + VLC->generaltexth->allTexts[355]; // + (learn %s)
		boost::algorithm::replace_first(hoverName,"%s", spell.toSpell()->name);
	}
	return hoverName;
}

std::string CGShrine::getHoverText(const CGHeroInstance * hero) const
{
	std::string hoverName = getHoverText(hero->tempOwner);
	if(vstd::contains(hero->spells, spell)) //hero knows that spell
		hoverName += "\n\n" + VLC->generaltexth->allTexts[354]; // (Already learned)
	return hoverName;
}

void CGSignBottle::initObj()
{
	//if no text is set than we pick random from the predefined ones
	if(message.empty())
	{
		message = *RandomGeneratorUtil::nextItem(VLC->generaltexth->randsign, cb->gameState()->getRandomGenerator());
	}

	if(ID == Obj::OCEAN_BOTTLE)
	{
		blockVisit = true;
	}
}

void CGSignBottle::onHeroVisit( const CGHeroInstance * h ) const
{
	InfoWindow iw;
	iw.soundID = soundBase::STORE;
	iw.player = h->getOwner();
	iw.text << message;
	cb->showInfoDialog(&iw);

	if(ID == Obj::OCEAN_BOTTLE)
		cb->removeObject(this);
}

//TODO: remove
//void CGScholar::giveAnyBonus( const CGHeroInstance * h ) const
//{
//
//}

void CGScholar::onHeroVisit( const CGHeroInstance * h ) const
{

	EBonusType type = bonusType;
	int bid = bonusID;
	//check if the bonus if applicable, if not - give primary skill (always possible)
	int ssl = h->getSecSkillLevel(SecondarySkill(bid)); //current sec skill level, used if bonusType == 1
	if((type == SECONDARY_SKILL
			&& ((ssl == 3)  ||  (!ssl  &&  !h->canLearnSkill()))) ////hero already has expert level in the skill or (don't know skill and doesn't have free slot)
		|| (type == SPELL  &&  (!h->getArt(ArtifactPosition::SPELLBOOK) || vstd::contains(h->spells, (ui32) bid)
		|| ( SpellID(bid).toSpell()->level > h->getSecSkillLevel(SecondarySkill::WISDOM) + 2)
		))) //hero doesn't have a spellbook or already knows the spell or doesn't have Wisdom
	{
		type = PRIM_SKILL;
		bid = cb->gameState()->getRandomGenerator().nextInt(GameConstants::PRIMARY_SKILLS - 1);
	}

	InfoWindow iw;
	iw.soundID = soundBase::gazebo;
	iw.player = h->getOwner();
	iw.text.addTxt(MetaString::ADVOB_TXT,115);

	switch (type)
	{
	case PRIM_SKILL:
		cb->changePrimSkill(h,static_cast<PrimarySkill::PrimarySkill>(bid),+1);
		iw.components.push_back(Component(Component::PRIM_SKILL,bid,+1,0));
		break;
	case SECONDARY_SKILL:
		cb->changeSecSkill(h,SecondarySkill(bid),+1);
		iw.components.push_back(Component(Component::SEC_SKILL,bid,ssl+1,0));
		break;
	case SPELL:
		{
			std::set<SpellID> hlp;
			hlp.insert(SpellID(bid));
			cb->changeSpells(h,true,hlp);
			iw.components.push_back(Component(Component::SPELL,bid,0,0));
		}
		break;
	default:
		logGlobal->errorStream() << "Error: wrong bonus type (" << (int)type << ") for Scholar!\n";
		return;
	}

	cb->showInfoDialog(&iw);
	cb->removeObject(this);
}

void CGScholar::initObj()
{
	blockVisit = true;
	if(bonusType == RANDOM)
	{
		bonusType = static_cast<EBonusType>(cb->gameState()->getRandomGenerator().nextInt(2));
		switch(bonusType)
		{
		case PRIM_SKILL:
			bonusID = cb->gameState()->getRandomGenerator().nextInt(GameConstants::PRIMARY_SKILLS -1);
			break;
		case SECONDARY_SKILL:
			bonusID = cb->gameState()->getRandomGenerator().nextInt(GameConstants::SKILL_QUANTITY -1);
			break;
		case SPELL:
			std::vector<SpellID> possibilities;
			for (int i = 1; i < 6; ++i)
				cb->getAllowedSpells (possibilities, i);
			bonusID = *RandomGeneratorUtil::nextItem(possibilities, cb->gameState()->getRandomGenerator());
			break;
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

void CGMagi::initObj()
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
		showInfoDialog(h, 61, soundBase::LIGHTHOUSE);

		if (!eyelist[subID].empty())
		{
			CenterView cv;
			cv.player = h->tempOwner;
			cv.focusTime = 2000;

			FoWChange fw;
			fw.player = h->tempOwner;
			fw.mode = 1;
			fw.waitForDialogs = true;

			for(auto it : eyelist[subID])
			{
				const CGObjectInstance *eye = cb->getObj(it);

				cb->getTilesInRange (fw.tiles, eye->pos, 10, h->tempOwner, 1);
				cb->sendAndApply(&fw);
				cv.pos = eye->pos;

				cb->sendAndApply(&cv);
			}
			cv.pos = h->getPosition(false);
			cb->sendAndApply(&cv);
		}
	}
	else if (ID == Obj::EYE_OF_MAGI)
	{
		showInfoDialog(h,48,soundBase::invalid);
	}

}
void CGBoat::initObj()
{
	hero = nullptr;
}

void CGSirens::initObj()
{
	blockVisit = true;
}

std::string CGSirens::getHoverText(const CGHeroInstance * hero) const
{
	return getObjectName() + " " + visitedTxt(hero->hasBonusFrom(Bonus::OBJECT,ID));
}

void CGSirens::onHeroVisit( const CGHeroInstance * h ) const
{
	InfoWindow iw;
	iw.soundID = soundBase::DANGER;
	iw.player = h->tempOwner;
	if(h->hasBonusFrom(Bonus::OBJECT,ID)) //has already visited Sirens
	{
		iw.text.addTxt(MetaString::ADVOB_TXT,133);
	}
	else
	{
		giveDummyBonus(h->id, Bonus::ONE_BATTLE);
		TExpType xp = 0;

		for (auto i = h->Slots().begin(); i != h->Slots().end(); i++)
		{
			TQuantity drown = i->second->count * 0.3;
			if(drown)
			{
				cb->changeStackCount(StackLocation(h, i->first), -drown);
				xp += drown * i->second->type->valOfBonuses(Bonus::STACK_HEALTH);
			}
		}

		if(xp)
		{
			xp = h->calculateXp(xp);
			iw.text.addTxt(MetaString::ADVOB_TXT,132);
			iw.text.addReplacement(xp);
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
		iw.player = tempOwner;
		getProblemText(iw.text, h);
		cb->showInfoDialog(&iw);
	}
	else
	{
		openWindow(OpenWindow::SHIPYARD_WINDOW,id.getNum(),h->id.getNum());
	}
}

void CCartographer::onHeroVisit( const CGHeroInstance * h ) const
{
	//if player has not bought map of this subtype yet and underground exist for stalagmite cartographer
	if (!wasVisited(h->getOwner()) && (subID != 2 || cb->gameState()->map->twoLevel))
	{
		if (cb->getResource(h->tempOwner, Res::GOLD) >= 1000) //if he can afford a map
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
                    logGlobal->warnStream() << "Unrecognized subtype of cartographer";
			}
			assert(text);
			BlockingDialog bd (true, false);
			bd.player = h->getOwner();
			bd.soundID = soundBase::LIGHTHOUSE;
			bd.text.addTxt (MetaString::ADVOB_TXT, text);
			cb->showBlockingDialog (&bd);
		}
		else //if he cannot afford
		{
			showInfoDialog(h,28,soundBase::CAVEHEAD);
		}
	}
	else //if he already visited carographer
	{
		showInfoDialog(h,24,soundBase::CAVEHEAD);
	}
}

void CCartographer::blockingDialogAnswered(const CGHeroInstance *hero, ui32 answer) const
{
	if (answer) //if hero wants to buy map
	{
		cb->giveResource (hero->tempOwner, Res::GOLD, -1000);
		FoWChange fw;
		fw.mode = 1;
		fw.player = hero->tempOwner;

		//subIDs of different types of cartographers:
		//water = 0; land = 1; underground = 2;
		cb->getAllTiles (fw.tiles, hero->tempOwner, subID - 1, !subID + 1); //reveal appropriate tiles
		cb->sendAndApply (&fw);
		cb->setObjProperty (id, 10, hero->tempOwner.getNum());
	}
}

void CGDenOfthieves::onHeroVisit (const CGHeroInstance * h) const
{
	cb->showThievesGuildWindow(h->tempOwner, id);
}

void CGObelisk::onHeroVisit( const CGHeroInstance * h ) const
{
	InfoWindow iw;
	iw.player = h->tempOwner;
	TeamState *ts = cb->gameState()->getPlayerTeam(h->tempOwner);
	assert(ts);
	TeamID team = ts->id;

	if(!wasVisited(team))
	{
		iw.text.addTxt(MetaString::ADVOB_TXT, 96);
		cb->sendAndApply(&iw);

		cb->setObjProperty(id, 20, h->tempOwner.getNum()); //increment general visited obelisks counter

		openWindow(OpenWindow::PUZZLE_MAP, h->tempOwner.getNum());

		cb->setObjProperty(id, 10, h->tempOwner.getNum()); //mark that particular obelisk as visited
	}
	else
	{
		iw.text.addTxt(MetaString::ADVOB_TXT, 97);
		cb->sendAndApply(&iw);
	}

}

void CGObelisk::initObj()
{
	obeliskCount++;
}

std::string CGObelisk::getHoverText(PlayerColor player) const
{
	return getObjectName() + " " + visitedTxt(wasVisited(player));
}

void CGObelisk::setPropertyDer( ui8 what, ui32 val )
{
	CPlayersVisited::setPropertyDer(what, val);
	switch(what)
	{
	case 20:
		assert(val < PlayerColor::PLAYER_LIMIT_I);
		visited[TeamID(val)]++;

		if(visited[TeamID(val)] > obeliskCount)
		{
            logGlobal->errorStream() << "Error: Visited " << visited[TeamID(val)] << "\t\t" << obeliskCount;
			assert(0);
		}

		break;
	}
}

void CGLighthouse::onHeroVisit( const CGHeroInstance * h ) const
{
	if(h->tempOwner != tempOwner)
	{
		PlayerColor oldOwner = tempOwner;
		cb->setOwner(this,h->tempOwner); //not ours? flag it!
		showInfoDialog(h,69,soundBase::LIGHTHOUSE);
		giveBonusTo(h->tempOwner);

		if(oldOwner < PlayerColor::PLAYER_LIMIT) //remove bonus from old owner
		{
			RemoveBonus rb(RemoveBonus::PLAYER);
			rb.whoID = oldOwner.getNum();
			rb.source = Bonus::OBJECT;
			rb.id = id.getNum();
			cb->sendAndApply(&rb);
		}
	}
}

void CGLighthouse::initObj()
{
	if(tempOwner < PlayerColor::PLAYER_LIMIT)
	{
		giveBonusTo(tempOwner);
	}
}

std::string CGLighthouse::getHoverText(PlayerColor player) const
{
	//TODO: owned by %s player
	return getObjectName();
}

void CGLighthouse::giveBonusTo( PlayerColor player ) const
{
	GiveBonus gb(GiveBonus::PLAYER);
	gb.bonus.type = Bonus::SEA_MOVEMENT;
	gb.bonus.val = 500;
	gb.id = player.getNum();
	gb.bonus.duration = Bonus::PERMANENT;
	gb.bonus.source = Bonus::OBJECT;
	gb.bonus.sid = id.getNum();
	cb->sendAndApply(&gb);
}
