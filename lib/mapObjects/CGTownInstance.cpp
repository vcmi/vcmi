/*
 * CGTownInstance.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "CGTownInstance.h"
#include "CObjectClassesHandler.h"

#include "../NetPacks.h"
#include "../CGeneralTextHandler.h"
#include "../CModHandler.h"
#include "../IGameCallback.h"
#include "../CGameState.h"
#include "../mapping/CMapDefines.h"
#include "../CPlayerState.h"

std::vector<const CArtifact *> CGTownInstance::merchantArtifacts;
std::vector<int> CGTownInstance::universitySkills;

void CGDwelling::initObj()
{
	switch(ID)
	{
	case Obj::CREATURE_GENERATOR1:
	case Obj::CREATURE_GENERATOR4:
		{
			VLC->objtypeh->getHandlerFor(ID, subID)->configureObject(this, cb->gameState()->getRandomGenerator());

			if (getOwner() != PlayerColor::NEUTRAL)
				cb->gameState()->players[getOwner()].dwellings.push_back (this);

			assert(!creatures.empty());
			assert(!creatures[0].second.empty());
			break;
		}
	case Obj::REFUGEE_CAMP:
		//is handled within newturn func
		break;

	case Obj::WAR_MACHINE_FACTORY:
		creatures.resize(3);
		creatures[0].second.push_back(CreatureID::BALLISTA);
		creatures[1].second.push_back(CreatureID::FIRST_AID_TENT);
		creatures[2].second.push_back(CreatureID::AMMO_CART);
		break;

	default:
		assert(0);
		break;
	}
}

void CGDwelling::setPropertyDer(ui8 what, ui32 val)
{
	switch (what)
	{
		case ObjProperty::OWNER: //change owner
			if (ID == Obj::CREATURE_GENERATOR1) //single generators
			{
				if (tempOwner != PlayerColor::NEUTRAL)
				{
					std::vector<ConstTransitivePtr<CGDwelling> >* dwellings = &cb->gameState()->players[tempOwner].dwellings;
					dwellings->erase (std::find(dwellings->begin(), dwellings->end(), this));
				}
				if (PlayerColor(val) != PlayerColor::NEUTRAL) //can new owner be neutral?
					cb->gameState()->players[PlayerColor(val)].dwellings.push_back (this);
			}
			break;
		case ObjProperty::AVAILABLE_CREATURE:
			creatures.resize(1);
			creatures[0].second.resize(1);
			creatures[0].second[0] = CreatureID(val);
			break;
	}
}

void CGDwelling::onHeroVisit( const CGHeroInstance * h ) const
{
	if(ID == Obj::REFUGEE_CAMP && !creatures[0].first) //Refugee Camp, no available cres
	{
		InfoWindow iw;
		iw.player = h->tempOwner;
		iw.text.addTxt(MetaString::ADVOB_TXT, 44); //{%s} \n\n The camp is deserted.  Perhaps you should try next week.
		iw.text.addReplacement(MetaString::OBJ_NAMES, ID);
		cb->sendAndApply(&iw);
		return;
	}

	PlayerRelations::PlayerRelations relations = cb->gameState()->getPlayerRelations( h->tempOwner, tempOwner );

	if ( relations == PlayerRelations::ALLIES )
		return;//do not allow recruiting or capturing

	if( !relations  &&  stacksCount() > 0) //object is guarded, owned by enemy
	{
		BlockingDialog bd(true,false);
		bd.player = h->tempOwner;
		bd.text.addTxt(MetaString::GENERAL_TXT, 421); //Much to your dismay, the %s is guarded by %s %s. Do you wish to fight the guards?
		bd.text.addReplacement(ID == Obj::CREATURE_GENERATOR1 ? MetaString::CREGENS : MetaString::CREGENS4, subID);
		bd.text.addReplacement(MetaString::ARRAY_TXT, 176 + Slots().begin()->second->getQuantityID()*3);
		bd.text.addReplacement(*Slots().begin()->second);
		cb->showBlockingDialog(&bd);
		return;
	}

	if(!relations  &&  ID != Obj::WAR_MACHINE_FACTORY)
	{
		cb->setOwner(this, h->tempOwner);
	}

	BlockingDialog bd (true,false);
	bd.player = h->tempOwner;
	if(ID == Obj::CREATURE_GENERATOR1 || ID == Obj::CREATURE_GENERATOR4)
	{
		bd.text.addTxt(MetaString::ADVOB_TXT, ID == Obj::CREATURE_GENERATOR1 ? 35 : 36); //{%s} Would you like to recruit %s? / {%s} Would you like to recruit %s, %s, %s, or %s?
		bd.text.addReplacement(ID == Obj::CREATURE_GENERATOR1 ? MetaString::CREGENS : MetaString::CREGENS4, subID);
		for(auto & elem : creatures)
			bd.text.addReplacement(MetaString::CRE_PL_NAMES, elem.second[0]);
	}
	else if(ID == Obj::REFUGEE_CAMP)
	{
		bd.text.addTxt(MetaString::ADVOB_TXT, 35); //{%s} Would you like to recruit %s?
		bd.text.addReplacement(MetaString::OBJ_NAMES, ID);
		for(auto & elem : creatures)
			bd.text.addReplacement(MetaString::CRE_PL_NAMES, elem.second[0]);
	}
	else if(ID == Obj::WAR_MACHINE_FACTORY)
		bd.text.addTxt(MetaString::ADVOB_TXT, 157); //{War Machine Factory} Would you like to purchase War Machines?
	else
		throw std::runtime_error("Illegal dwelling!");

	cb->showBlockingDialog(&bd);
}

void CGDwelling::newTurn() const
{
	if(cb->getDate(Date::DAY_OF_WEEK) != 1) //not first day of week
		return;

	//town growths and War Machines Factories are handled separately
	if(ID == Obj::TOWN  ||  ID == Obj::WAR_MACHINE_FACTORY)
		return;

	if(ID == Obj::REFUGEE_CAMP) //if it's a refugee camp, we need to pick an available creature
	{
		cb->setObjProperty(id, ObjProperty::AVAILABLE_CREATURE, VLC->creh->pickRandomMonster(cb->gameState()->getRandomGenerator()));
	}

	bool change = false;

	SetAvailableCreatures sac;
	sac.creatures = creatures;
	sac.tid = id;
	for (size_t i = 0; i < creatures.size(); i++)
	{
		if(creatures[i].second.size())
		{
			CCreature *cre = VLC->creh->creatures[creatures[i].second[0]];
			TQuantity amount = cre->growth * (1 + cre->valOfBonuses(Bonus::CREATURE_GROWTH_PERCENT)/100) + cre->valOfBonuses(Bonus::CREATURE_GROWTH);
			if (VLC->modh->settings.DWELLINGS_ACCUMULATE_CREATURES && ID != Obj::REFUGEE_CAMP) //camp should not try to accumulate different kinds of creatures
				sac.creatures[i].first += amount;
			else
				sac.creatures[i].first = amount;
			change = true;
		}
	}

	if(change)
		cb->sendAndApply(&sac);

	updateGuards();
}

void CGDwelling::updateGuards() const
{
	//TODO: store custom guard config and use it
	//TODO: store boolean flag for guards

	bool guarded = false;
	//default condition - creatures are of level 5 or higher
	for (auto creatureEntry : creatures)
	{
		if (VLC->creh->creatures[creatureEntry.second.at(0)]->level >= 5 && ID != Obj::REFUGEE_CAMP)
		{
			guarded = true;
			break;
		}
	}

	if (guarded)
	{
		for (auto creatureEntry : creatures)
		{
			const CCreature * crea = VLC->creh->creatures[creatureEntry.second.at(0)];

			SlotID slot = getSlotFor(crea->idNumber);
			StackLocation stackLocation = StackLocation(this, slot);;
			if (hasStackAtSlot(slot)) //stack already exists, overwrite it
			{
				ChangeStackCount csc;
				csc.sl = stackLocation;
				csc.count = crea->growth * 3;
				csc.absoluteValue = true;
				cb->sendAndApply(&csc);
			}
			else //slot is empty, create whole new stack
			{
				InsertNewStack ns;
				ns.sl = stackLocation;
				ns.stack = CStackBasicDescriptor(crea->idNumber, crea->growth * 3);
				cb->sendAndApply(&ns);
			}
		}
	}
}

void CGDwelling::heroAcceptsCreatures( const CGHeroInstance *h) const
{
	CreatureID crid = creatures[0].second[0];
	CCreature *crs = VLC->creh->creatures[crid];
	TQuantity count = creatures[0].first;

	if(crs->level == 1  &&  ID != Obj::REFUGEE_CAMP) //first level - creatures are for free
	{
		if(count) //there are available creatures
		{
			SlotID slot = h->getSlotFor(crid);
			if(!slot.validSlot()) //no available slot
			{
				InfoWindow iw;
				iw.player = h->tempOwner;
				iw.text.addTxt(MetaString::GENERAL_TXT, 425);//The %s would join your hero, but there aren't enough provisions to support them.
				iw.text.addReplacement(MetaString::CRE_PL_NAMES, crid);
				cb->showInfoDialog(&iw);
			}
			else //give creatures
			{
				SetAvailableCreatures sac;
				sac.tid = id;
				sac.creatures = creatures;
				sac.creatures[0].first = 0;


				InfoWindow iw;
				iw.player = h->tempOwner;
				iw.text.addTxt(MetaString::GENERAL_TXT, 423); //%d %s join your army.
				iw.text.addReplacement(count);
				iw.text.addReplacement(MetaString::CRE_PL_NAMES, crid);

				cb->showInfoDialog(&iw);
				cb->sendAndApply(&sac);
				cb->addToSlot(StackLocation(h, slot), crs, count);
			}
		}
		else //there no creatures
		{
			InfoWindow iw;
			iw.text.addTxt(MetaString::GENERAL_TXT, 422); //There are no %s here to recruit.
			iw.text.addReplacement(MetaString::CRE_PL_NAMES, crid);
			iw.player = h->tempOwner;
			cb->sendAndApply(&iw);
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
			sac.creatures[0].first = !h->getArt(ArtifactPosition::MACH1); //ballista
			sac.creatures[1].first = !h->getArt(ArtifactPosition::MACH3); //first aid tent
			sac.creatures[2].first = !h->getArt(ArtifactPosition::MACH2); //ammo cart
			cb->sendAndApply(&sac);
		}

		OpenWindow ow;
		ow.id1 = id.getNum();
		ow.id2 = h->id.getNum();
		ow.window = (ID == Obj::CREATURE_GENERATOR1 || ID == Obj::REFUGEE_CAMP)
			? OpenWindow::RECRUITMENT_FIRST
			: OpenWindow::RECRUITMENT_ALL;
		cb->sendAndApply(&ow);
	}
}

void CGDwelling::battleFinished(const CGHeroInstance *hero, const BattleResult &result) const
{
	if (result.winner == 0)
	{
		onHeroVisit(hero);
	}
}

void CGDwelling::blockingDialogAnswered(const CGHeroInstance *hero, ui32 answer) const
{
	auto relations = cb->getPlayerRelations(getOwner(), hero->getOwner());
	if(stacksCount() > 0  && relations == PlayerRelations::ENEMIES) //guards present
	{
		if(answer)
			cb->startBattleI(hero, this);
	}
	else if(answer)
	{
		heroAcceptsCreatures(hero);
	}
}

int CGTownInstance::getSightRadius() const //returns sight distance
{
	if (subID == ETownType::TOWER)
	{
		if (hasBuilt(BuildingID::GRAIL)) //skyship
			return -1; //entire map
		if (hasBuilt(BuildingID::LOOKOUT_TOWER)) //lookout tower
			return 20;
	}
	return 5;
}

void CGTownInstance::setPropertyDer(ui8 what, ui32 val)
{
///this is freakin' overcomplicated solution
	switch (what)
	{
	case ObjProperty::STRUCTURE_ADD_VISITING_HERO:
			bonusingBuildings[val]->setProperty (ObjProperty::VISITORS, visitingHero->id.getNum());
			break;
		case ObjProperty::STRUCTURE_CLEAR_VISITORS:
			bonusingBuildings[val]->setProperty (ObjProperty::STRUCTURE_CLEAR_VISITORS, 0);
			break;
		case ObjProperty::STRUCTURE_ADD_GARRISONED_HERO: //add garrisoned hero to visitors
			bonusingBuildings[val]->setProperty (ObjProperty::VISITORS, garrisonHero->id.getNum());
			break;
		case ObjProperty::BONUS_VALUE_FIRST:
			bonusValue.first = val;
			break;
		case ObjProperty::BONUS_VALUE_SECOND:
			bonusValue.second = val;
			break;
	}
}
CGTownInstance::EFortLevel CGTownInstance::fortLevel() const //0 - none, 1 - fort, 2 - citadel, 3 - castle
{
	if (hasBuilt(BuildingID::CASTLE))
		return CASTLE;
	if (hasBuilt(BuildingID::CITADEL))
		return CITADEL;
	if (hasBuilt(BuildingID::FORT))
		return FORT;
	return NONE;
}

int CGTownInstance::hallLevel() const // -1 - none, 0 - village, 1 - town, 2 - city, 3 - capitol
{
	
	if (hasBuilt(BuildingID::CAPITOL))
		return 3;
	if (hasBuilt(BuildingID::CITY_HALL))
		return 2;
	if (hasBuilt(BuildingID::TOWN_HALL))
		return 1;
	if (hasBuilt(BuildingID::VILLAGE_HALL))
		return 0;
	return -1;
}
int CGTownInstance::mageGuildLevel() const
{
	if (hasBuilt(BuildingID::MAGES_GUILD_5))
		return 5;
	if (hasBuilt(BuildingID::MAGES_GUILD_4))
		return 4;
	if (hasBuilt(BuildingID::MAGES_GUILD_3))
		return 3;
	if (hasBuilt(BuildingID::MAGES_GUILD_2))
		return 2;
	if (hasBuilt(BuildingID::MAGES_GUILD_1))
		return 1;
	return 0;
}

int CGTownInstance::getHordeLevel(const int & HID)  const//HID - 0 or 1; returns creature level or -1 if that horde structure is not present
{
	return town->hordeLvl.at(HID);
}

int CGTownInstance::creatureGrowth(const int & level) const
{
	return getGrowthInfo(level).totalGrowth();
}

GrowthInfo CGTownInstance::getGrowthInfo(int level) const
{
	GrowthInfo ret;

	if (level<0 || level >=GameConstants::CREATURES_PER_TOWN)
		return ret;
	if (creatures[level].second.empty())
		return ret; //no dwelling

	const CCreature *creature = VLC->creh->creatures[creatures[level].second.back()];
	const int base = creature->growth;
	int castleBonus = 0;

	ret.entries.push_back(GrowthInfo::Entry(VLC->generaltexth->allTexts[590], base));// \n\nBasic growth %d"

	if (hasBuilt(BuildingID::CASTLE))
		ret.entries.push_back(GrowthInfo::Entry(subID, BuildingID::CASTLE, castleBonus = base));
	else if (hasBuilt(BuildingID::CITADEL))
		ret.entries.push_back(GrowthInfo::Entry(subID, BuildingID::CITADEL, castleBonus = base / 2));

	if(town->hordeLvl.at(0) == level)//horde 1
		if(hasBuilt(BuildingID::HORDE_1))
			ret.entries.push_back(GrowthInfo::Entry(subID, BuildingID::HORDE_1, creature->hordeGrowth));

	if(town->hordeLvl.at(1) == level)//horde 2
		if(hasBuilt(BuildingID::HORDE_2))
			ret.entries.push_back(GrowthInfo::Entry(subID, BuildingID::HORDE_2, creature->hordeGrowth));

	int dwellingBonus = 0;
	if(const PlayerState *p = cb->getPlayer(tempOwner, false))
	{
		for(const CGDwelling *dwelling : p->dwellings)
			if(vstd::contains(creatures[level].second, dwelling->creatures[0].second[0]))
				dwellingBonus++;
	}

	if(dwellingBonus)
		ret.entries.push_back(GrowthInfo::Entry(VLC->generaltexth->allTexts[591], dwellingBonus));// \nExternal dwellings %+d

	//other *-of-legion-like bonuses (%d to growth cumulative with grail)
	TBonusListPtr bonuses = getBonuses(Selector::type(Bonus::CREATURE_GROWTH).And(Selector::subtype(level)));
	for(const Bonus *b : *bonuses)
		ret.entries.push_back(GrowthInfo::Entry(b->val, b->Description()));

	//statue-of-legion-like bonus: % to base+castle
	TBonusListPtr bonuses2 = getBonuses(Selector::type(Bonus::CREATURE_GROWTH_PERCENT));
	for(const Bonus *b : *bonuses2)
		ret.entries.push_back(GrowthInfo::Entry(b->val * (base + castleBonus) / 100, b->Description()));

	if(hasBuilt(BuildingID::GRAIL)) //grail - +50% to ALL (so far added) growth
		ret.entries.push_back(GrowthInfo::Entry(subID, BuildingID::GRAIL, ret.totalGrowth() / 2));

	return ret;
}

TResources CGTownInstance::dailyIncome() const
{
	TResources ret;

	for (auto & p : town->buildings) 
	{ 
		BuildingID buildingUpgrade;

		for (auto & p2 : town->buildings) 
		{ 
			if (p2.second->upgrade == p.first)
			{
				buildingUpgrade = p2.first;
			}
		}

		if (!hasBuilt(buildingUpgrade)&&(hasBuilt(p.first)))
		{
			ret += p.second->produce;
		}
	
	}

	return ret;
}
bool CGTownInstance::hasFort() const
{
	return hasBuilt(BuildingID::FORT);
}
bool CGTownInstance::hasCapitol() const
{
	return hasBuilt(BuildingID::CAPITOL);
}
CGTownInstance::CGTownInstance()
	:IShipyard(this), IMarket(this), town(nullptr), builded(0), destroyed(0), identifier(0), alignment(0xff)
{

}

CGTownInstance::~CGTownInstance()
{
	for (auto & elem : bonusingBuildings)
		delete elem;
}

int CGTownInstance::spellsAtLevel(int level, bool checkGuild) const
{
	if(checkGuild && mageGuildLevel() < level)
		return 0;
	int ret = 6 - level; //how many spells are available at this level

	if (hasBuilt(BuildingID::LIBRARY, ETownType::TOWER))
		ret++;

	return ret;
}

bool CGTownInstance::needsLastStack() const
{
	if(garrisonHero)
		return true;
	else return false;
}

void CGTownInstance::onHeroVisit(const CGHeroInstance * h) const
{
	if( !cb->gameState()->getPlayerRelations( getOwner(), h->getOwner() ))//if this is enemy
	{
		if(armedGarrison() || visitingHero)
		{
			const CGHeroInstance *defendingHero = nullptr;
			const CArmedInstance *defendingArmy = this;

			if(visitingHero)
				defendingHero = visitingHero;
			else if(garrisonHero)
				defendingHero = garrisonHero;

			if(defendingHero)
				defendingArmy = defendingHero;

			bool outsideTown = (defendingHero == visitingHero && garrisonHero);

			//TODO
			//"borrowing" army from garrison to visiting hero

			cb->startBattlePrimary(h, defendingArmy, getSightCenter(), h, defendingHero, false, (outsideTown ? nullptr : this));
		}
		else
		{
			cb->setOwner(this, h->tempOwner);
			removeCapitols(h->getOwner());
			cb->heroVisitCastle(this, h);
		}
	}
	else if(h->visitablePos() == visitablePos())
	{
		if (h->commander && !h->commander->alive) //rise commander. TODO: interactive script
		{
			SetCommanderProperty scp;
			scp.heroid = h->id;
			scp.which = SetCommanderProperty::ALIVE;
			scp.amount = 1;
			cb->sendAndApply (&scp);
		}
		cb->heroVisitCastle(this, h);
	}
	else
	{
		logGlobal->errorStream() << h->name << " visits allied town of " << name << " from different pos?";
	}
}

void CGTownInstance::onHeroLeave(const CGHeroInstance * h) const
{
	//FIXME: find out why this issue appears on random maps
	if (visitingHero == h)
	{
		cb->stopHeroVisitCastle(this, h);
		//logGlobal->warnStream() << h->name << " correctly left town " << name;
	}
	else
		logGlobal->warnStream() << "Warning, " << h->name << " tries to leave the town " << name << " but hero is not inside.";
}

std::string CGTownInstance::getObjectName() const
{
	return name + ", " + town->faction->name;
}

void CGTownInstance::initObj()
///initialize town structures
{
	blockVisit = true;

	if (subID == ETownType::DUNGEON)
		creatures.resize(GameConstants::CREATURES_PER_TOWN+1);//extra dwelling for Dungeon
	else
		creatures.resize(GameConstants::CREATURES_PER_TOWN);
	for (int level = 0; level < GameConstants::CREATURES_PER_TOWN; level++)
	{
		BuildingID buildID = BuildingID(BuildingID::DWELL_FIRST).advance(level);
		int upgradeNum = 0;

		for (; town->buildings.count(buildID); upgradeNum++, buildID.advance(GameConstants::CREATURES_PER_TOWN))
		{
			if (hasBuilt(buildID) && town->creatures.at(level).size() > upgradeNum)
				creatures[level].second.push_back(town->creatures[level][upgradeNum]);
		}
	}

	switch (subID)
	{ //add new visitable objects
		case 0:
			bonusingBuildings.push_back (new COPWBonus(BuildingID::STABLES, this));
			break;
		case 5:
			bonusingBuildings.push_back (new COPWBonus(BuildingID::MANA_VORTEX, this));
			//fallthrough
		case 2: case 3: case 6:
			bonusingBuildings.push_back (new CTownBonus(BuildingID::SPECIAL_4, this));
			break;
		case 7:
			bonusingBuildings.push_back (new CTownBonus(BuildingID::SPECIAL_1, this));
			break;
	}
	//add special bonuses from buildings

	recreateBuildingsBonuses();
	updateAppearance();
}

void CGTownInstance::newTurn() const
{
	if (cb->getDate(Date::DAY_OF_WEEK) == 1) //reset on new week
	{
		auto & rand = cb->gameState()->getRandomGenerator();

		//give resources for Rampart, Mystic Pond
		if (hasBuilt(BuildingID::MYSTIC_POND, ETownType::RAMPART)
			&& cb->getDate(Date::DAY) != 1 && (tempOwner < PlayerColor::PLAYER_LIMIT))
		{
			int resID = rand.nextInt(2, 5); //bonus to random rare resource
			resID = (resID==2)?1:resID;
			int resVal = rand.nextInt(1, 4);//with size 1..4
			cb->giveResource(tempOwner, static_cast<Res::ERes>(resID), resVal);
			cb->setObjProperty (id, ObjProperty::BONUS_VALUE_FIRST, resID);
			cb->setObjProperty (id, ObjProperty::BONUS_VALUE_SECOND, resVal);
		}

		if ( subID == ETownType::DUNGEON )
			for (auto & elem : bonusingBuildings)
		{
			if ((elem)->ID == BuildingID::MANA_VORTEX)
				cb->setObjProperty (id, ObjProperty::STRUCTURE_CLEAR_VISITORS, (elem)->id); //reset visitors for Mana Vortex
		}

		if (tempOwner == PlayerColor::NEUTRAL) //garrison growth for neutral towns
			{
				std::vector<SlotID> nativeCrits; //slots
				for (auto & elem : Slots())
				{
					if (elem.second->type->faction == subID) //native
					{
						nativeCrits.push_back(elem.first); //collect matching slots
					}
				}
				if (nativeCrits.size())
				{
					SlotID pos = *RandomGeneratorUtil::nextItem(nativeCrits, rand);
					StackLocation sl(this, pos);

					const CCreature *c = getCreature(pos);
					if (rand.nextInt(99) < 90 || c->upgrades.empty()) //increase number if no upgrade available
					{
						cb->changeStackCount(sl, c->growth);
					}
					else //upgrade
					{
						cb->changeStackType(sl, VLC->creh->creatures[*c->upgrades.begin()]);
					}
				}
				if ((stacksCount() < GameConstants::ARMY_SIZE && rand.nextInt(99) < 25) || Slots().empty()) //add new stack
				{
					int i = rand.nextInt(std::min(GameConstants::CREATURES_PER_TOWN, cb->getDate(Date::MONTH) << 1) - 1);
					if (!town->creatures[i].empty())
					{
						CreatureID c = town->creatures[i][0];
						SlotID n;

						TQuantity count = creatureGrowth(i);
						if (!count) // no dwelling
							count = VLC->creh->creatures[c]->growth;

						{//no lower tiers or above current month

							if ((n = getSlotFor(c)).validSlot())
							{
								StackLocation sl(this, n);
								if (slotEmpty(n))
									cb->insertNewStack(sl, VLC->creh->creatures[c], count);
								else //add to existing
									cb->changeStackCount(sl, count);
							}
						}
					}
				}
			}
	}
}
/*
int3 CGTownInstance::getSightCenter() const
{
	return pos - int3(2,0,0);
}
*/
bool CGTownInstance::passableFor(PlayerColor color) const
{
	if (!armedGarrison())//empty castle - anyone can visit
		return true;
	if ( tempOwner == PlayerColor::NEUTRAL )//neutral guarded - no one can visit
		return false;

	if (cb->getPlayerRelations(tempOwner, color) != PlayerRelations::ENEMIES)
		return true;
	return false;
}

void CGTownInstance::getOutOffsets( std::vector<int3> &offsets ) const
{
	offsets = {int3(-1,2,0), int3(-3,2,0)};
}

void CGTownInstance::removeCapitols (PlayerColor owner) const
{
	if (hasCapitol()) // search if there's an older capitol
	{
		PlayerState* state = cb->gameState()->getPlayer (owner); //get all towns owned by player
		for (auto i = state->towns.cbegin(); i < state->towns.cend(); ++i)
		{
			if (*i != this && (*i)->hasCapitol())
			{
				RazeStructures rs;
				rs.tid = id;
				rs.bid.insert(BuildingID::CAPITOL);
				rs.destroyed = destroyed;
				cb->sendAndApply(&rs);
				return;
			}
		}
	}
}

int CGTownInstance::getBoatType() const
{
	switch (town->faction->alignment)
	{
	case EAlignment::EVIL : return 0;
	case EAlignment::GOOD : return 1;
	case EAlignment::NEUTRAL : return 2;
	}
	assert(0);
	return -1;
}

int CGTownInstance::getMarketEfficiency() const
{
	if (!hasBuilt(BuildingID::MARKETPLACE))
		return 0;

	const PlayerState *p = cb->getPlayer(tempOwner);
	assert(p);

	int marketCount = 0;
	for(const CGTownInstance *t : p->towns)
		if(t->hasBuilt(BuildingID::MARKETPLACE))
			marketCount++;

	return marketCount;
}

bool CGTownInstance::allowsTrade(EMarketMode::EMarketMode mode) const
{
	switch(mode)
	{
	case EMarketMode::RESOURCE_RESOURCE:
	case EMarketMode::RESOURCE_PLAYER:
		return hasBuilt(BuildingID::MARKETPLACE);

	case EMarketMode::ARTIFACT_RESOURCE:
	case EMarketMode::RESOURCE_ARTIFACT:
		return hasBuilt(BuildingID::ARTIFACT_MERCHANT, ETownType::TOWER)
		    || hasBuilt(BuildingID::ARTIFACT_MERCHANT, ETownType::DUNGEON)
		    || hasBuilt(BuildingID::ARTIFACT_MERCHANT, ETownType::CONFLUX);

	case EMarketMode::CREATURE_RESOURCE:
		return hasBuilt(BuildingID::FREELANCERS_GUILD, ETownType::STRONGHOLD);

	case EMarketMode::CREATURE_UNDEAD:
		return hasBuilt(BuildingID::SKELETON_TRANSFORMER, ETownType::NECROPOLIS);

	case EMarketMode::RESOURCE_SKILL:
		return hasBuilt(BuildingID::MAGIC_UNIVERSITY, ETownType::CONFLUX);
	default:
		assert(0);
		return false;
	}
}

std::vector<int> CGTownInstance::availableItemsIds(EMarketMode::EMarketMode mode) const
{
	if(mode == EMarketMode::RESOURCE_ARTIFACT)
	{
		std::vector<int> ret;
		for(const CArtifact *a : merchantArtifacts)
			if(a)
				ret.push_back(a->id);
			else
				ret.push_back(-1);
		return ret;
	}
	else if ( mode == EMarketMode::RESOURCE_SKILL )
	{
		return universitySkills;
	}
	else
		return IMarket::availableItemsIds(mode);
}

void CGTownInstance::setType(si32 ID, si32 subID)
{
	assert(ID == Obj::TOWN); // just in case
	CGObjectInstance::setType(ID, subID);
	town = VLC->townh->factions[subID]->town;
	randomizeArmy(subID);
	updateAppearance();
}

void CGTownInstance::updateAppearance()
{
	//FIXME: not the best way to do this
	auto app = VLC->objtypeh->getHandlerFor(ID, subID)->getOverride(cb->gameState()->getTile(visitablePos())->terType, this);
	if (app)
		appearance = app.get();
}

std::string CGTownInstance::nodeName() const
{
	return "Town (" + (town ? town->faction->name : "unknown") + ") of " +  name;
}

void CGTownInstance::deserializationFix()
{
	attachTo(&townAndVis);

	//Hero is already handled by CGameState::attachArmedObjects

// 	if(visitingHero)
// 		visitingHero->attachTo(&townAndVis);
// 	if(garrisonHero)
// 		garrisonHero->attachTo(this);
}

void CGTownInstance::updateMoraleBonusFromArmy()
{
	Bonus *b = getExportedBonusList().getFirst(Selector::sourceType(Bonus::ARMY).And(Selector::type(Bonus::MORALE)));
	if(!b)
	{
		b = new Bonus(Bonus::PERMANENT, Bonus::MORALE, Bonus::ARMY, 0, -1);
		addNewBonus(b);
	}

	if (garrisonHero)
	{
		b->val = 0;
		CBonusSystemNode::treeHasChanged();
	}
	else
		CArmedInstance::updateMoraleBonusFromArmy();
}

void CGTownInstance::recreateBuildingsBonuses()
{
	static TPropagatorPtr playerProp(new CPropagatorNodeType(PLAYER));

	BonusList bl;
	getExportedBonusList().getBonuses(bl, Selector::sourceType(Bonus::TOWN_STRUCTURE));
	for(Bonus *b : bl)
		removeBonus(b);

	//tricky! -> checks tavern only if no bratherhood of sword or not a castle
	if(subID != ETownType::CASTLE || !addBonusIfBuilt(BuildingID::BROTHERHOOD, Bonus::MORALE, +2))
		addBonusIfBuilt(BuildingID::TAVERN, Bonus::MORALE, +1);

	if(subID == ETownType::CASTLE) //castle
	{
		addBonusIfBuilt(BuildingID::LIGHTHOUSE, Bonus::SEA_MOVEMENT, +500, playerProp);
		addBonusIfBuilt(BuildingID::GRAIL,      Bonus::MORALE, +2, playerProp); //colossus
	}
	else if(subID == ETownType::RAMPART) //rampart
	{
		addBonusIfBuilt(BuildingID::FOUNTAIN_OF_FORTUNE, Bonus::LUCK, +2); //fountain of fortune
		addBonusIfBuilt(BuildingID::GRAIL, Bonus::LUCK, +2, playerProp); //guardian spirit
	}
	else if(subID == ETownType::TOWER) //tower
	{
		addBonusIfBuilt(BuildingID::GRAIL, Bonus::PRIMARY_SKILL, +15, PrimarySkill::KNOWLEDGE); //grail
	}
	else if(subID == ETownType::INFERNO) //Inferno
	{
		addBonusIfBuilt(BuildingID::STORMCLOUDS, Bonus::PRIMARY_SKILL, +2, PrimarySkill::SPELL_POWER); //Brimstone Clouds
	}
	else if(subID == ETownType::NECROPOLIS) //necropolis
	{
		addBonusIfBuilt(BuildingID::COVER_OF_DARKNESS,    Bonus::DARKNESS, +20);
		addBonusIfBuilt(BuildingID::NECROMANCY_AMPLIFIER, Bonus::SECONDARY_SKILL_PREMY, +10, playerProp, SecondarySkill::NECROMANCY); //necromancy amplifier
		addBonusIfBuilt(BuildingID::GRAIL, Bonus::SECONDARY_SKILL_PREMY, +20, playerProp, SecondarySkill::NECROMANCY); //Soul prison
	}
	else if(subID == ETownType::DUNGEON) //Dungeon
	{
		addBonusIfBuilt(BuildingID::GRAIL, Bonus::PRIMARY_SKILL, +12, PrimarySkill::SPELL_POWER); //grail
	}
	else if(subID == ETownType::STRONGHOLD) //Stronghold
	{
		addBonusIfBuilt(BuildingID::GRAIL, Bonus::PRIMARY_SKILL, +20, PrimarySkill::ATTACK); //grail
	}
	else if(subID == ETownType::FORTRESS) //Fortress
	{
		addBonusIfBuilt(BuildingID::GLYPHS_OF_FEAR, Bonus::PRIMARY_SKILL, +2, PrimarySkill::DEFENSE); //Glyphs of Fear
		addBonusIfBuilt(BuildingID::BLOOD_OBELISK,  Bonus::PRIMARY_SKILL, +2, PrimarySkill::ATTACK); //Blood Obelisk
		addBonusIfBuilt(BuildingID::GRAIL, Bonus::PRIMARY_SKILL, +10, PrimarySkill::ATTACK); //grail
		addBonusIfBuilt(BuildingID::GRAIL, Bonus::PRIMARY_SKILL, +10, PrimarySkill::DEFENSE); //grail
	}
	else if(subID == ETownType::CONFLUX)
	{

	}
}

bool CGTownInstance::addBonusIfBuilt(BuildingID building, Bonus::BonusType type, int val, int subtype /*= -1*/)
{
	static auto emptyPropagator = TPropagatorPtr();
	return addBonusIfBuilt(building, type, val, emptyPropagator, subtype);
}

bool CGTownInstance::addBonusIfBuilt(BuildingID building, Bonus::BonusType type, int val, TPropagatorPtr & prop, int subtype /*= -1*/)
{
	if(hasBuilt(building))
	{
		std::ostringstream descr;
		descr << town->buildings.at(building)->Name() << " ";
		if(val > 0)
			descr << "+";
		else if(val < 0)
			descr << "-";
		descr << val;

		Bonus *b = new Bonus(Bonus::PERMANENT, type, Bonus::TOWN_STRUCTURE, val, building, descr.str(), subtype);
		if(prop)
			b->addPropagator(prop);
		addNewBonus(b);
		return true;
	}

	return false;
}

void CGTownInstance::setVisitingHero(CGHeroInstance *h)
{
	//if (!(!!visitingHero == !h))
	//{
	//	logGlobal->warnStream() << boost::format("Hero visiting town %s is %s ") % name % (visitingHero.get() ? visitingHero->name : "NULL");
	//	logGlobal->warnStream() << boost::format("New hero will be %s ") % (h ? h->name : "NULL");
	//	
	//}
	assert(!!visitingHero == !h);

	if(h)
	{
		PlayerState *p = cb->gameState()->getPlayer(h->tempOwner);
		assert(p);
		h->detachFrom(p);
		h->attachTo(&townAndVis);
		visitingHero = h;
		h->visitedTown = this;
		h->inTownGarrison = false;
	}
	else
	{
		PlayerState *p = cb->gameState()->getPlayer(visitingHero->tempOwner);
		visitingHero->visitedTown = nullptr;
		visitingHero->detachFrom(&townAndVis);
		visitingHero->attachTo(p);
		visitingHero = nullptr;
	}
}

void CGTownInstance::setGarrisonedHero(CGHeroInstance *h)
{
	assert(!!garrisonHero == !h);
	if(h)
	{
		PlayerState *p = cb->gameState()->getPlayer(h->tempOwner);
		assert(p);
		h->detachFrom(p);
		h->attachTo(this);
		garrisonHero = h;
		h->visitedTown = this;
		h->inTownGarrison = true;
	}
	else
	{
		PlayerState *p = cb->gameState()->getPlayer(garrisonHero->tempOwner);
		garrisonHero->visitedTown = nullptr;
		garrisonHero->inTownGarrison = false;
		garrisonHero->detachFrom(this);
		garrisonHero->attachTo(p);
		garrisonHero = nullptr;
	}
	updateMoraleBonusFromArmy(); //avoid giving morale bonus for same army twice
}

bool CGTownInstance::armedGarrison() const
{
	return stacksCount() || garrisonHero;
}

int CGTownInstance::getTownLevel() const
{
	// count all buildings that are not upgrades
	return boost::range::count_if(builtBuildings, [&](const BuildingID & build)
	{
		return town->buildings.at(build) && town->buildings.at(build)->upgrade == -1;
	});
}

CBonusSystemNode * CGTownInstance::whatShouldBeAttached()
{
	return &townAndVis;
}

const CArmedInstance * CGTownInstance::getUpperArmy() const
{
	if(garrisonHero)
		return garrisonHero;
	return this;
}

bool CGTownInstance::hasBuilt(BuildingID buildingID, int townID) const
{
	if (townID == town->faction->index || townID == ETownType::ANY)
		return hasBuilt(buildingID);
	return false;
}

bool CGTownInstance::hasBuilt(BuildingID buildingID) const
{
	return vstd::contains(builtBuildings, buildingID);
}

CBuilding::TRequired CGTownInstance::genBuildingRequirements(BuildingID buildID, bool includeUpgrade) const
{
	const CBuilding * building = town->buildings.at(buildID);

	std::function<CBuilding::TRequired::Variant(const BuildingID &)> dependTest =
	[&](const BuildingID & id) -> CBuilding::TRequired::Variant
	{
		const CBuilding * build = town->buildings.at(id);

		if (!hasBuilt(id))
			return id;

		if (build->upgrade != BuildingID::NONE && !hasBuilt(build->upgrade))
			return build->upgrade;

		return build->requirements.morph(dependTest);
	};

	CBuilding::TRequired::OperatorAll requirements;
	if (building->upgrade != BuildingID::NONE)
	{
		const CBuilding * upgr = town->buildings.at(building->upgrade);

		if (includeUpgrade)
			requirements.expressions.push_back(upgr->bid);
		requirements.expressions.push_back(upgr->requirements.morph(dependTest));
	}
	requirements.expressions.push_back(building->requirements.morph(dependTest));

	CBuilding::TRequired::Variant variant(requirements);
	CBuilding::TRequired ret(variant);
	ret.minimize();
	return ret;
}

void CGTownInstance::addHeroToStructureVisitors( const CGHeroInstance *h, si32 structureInstanceID ) const
{
	if(visitingHero == h)
		cb->setObjProperty(id, ObjProperty::STRUCTURE_ADD_VISITING_HERO, structureInstanceID); //add to visitors
	else if(garrisonHero == h)
		cb->setObjProperty(id, ObjProperty::STRUCTURE_ADD_GARRISONED_HERO, structureInstanceID); //then it must be garrisoned hero
	else
	{
		//should never ever happen
        logGlobal->errorStream() << "Cannot add hero " << h->name << " to visitors of structure #" << structureInstanceID;
		assert(0);
	}
}

void CGTownInstance::battleFinished(const CGHeroInstance *hero, const BattleResult &result) const
{
	if(result.winner == 0)
	{
		removeCapitols(hero->getOwner());
		cb->setOwner (this, hero->tempOwner); //give control after checkout is done
		FoWChange fw;
		fw.player = hero->tempOwner;
		fw.mode = 1;
		cb->getTilesInRange(fw.tiles, getSightCenter(), getSightRadius(), tempOwner, 1);
		cb->sendAndApply (&fw);
	}
}

COPWBonus::COPWBonus (BuildingID index, CGTownInstance *TOWN)
{
	ID = index;
	town = TOWN;
	id = town->bonusingBuildings.size();
}
void COPWBonus::setProperty(ui8 what, ui32 val)
{
	switch (what)
	{
		case ObjProperty::VISITORS:
			visitors.insert(val);
			break;
		case ObjProperty::STRUCTURE_CLEAR_VISITORS:
			visitors.clear();
			break;
	}
}
void COPWBonus::onHeroVisit (const CGHeroInstance * h) const
{
	ObjectInstanceID heroID = h->id;
	if (town->hasBuilt(ID))
	{
		InfoWindow iw;
		iw.player = h->tempOwner;
		switch (town->subID)
		{
			case ETownType::CASTLE: //Stables
				if (!h->hasBonusFrom(Bonus::OBJECT, Obj::STABLES)) //does not stack with advMap Stables
				{
					GiveBonus gb;
					gb.bonus = Bonus(Bonus::ONE_WEEK, Bonus::LAND_MOVEMENT, Bonus::OBJECT, 600, 94, VLC->generaltexth->arraytxt[100]);
					gb.id = heroID.getNum();
					cb->giveHeroBonus(&gb);
					iw.text << VLC->generaltexth->allTexts[580];
					cb->showInfoDialog(&iw);
				}
				break;
			case ETownType::DUNGEON: //Mana Vortex
				if (visitors.empty() && h->mana <= h->manaLimit() * 2)
				{
					cb->setManaPoints (heroID, 2 * h->manaLimit());
					//TODO: investigate line below
					//cb->setObjProperty (town->id, ObjProperty::VISITED, true);
					iw.text << VLC->generaltexth->allTexts[579];
					cb->showInfoDialog(&iw);
					town->addHeroToStructureVisitors(h, id);
				}
				break;
		}
	}
}
CTownBonus::CTownBonus (BuildingID index, CGTownInstance *TOWN)
{
	ID = index;
	town = TOWN;
	id = town->bonusingBuildings.size();
}
void CTownBonus::setProperty (ui8 what, ui32 val)
{
	if(what == ObjProperty::VISITORS)
		visitors.insert(ObjectInstanceID(val));
}
void CTownBonus::onHeroVisit (const CGHeroInstance * h) const
{
	ObjectInstanceID heroID = h->id;
	if (town->hasBuilt(ID) && visitors.find(heroID) == visitors.end())
	{
		InfoWindow iw;
		PrimarySkill::PrimarySkill what = PrimarySkill::ATTACK;
		int val=0, mid=0;
		switch (ID)
		{
			case BuildingID::SPECIAL_4:
				switch(town->subID)
				{
					case ETownType::TOWER: //wall
						what = PrimarySkill::KNOWLEDGE;
						val = 1;
						mid = 581;
						iw.components.push_back (Component(Component::PRIM_SKILL, 3, 1, 0));
						break;
					case ETownType::INFERNO: //order of fire
						what = PrimarySkill::SPELL_POWER;
						val = 1;
						mid = 582;
						iw.components.push_back (Component(Component::PRIM_SKILL, 2, 1, 0));
						break;
					case ETownType::STRONGHOLD://hall of Valhalla
						what = PrimarySkill::ATTACK;
						val = 1;
						mid = 584;
						iw.components.push_back (Component(Component::PRIM_SKILL, 0, 1, 0));
						break;
					case ETownType::DUNGEON://academy of battle scholars
						what = PrimarySkill::EXPERIENCE;
						val = h->calculateXp(1000);
						mid = 583;
						iw.components.push_back (Component(Component::EXPERIENCE, 0, val, 0));
						break;
				}
				break;
			case BuildingID::SPECIAL_1:
				switch(town->subID)
				{
					case ETownType::FORTRESS: //cage of warlords
						what = PrimarySkill::DEFENSE;
						val = 1;
						mid = 585;
						iw.components.push_back (Component(Component::PRIM_SKILL, 1, 1, 0));
						break;
				}
				break;
		}
		assert(mid);
		iw.player = cb->getOwner(heroID);
		iw.text << VLC->generaltexth->allTexts[mid];
		cb->showInfoDialog(&iw);
		cb->changePrimSkill (cb->getHero(heroID), what, val);
		town->addHeroToStructureVisitors(h, id);
	}
}

GrowthInfo::Entry::Entry(const std::string &format, int _count)
	: count(_count)
{
	description = boost::str(boost::format(format) % count);
}

GrowthInfo::Entry::Entry(int subID, BuildingID building, int _count)
	: count(_count)
{
	description = boost::str(boost::format("%s %+d") % VLC->townh->factions[subID]->town->buildings.at(building)->Name() % count);
}

GrowthInfo::Entry::Entry(int _count, const std::string &fullDescription)
	: count(_count)
{
	description = fullDescription;
}

CTownAndVisitingHero::CTownAndVisitingHero()
{
	setNodeType(TOWN_AND_VISITOR);
}

int GrowthInfo::totalGrowth() const
{
	int ret = 0;
	for(const Entry &entry : entries)
		ret += entry.count;

	return ret;
}
