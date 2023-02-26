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
#include "../spells/CSpellHandler.h"

#include "../NetPacks.h"
#include "../CConfigHandler.h"
#include "../CGeneralTextHandler.h"
#include "../CModHandler.h"
#include "../IGameCallback.h"
#include "../CGameState.h"
#include "../mapping/CMap.h"
#include "../CPlayerState.h"
#include "../TerrainHandler.h"
#include "../serializer/JsonSerializeFormat.h"
#include "../HeroBonus.h"

VCMI_LIB_NAMESPACE_BEGIN

std::vector<const CArtifact *> CGTownInstance::merchantArtifacts;
std::vector<int> CGTownInstance::universitySkills;

CSpecObjInfo::CSpecObjInfo():
	owner(nullptr)
{

}

void CCreGenAsCastleInfo::serializeJson(JsonSerializeFormat & handler)
{
	handler.serializeString("sameAsTown", instanceId);

	if(!handler.saving)
	{
		asCastle = !instanceId.empty();
		allowedFactions.clear();
	}

	if(!asCastle)
	{
		std::vector<bool> standard;
		standard.resize(VLC->townh->size(), true);

		JsonSerializeFormat::LIC allowedLIC(standard, &FactionID::decode, &FactionID::encode);
		allowedLIC.any = allowedFactions;

		handler.serializeLIC("allowedFactions", allowedLIC);

		if(!handler.saving)
		{
			allowedFactions = allowedLIC.any;
		}
	}
}

void CCreGenLeveledInfo::serializeJson(JsonSerializeFormat & handler)
{
	handler.serializeInt("minLevel", minLevel, static_cast<ui8>(1));
	handler.serializeInt("maxLevel", maxLevel, static_cast<ui8>(7));

	if(!handler.saving)
	{
		//todo: safely allow any level > 7
		vstd::amax(minLevel, 1);
		vstd::amin(minLevel, 7);
		vstd::abetween(maxLevel, minLevel, 7);
	}
}

void CCreGenLeveledCastleInfo::serializeJson(JsonSerializeFormat & handler)
{
	CCreGenAsCastleInfo::serializeJson(handler);
	CCreGenLeveledInfo::serializeJson(handler);
}

CGDwelling::CGDwelling()
	: info(nullptr)
{
}

CGDwelling::~CGDwelling()
{
	vstd::clear_pointer(info);
}

void CGDwelling::initObj(CRandomGenerator & rand)
{
	switch(ID)
	{
	case Obj::CREATURE_GENERATOR1:
	case Obj::CREATURE_GENERATOR4:
		{
			VLC->objtypeh->getHandlerFor(ID, subID)->configureObject(this, rand);

			if (getOwner() != PlayerColor::NEUTRAL)
				cb->gameState()->players[getOwner()].dwellings.emplace_back(this);

			assert(!creatures.empty());
			assert(!creatures[0].second.empty());
			break;
		}
	case Obj::REFUGEE_CAMP:
		//is handled within newturn func
		break;

	case Obj::WAR_MACHINE_FACTORY:
		creatures.resize(3);
		creatures[0].second.emplace_back(CreatureID::BALLISTA);
		creatures[1].second.emplace_back(CreatureID::FIRST_AID_TENT);
		creatures[2].second.emplace_back(CreatureID::AMMO_CART);
		break;

	default:
		assert(0);
		break;
	}
}

void CGDwelling::initRandomObjectInfo()
{
	vstd::clear_pointer(info);
	switch(ID)
	{
		case Obj::RANDOM_DWELLING: info = new CCreGenLeveledCastleInfo();
			break;
		case Obj::RANDOM_DWELLING_LVL: info = new CCreGenAsCastleInfo();
			break;
		case Obj::RANDOM_DWELLING_FACTION: info = new CCreGenLeveledInfo();
			break;
	}

	if(info)
		info->owner = this;
}

void CGDwelling::setPropertyDer(ui8 what, ui32 val)
{
	switch (what)
	{
		case ObjProperty::OWNER: //change owner
			if (ID == Obj::CREATURE_GENERATOR1 || ID == Obj::CREATURE_GENERATOR2
				|| ID == Obj::CREATURE_GENERATOR3 || ID == Obj::CREATURE_GENERATOR4)
			{
				if (tempOwner != PlayerColor::NEUTRAL)
				{
					std::vector<ConstTransitivePtr<CGDwelling> >* dwellings = &cb->gameState()->players[tempOwner].dwellings;
					dwellings->erase (std::find(dwellings->begin(), dwellings->end(), this));
				}
				if (PlayerColor(val) != PlayerColor::NEUTRAL) //can new owner be neutral?
					cb->gameState()->players[PlayerColor(val)].dwellings.emplace_back(this);
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
		if(settings["gameTweaks"]["numericCreaturesQuantities"].Bool())
			bd.text.addReplacement(CCreature::getQuantityRangeStringForId(Slots().begin()->second->getQuantityID()));
		else
			bd.text.addReplacement(MetaString::ARRAY_TXT, 173 + (int)Slots().begin()->second->getQuantityID()*3);
		bd.text.addReplacement(*Slots().begin()->second);
		cb->showBlockingDialog(&bd);
		return;
	}

	// TODO this shouldn't be hardcoded
	if(!relations && ID != Obj::WAR_MACHINE_FACTORY && ID != Obj::REFUGEE_CAMP)
	{
		cb->setOwner(this, h->tempOwner);
	}

	BlockingDialog bd (true,false);
	bd.player = h->tempOwner;
	if(ID == Obj::CREATURE_GENERATOR1 || ID == Obj::CREATURE_GENERATOR4)
	{
		bd.text.addTxt(MetaString::ADVOB_TXT, ID == Obj::CREATURE_GENERATOR1 ? 35 : 36); //{%s} Would you like to recruit %s? / {%s} Would you like to recruit %s, %s, %s, or %s?
		bd.text.addReplacement(ID == Obj::CREATURE_GENERATOR1 ? MetaString::CREGENS : MetaString::CREGENS4, subID);
		for(const auto & elem : creatures)
			bd.text.addReplacement(MetaString::CRE_PL_NAMES, elem.second[0]);
	}
	else if(ID == Obj::REFUGEE_CAMP)
	{
		bd.text.addTxt(MetaString::ADVOB_TXT, 35); //{%s} Would you like to recruit %s?
		bd.text.addReplacement(MetaString::OBJ_NAMES, ID);
		for(const auto & elem : creatures)
			bd.text.addReplacement(MetaString::CRE_PL_NAMES, elem.second[0]);
	}
	else if(ID == Obj::WAR_MACHINE_FACTORY)
		bd.text.addTxt(MetaString::ADVOB_TXT, 157); //{War Machine Factory} Would you like to purchase War Machines?
	else
		throw std::runtime_error("Illegal dwelling!");

	cb->showBlockingDialog(&bd);
}

void CGDwelling::newTurn(CRandomGenerator & rand) const
{
	if(cb->getDate(Date::DAY_OF_WEEK) != 1) //not first day of week
		return;

	//town growths and War Machines Factories are handled separately
	if(ID == Obj::TOWN  ||  ID == Obj::WAR_MACHINE_FACTORY)
		return;

	if(ID == Obj::REFUGEE_CAMP) //if it's a refugee camp, we need to pick an available creature
	{
		cb->setObjProperty(id, ObjProperty::AVAILABLE_CREATURE, VLC->creh->pickRandomMonster(rand));
	}

	bool change = false;

	SetAvailableCreatures sac;
	sac.creatures = creatures;
	sac.tid = id;
	for (size_t i = 0; i < creatures.size(); i++)
	{
		if(!creatures[i].second.empty())
		{
			CCreature *cre = VLC->creh->objects[creatures[i].second[0]];
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
		if (VLC->creh->objects[creatureEntry.second.at(0)]->level >= 5 && ID != Obj::REFUGEE_CAMP)
		{
			guarded = true;
			break;
		}
	}

	if (guarded)
	{
		for (auto creatureEntry : creatures)
		{
			const CCreature * crea = VLC->creh->objects[creatureEntry.second.at(0)];
			SlotID slot = getSlotFor(crea->idNumber);

			if (hasStackAtSlot(slot)) //stack already exists, overwrite it
			{
				ChangeStackCount csc;
				csc.army = this->id;
				csc.slot = slot;
				csc.count = crea->growth * 3;
				csc.absoluteValue = true;
				cb->sendAndApply(&csc);
			}
			else //slot is empty, create whole new stack
			{
				InsertNewStack ns;
				ns.army = this->id;
				ns.slot = slot;
				ns.type = crea->idNumber;
				ns.count = crea->growth * 3;
				cb->sendAndApply(&ns);
			}
		}
	}
}

void CGDwelling::heroAcceptsCreatures( const CGHeroInstance *h) const
{
	CreatureID crid = creatures[0].second[0];
	CCreature *crs = VLC->creh->objects[crid];
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

void CGDwelling::serializeJsonOptions(JsonSerializeFormat & handler)
{
	if(!handler.saving)
		initRandomObjectInfo();

	switch (ID)
	{
	case Obj::WAR_MACHINE_FACTORY:
	case Obj::REFUGEE_CAMP:
		//do nothing
		break;
	case Obj::RANDOM_DWELLING:
	case Obj::RANDOM_DWELLING_LVL:
	case Obj::RANDOM_DWELLING_FACTION:
		info->serializeJson(handler);
		//fall through
	default:
		serializeJsonOwner(handler);
		break;
	}
}

int CGTownInstance::getSightRadius() const //returns sight distance
{
	auto ret = CBuilding::HEIGHT_NO_TOWER;

	for(const auto & bid : builtBuildings)
	{
		if(bid.IsSpecialOrGrail())
		{
			auto height = town->buildings.at(bid)->height;
			if(ret < height)
				ret = height;
	}
}
	return ret;
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

	const CCreature *creature = VLC->creh->objects[creatures[level].second.back()];
	const int base = creature->growth;
	int castleBonus = 0;

	ret.entries.emplace_back(VLC->generaltexth->allTexts[590], base); // \n\nBasic growth %d"

	if (hasBuilt(BuildingID::CASTLE))
		ret.entries.emplace_back(subID, BuildingID::CASTLE, castleBonus = base);
	else if (hasBuilt(BuildingID::CITADEL))
		ret.entries.emplace_back(subID, BuildingID::CITADEL, castleBonus = base / 2);

	if(town->hordeLvl.at(0) == level)//horde 1
		if(hasBuilt(BuildingID::HORDE_1))
			ret.entries.emplace_back(subID, BuildingID::HORDE_1, creature->hordeGrowth);

	if(town->hordeLvl.at(1) == level)//horde 2
		if(hasBuilt(BuildingID::HORDE_2))
			ret.entries.emplace_back(subID, BuildingID::HORDE_2, creature->hordeGrowth);

	//statue-of-legion-like bonus: % to base+castle
	TConstBonusListPtr bonuses2 = getBonuses(Selector::type()(Bonus::CREATURE_GROWTH_PERCENT));
	for(const auto & b : *bonuses2)
	{
		const auto growth = b->val * (base + castleBonus) / 100;
		ret.entries.emplace_back(growth, b->Description(growth));
	}

	//other *-of-legion-like bonuses (%d to growth cumulative with grail)
	TConstBonusListPtr bonuses = getBonuses(Selector::type()(Bonus::CREATURE_GROWTH).And(Selector::subtype()(level)));
	for(const auto & b : *bonuses)
		ret.entries.emplace_back(b->val, b->Description());

	int dwellingBonus = 0;
	if(const PlayerState *p = cb->getPlayerState(tempOwner, false))
	{
		dwellingBonus = getDwellingBonus(creatures[level].second, p->dwellings);
	}
	if(dwellingBonus)
		ret.entries.emplace_back(VLC->generaltexth->allTexts[591], dwellingBonus); // \nExternal dwellings %+d

	if(hasBuilt(BuildingID::GRAIL)) //grail - +50% to ALL (so far added) growth
		ret.entries.emplace_back(subID, BuildingID::GRAIL, ret.totalGrowth() / 2);

	return ret;
}

int CGTownInstance::getDwellingBonus(const std::vector<CreatureID>& creatureIds, const std::vector<ConstTransitivePtr<CGDwelling> >& dwellings) const
{
	int totalBonus = 0;
	for (const auto& dwelling : dwellings)
	{
		for (const auto& creature : dwelling->creatures)
		{
			totalBonus += vstd::contains(creatureIds, creature.second[0]) ? 1 : 0;
		}
	}
	return totalBonus;
}

TResources CGTownInstance::dailyIncome() const
{
	TResources ret;
	for(const auto & p : town->buildings)
	{
		BuildingID buildingUpgrade;

		for(const auto & p2 : town->buildings)
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

CGTownInstance::CGTownInstance():
	IShipyard(this),
	IMarket(this),
	town(nullptr),
	builded(0),
	destroyed(0),
	identifier(0),
	alignment(0xff)
{
	this->setNodeType(CBonusSystemNode::TOWN);
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

	if (hasBuilt(BuildingSubID::LIBRARY))
		ret++;

	return ret;
}

bool CGTownInstance::needsLastStack() const
{
	return garrisonHero != nullptr;
}

void CGTownInstance::setOwner(const PlayerColor & player) const
{
	removeCapitols(player);
	cb->setOwner(this, player);
}

void CGTownInstance::onHeroVisit(const CGHeroInstance * h) const
{
	if(!cb->gameState()->getPlayerRelations( getOwner(), h->getOwner() ))//if this is enemy
	{
		if(armedGarrison() || visitingHero)
		{
			const CGHeroInstance * defendingHero = visitingHero ? visitingHero : garrisonHero;
			const CArmedInstance * defendingArmy = defendingHero ? (CArmedInstance *)defendingHero : this;
			const bool isBattleOutside = isBattleOutsideTown(defendingHero);

			if(!isBattleOutside && visitingHero && defendingHero == visitingHero)
			{
				//we have two approaches to merge armies: mergeGarrisonOnSiege() and used in the CGameHandler::garrisonSwap(ObjectInstanceID tid)
				auto * nodeSiege = defendingHero->whereShouldBeAttachedOnSiege(isBattleOutside);

				if(nodeSiege == (CBonusSystemNode *)this)
					cb->swapGarrisonOnSiege(this->id);

				const_cast<CGHeroInstance *>(defendingHero)->inTownGarrison = false; //hack to return visitor from garrison after battle
			}
			cb->startBattlePrimary(h, defendingArmy, getSightCenter(), h, defendingHero, false, (isBattleOutside ? nullptr : this));
		}
		else
		{
			auto heroColor = h->getOwner();
			onTownCaptured(heroColor);

			if(cb->gameState()->getPlayerStatus(heroColor) == EPlayerStatus::WINNER)
			{
				return; //we just won game, we do not need to perform any extra actions
				//TODO: check how does H3 behave, visiting town on victory can affect campaigns (spells learned, +1 stat building visited)
			}
			cb->heroVisitCastle(this, h);
		}
	}
	else if(h->visitablePos() == visitablePos())
	{
		bool commander_recover = h->commander && !h->commander->alive;
		if (commander_recover) // rise commander from dead
		{
			SetCommanderProperty scp;
			scp.heroid = h->id;
			scp.which = SetCommanderProperty::ALIVE;
			scp.amount = 1;
			cb->sendAndApply(&scp);
		}
		cb->heroVisitCastle(this, h);
		// TODO(vmarkovtsev): implement payment for rising the commander
		if (commander_recover) // info window about commander
		{
			InfoWindow iw;
			iw.player = h->tempOwner;
			iw.text << h->commander->getName();
			iw.components.emplace_back(*h->commander);
			cb->showInfoDialog(&iw);
		}
	}
	else
	{
		logGlobal->error("%s visits allied town of %s from different pos?", h->getNameTranslated(), name);
	}
}

void CGTownInstance::onHeroLeave(const CGHeroInstance * h) const
{
	//FIXME: find out why this issue appears on random maps
	if(visitingHero == h)
	{
		cb->stopHeroVisitCastle(this, h);
		logGlobal->trace("%s correctly left town %s", h->getNameTranslated(), name);
	}
	else
		logGlobal->warn("Warning, %s tries to leave the town %s but hero is not inside.", h->getNameTranslated(), name);
}

std::string CGTownInstance::getObjectName() const
{
	return name + ", " + town->faction->getNameTranslated();
}

bool CGTownInstance::townEnvisagesBuilding(BuildingSubID::EBuildingSubID subId) const
{
	return town->getBuildingType(subId) != BuildingID::NONE;
}

void CGTownInstance::initOverriddenBids()
{
	for(const auto & bid : builtBuildings)
	{
		const auto & overrideThem = town->buildings.at(bid)->overrideBids;

		for(const auto & overrideIt : overrideThem)
			overriddenBuildings.insert(overrideIt);
	}
}

bool CGTownInstance::isBonusingBuildingAdded(BuildingID::EBuildingID bid) const
{
	auto present = std::find_if(bonusingBuildings.begin(), bonusingBuildings.end(), [&](CGTownBuilding* building)
		{
			return building->getBuildingType().num == bid;
		});

	return present != bonusingBuildings.end();
}

//it does not check hasBuilt because this check is in the OnHeroVisit handler
void CGTownInstance::tryAddOnePerWeekBonus(BuildingSubID::EBuildingSubID subID)
{
	auto bid = town->getBuildingType(subID);

	if(bid != BuildingID::NONE && !isBonusingBuildingAdded(bid))
		bonusingBuildings.push_back(new COPWBonus(bid, subID, this));
}

void CGTownInstance::tryAddVisitingBonus(BuildingSubID::EBuildingSubID subID)
{
	auto bid = town->getBuildingType(subID);

	if(bid != BuildingID::NONE && !isBonusingBuildingAdded(bid))
		bonusingBuildings.push_back(new CTownBonus(bid, subID, this));
}

void CGTownInstance::addTownBonuses()
{
	for(const auto & kvp : town->buildings)
	{
		if(vstd::contains(overriddenBuildings, kvp.first))
			continue;

		if(kvp.second->IsVisitingBonus())
			bonusingBuildings.push_back(new CTownBonus(kvp.second->bid, kvp.second->subId, this));

		if(kvp.second->IsWeekBonus())
			bonusingBuildings.push_back(new COPWBonus(kvp.second->bid, kvp.second->subId, this));
	}
}

TDmgRange CGTownInstance::getTowerDamageRange() const
{
	assert(hasBuilt(BuildingID::CASTLE));

	// http://heroes.thelazy.net/wiki/Arrow_tower
	// base damage, irregardless of town level
	static constexpr int baseDamage = 6;
	// extra damage, for each building in town
	static constexpr int extraDamage = 1;

	const int minDamage = baseDamage + extraDamage * getTownLevel();

	return {
		minDamage,
		minDamage * 2
	};
}

TDmgRange CGTownInstance::getKeepDamageRange() const
{
	assert(hasBuilt(BuildingID::CITADEL));

	// http://heroes.thelazy.net/wiki/Arrow_tower
	// base damage, irregardless of town level
	static constexpr int baseDamage = 10;
	// extra damage, for each building in town
	static constexpr int extraDamage = 2;

	const int minDamage = baseDamage + extraDamage * getTownLevel();

	return {
		minDamage,
		minDamage * 2
	};
}

void CGTownInstance::deleteTownBonus(BuildingID::EBuildingID bid)
{
	size_t i = 0;
	CGTownBuilding * freeIt = nullptr;

	for(i = 0; i != bonusingBuildings.size(); i++)
	{
		if(bonusingBuildings[i]->getBuildingType() == bid)
		{
			freeIt = bonusingBuildings[i];
			break;
		}
	}
	if(freeIt == nullptr)
		return;

	auto building = town->buildings.at(bid);
	auto isVisitingBonus = building->IsVisitingBonus();
	auto isWeekBonus = building->IsWeekBonus();

	if(!isVisitingBonus && !isWeekBonus)
		return;

	bonusingBuildings.erase(bonusingBuildings.begin() + i);

	delete freeIt;
}

void CGTownInstance::initObj(CRandomGenerator & rand) ///initialize town structures
{
	blockVisit = true;

	if(townEnvisagesBuilding(BuildingSubID::PORTAL_OF_SUMMONING)) //Dungeon for example
		creatures.resize(GameConstants::CREATURES_PER_TOWN + 1);
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
	initOverriddenBids();
	addTownBonuses(); //add special bonuses from buildings to the bonusingBuildings vector.
	recreateBuildingsBonuses();
	updateAppearance();
}

bool CGTownInstance::hasBuiltInOldWay(ETownType::ETownType type, const BuildingID & bid) const
{
	return (this->town->faction != nullptr && this->town->faction->getIndex() == type && hasBuilt(bid));
}

void CGTownInstance::newTurn(CRandomGenerator & rand) const
{
	if (cb->getDate(Date::DAY_OF_WEEK) == 1) //reset on new week
	{
		//give resources if there's a Mystic Pond
		if (hasBuilt(BuildingSubID::MYSTIC_POND)
			&& cb->getDate(Date::DAY) != 1
			&& (tempOwner < PlayerColor::PLAYER_LIMIT)
			)
		{
			int resID = rand.nextInt(2, 5); //bonus to random rare resource
			resID = (resID==2)?1:resID;
			int resVal = rand.nextInt(1, 4);//with size 1..4
			cb->giveResource(tempOwner, static_cast<Res::ERes>(resID), resVal);
			cb->setObjProperty (id, ObjProperty::BONUS_VALUE_FIRST, resID);
			cb->setObjProperty (id, ObjProperty::BONUS_VALUE_SECOND, resVal);
		}

		const auto * manaVortex = getBonusingBuilding(BuildingSubID::MANA_VORTEX);

		if (manaVortex != nullptr)
			cb->setObjProperty(id, ObjProperty::STRUCTURE_CLEAR_VISITORS, manaVortex->indexOnTV); //reset visitors for Mana Vortex

		//get Mana Vortex or Stables bonuses
		//same code is in the CGameHandler::buildStructure method
		if (visitingHero != nullptr)
			cb->visitCastleObjects(this, visitingHero);

		if (garrisonHero != nullptr)
			cb->visitCastleObjects(this, garrisonHero);

		if (tempOwner == PlayerColor::NEUTRAL) //garrison growth for neutral towns
			{
				std::vector<SlotID> nativeCrits; //slots
				for(const auto & elem : Slots())
				{
					if (elem.second->type->faction == subID) //native
					{
						nativeCrits.push_back(elem.first); //collect matching slots
					}
				}
				if(!nativeCrits.empty())
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
						cb->changeStackType(sl, VLC->creh->objects[*c->upgrades.begin()]);
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
							count = VLC->creh->objects[c]->growth;

						{//no lower tiers or above current month

							if ((n = getSlotFor(c)).validSlot())
							{
								StackLocation sl(this, n);
								if (slotEmpty(n))
									cb->insertNewStack(sl, VLC->creh->objects[c], count);
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

	return cb->getPlayerRelations(tempOwner, color) != PlayerRelations::ENEMIES;
}

void CGTownInstance::getOutOffsets( std::vector<int3> &offsets ) const
{
	offsets = {int3(-1,2,0), int3(-3,2,0)};
}

void CGTownInstance::mergeGarrisonOnSiege() const
{
	auto getWeakestStackSlot = [&](ui64 powerLimit)
	{
		std::vector<SlotID> weakSlots;
		auto stacksList = visitingHero->stacks;
		std::pair<SlotID, CStackInstance *> pair;
		while(!stacksList.empty())
		{
			pair = *vstd::minElementByFun(stacksList, [&](const std::pair<SlotID, CStackInstance *> & elem) { return elem.second->getPower(); });
			if(powerLimit > pair.second->getPower() &&
				(weakSlots.empty() || pair.second->getPower() == visitingHero->getStack(weakSlots.front()).getPower()))
			{
				weakSlots.push_back(pair.first);
				stacksList.erase(pair.first);
			}
			else
				break;
		}

		if(!weakSlots.empty())
			return *std::max_element(weakSlots.begin(), weakSlots.end());

		return SlotID();
	};

	auto count = static_cast<int>(stacks.size());

	for(int i = 0; i < count; i++)
	{
		auto pair = *vstd::maxElementByFun(stacks, [&](const std::pair<SlotID, CStackInstance *> & elem)
		{
			ui64 power = elem.second->getPower();
			auto dst = visitingHero->getSlotFor(elem.second->getCreatureID());
			if(dst.validSlot() && visitingHero->hasStackAtSlot(dst))
				power += visitingHero->getStack(dst).getPower();

			return power;
		});
		auto dst = visitingHero->getSlotFor(pair.second->getCreatureID());
		if(dst.validSlot())
			cb->moveStack(StackLocation(this, pair.first), StackLocation(visitingHero, dst), -1);
		else
		{
			dst = getWeakestStackSlot(static_cast<int>(pair.second->getPower()));
			if(dst.validSlot())
				cb->swapStacks(StackLocation(this, pair.first), StackLocation(visitingHero, dst));
		}
	}
}

void CGTownInstance::removeCapitols(const PlayerColor & owner) const
{
	if (hasCapitol()) // search if there's an older capitol
	{
		PlayerState* state = cb->gameState()->getPlayerState(owner); //get all towns owned by player
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

void CGTownInstance::clearArmy() const
{
	while(!stacks.empty())
	{
		cb->eraseStack(StackLocation(this, stacks.begin()->first));
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
	if(!hasBuiltSomeTradeBuilding())
		return 0;

	const PlayerState *p = cb->getPlayerState(tempOwner);
	assert(p);

	int marketCount = 0;
	for(const CGTownInstance *t : p->towns)
		if(t->hasBuiltSomeTradeBuilding())
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
		return hasBuilt(BuildingSubID::ARTIFACT_MERCHANT);

	case EMarketMode::CREATURE_RESOURCE:
		return hasBuilt(BuildingSubID::FREELANCERS_GUILD);

	case EMarketMode::CREATURE_UNDEAD:
		return hasBuilt(BuildingSubID::CREATURE_TRANSFORMER);

	case EMarketMode::RESOURCE_SKILL:
		return hasBuilt(BuildingSubID::MAGIC_UNIVERSITY);
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
				ret.push_back(a->getId());
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
	town = (*VLC->townh)[subID]->town;
	randomizeArmy(subID);
	updateAppearance();
}

void CGTownInstance::updateAppearance()
{
	auto terrain = cb->gameState()->getTile(visitablePos())->terType->getId();
	//FIXME: not the best way to do this
	auto app = VLC->objtypeh->getHandlerFor(ID, subID)->getOverride(terrain, this);
	if (app)
		appearance = app;
}

std::string CGTownInstance::nodeName() const
{
	return "Town (" + (town ? town->faction->getNameTranslated() : "unknown") + ") of " +  name;
}

void CGTownInstance::deserializationFix()
{
	attachTo(townAndVis);

	//Hero is already handled by CGameState::attachArmedObjects

// 	if(visitingHero)
// 		visitingHero->attachTo(&townAndVis);
// 	if(garrisonHero)
// 		garrisonHero->attachTo(this);
}

void CGTownInstance::updateMoraleBonusFromArmy()
{
	auto b = getExportedBonusList().getFirst(Selector::sourceType()(Bonus::ARMY).And(Selector::type()(Bonus::MORALE)));
	if(!b)
	{
		b = std::make_shared<Bonus>(Bonus::PERMANENT, Bonus::MORALE, Bonus::ARMY, 0, -1);
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
	BonusList bl;
	getExportedBonusList().getBonuses(bl, Selector::sourceType()(Bonus::TOWN_STRUCTURE));

	for(const auto & b : bl)
		removeBonus(b);

	for(const auto & bid : builtBuildings)
	{
		if(vstd::contains(overriddenBuildings, bid)) //tricky! -> checks tavern only if no bratherhood of sword
			continue;

		auto building = town->buildings.at(bid);

		if(building->buildingBonuses.empty())
			continue;

		for(auto & bonus : building->buildingBonuses)
		{
			if(bonus->limiter && bonus->effectRange == Bonus::ONLY_ENEMY_ARMY) //ONLY_ENEMY_ARMY is only mark for OppositeSide limiter to avoid extra dynamic_cast.
			{
				auto bCopy = std::make_shared<Bonus>(*bonus); //just a copy of the shared_ptr has been changed and reassigned.
				bCopy->limiter = std::make_shared<OppositeSideLimiter>(this->getOwner());
				addNewBonus(bCopy);
				continue;
			}
			if(bonus->propagator != nullptr && bonus->propagator->getPropagatorType() == ALL_CREATURES)
				VLC->creh->addBonusForAllCreatures(bonus);
			else
				addNewBonus(bonus);
		}
	}
}

void CGTownInstance::setVisitingHero(CGHeroInstance *h)
{
	assert(!!visitingHero == !h);
	if(h)
	{
		PlayerState *p = cb->gameState()->getPlayerState(h->tempOwner);
		assert(p);
		h->detachFrom(*p);
		h->attachTo(townAndVis);
		visitingHero = h;
		h->visitedTown = this;
		h->inTownGarrison = false;
	}
	else
	{
		PlayerState *p = cb->gameState()->getPlayerState(visitingHero->tempOwner);
		visitingHero->visitedTown = nullptr;
		visitingHero->detachFrom(townAndVis);
		visitingHero->attachTo(*p);
		visitingHero = nullptr;
	}
}

void CGTownInstance::setGarrisonedHero(CGHeroInstance *h)
{
	assert(!!garrisonHero == !h);
	if(h)
	{
		PlayerState *p = cb->gameState()->getPlayerState(h->tempOwner);
		assert(p);
		h->detachFrom(*p);
		h->attachTo(*this);
		garrisonHero = h;
		h->visitedTown = this;
		h->inTownGarrison = true;
	}
	else
	{
		PlayerState *p = cb->gameState()->getPlayerState(garrisonHero->tempOwner);
		garrisonHero->visitedTown = nullptr;
		garrisonHero->inTownGarrison = false;
		garrisonHero->detachFrom(*this);
		garrisonHero->attachTo(*p);
		garrisonHero = nullptr;
	}
	updateMoraleBonusFromArmy(); //avoid giving morale bonus for same army twice
}

bool CGTownInstance::armedGarrison() const
{
	return !stacks.empty() || garrisonHero;
}

const CTown * CGTownInstance::getTown() const
{
    if(ID == Obj::RANDOM_TOWN)
		return VLC->townh->randomTown;
	else
	{
		if(nullptr == town)
		{
			return (*VLC->townh)[subID]->town;
		}
		else
			return town;
	}
}

int CGTownInstance::getTownLevel() const
{
	// count all buildings that are not upgrades
	int level = 0;

	for(const auto & bid : builtBuildings)
	{
		if(town->buildings.at(bid)->upgrade == BuildingID::NONE)
			level++;
	}
	return level;
}

CBonusSystemNode & CGTownInstance::whatShouldBeAttached()
{
	return townAndVis;
}

std::string CGTownInstance::getNameTranslated() const
{
	return name;
}

void CGTownInstance::setNameTranslated( const std::string & newName )
{
	name = newName;
}

const CArmedInstance * CGTownInstance::getUpperArmy() const
{
	if(garrisonHero)
		return garrisonHero;
	return this;
}

const CGTownBuilding * CGTownInstance::getBonusingBuilding(BuildingSubID::EBuildingSubID subId) const
{
	for(auto * const building : bonusingBuildings)
	{
		if(building->getBuildingSubtype() == subId)
			return building;
	}
	return nullptr;
}


bool CGTownInstance::hasBuiltSomeTradeBuilding() const
{
	for(const auto & bid : builtBuildings)
	{
		if(town->buildings.at(bid)->IsTradeBuilding())
			return true;
	}
	return false;
}

bool CGTownInstance::hasBuilt(BuildingSubID::EBuildingSubID buildingID) const
{
	for(const auto & bid : builtBuildings)
	{
		if(town->buildings.at(bid)->subId == buildingID)
			return true;
	}
	return false;
}

bool CGTownInstance::hasBuilt(const BuildingID & buildingID) const
{
	return vstd::contains(builtBuildings, buildingID);
}

bool CGTownInstance::hasBuilt(const BuildingID & buildingID, int townID) const
{
	if (townID == town->faction->getIndex() || townID == ETownType::ANY)
		return hasBuilt(buildingID);
	return false;
}

TResources CGTownInstance::getBuildingCost(const BuildingID & buildingID) const
{
	if (vstd::contains(town->buildings, buildingID))
		return town->buildings.at(buildingID)->resources;
	else
	{
		logGlobal->error("Town %s at %s has no possible building %d!", name, pos.toString(), buildingID.toEnum());
		return TResources();
	}

}

CBuilding::TRequired CGTownInstance::genBuildingRequirements(const BuildingID & buildID, bool deep) const
{
	const CBuilding * building = town->buildings.at(buildID);

	//TODO: find better solution to prevent infinite loops
	std::set<BuildingID> processed;

	std::function<CBuilding::TRequired::Variant(const BuildingID &)> dependTest =
	[&](const BuildingID & id) -> CBuilding::TRequired::Variant
	{
		const CBuilding * build = town->buildings.at(id);
		CBuilding::TRequired::OperatorAll requirements;

		if (!hasBuilt(id))
		{
			if (deep)
				requirements.expressions.emplace_back(id);
			else
				return id;
		}

		if(!vstd::contains(processed, id))
		{
			processed.insert(id);
			if (build->upgrade != BuildingID::NONE)
				requirements.expressions.push_back(dependTest(build->upgrade));

			requirements.expressions.push_back(build->requirements.morph(dependTest));
		}
		return requirements;
	};

	CBuilding::TRequired::OperatorAll requirements;
	if (building->upgrade != BuildingID::NONE)
	{
		const CBuilding * upgr = town->buildings.at(building->upgrade);

		requirements.expressions.push_back(dependTest(upgr->bid));
		processed.clear();
	}
	requirements.expressions.push_back(building->requirements.morph(dependTest));

	CBuilding::TRequired::Variant variant(requirements);
	CBuilding::TRequired ret(variant);
	ret.minimize();
	return ret;
}

void CGTownInstance::addHeroToStructureVisitors(const CGHeroInstance *h, si64 structureInstanceID ) const
{
	if(visitingHero == h)
		cb->setObjProperty(id, ObjProperty::STRUCTURE_ADD_VISITING_HERO, structureInstanceID); //add to visitors
	else if(garrisonHero == h)
		cb->setObjProperty(id, ObjProperty::STRUCTURE_ADD_GARRISONED_HERO, structureInstanceID); //then it must be garrisoned hero
	else
	{
		//should never ever happen
		logGlobal->error("Cannot add hero %s to visitors of structure # %d", h->getNameTranslated(), structureInstanceID);
		throw std::runtime_error("internal error");
	}
}

void CGTownInstance::battleFinished(const CGHeroInstance * hero, const BattleResult & result) const
{
	if(result.winner == BattleSide::ATTACKER)
	{
		clearArmy();
		onTownCaptured(hero->getOwner());
	}
}

void CGTownInstance::onTownCaptured(const PlayerColor & winner) const
{
	setOwner(winner);
	FoWChange fw;
	fw.player = winner;
	fw.mode = 1;
	cb->getTilesInRange(fw.tiles, getSightCenter(), getSightRadius(), winner, 1);
	cb->sendAndApply(& fw);
}

void CGTownInstance::afterAddToMap(CMap * map)
{
	if(ID == Obj::TOWN)
		map->towns.emplace_back(this);
}

void CGTownInstance::afterRemoveFromMap(CMap * map)
{
	if (ID == Obj::TOWN)
		vstd::erase_if_present(map->towns, this);
}

void CGTownInstance::reset()
{
	CGTownInstance::merchantArtifacts.clear();
	CGTownInstance::universitySkills.clear();
}

void CGTownInstance::serializeJsonOptions(JsonSerializeFormat & handler)
{
	CGObjectInstance::serializeJsonOwner(handler);
	CCreatureSet::serializeJson(handler, "army", 7);
	handler.serializeBool<ui8>("tightFormation", formation, 1, 0, 0);
	handler.serializeString("name", name);

	{
		auto decodeBuilding = [this](const std::string & identifier) -> si32
		{
			auto rawId = VLC->modh->identifiers.getIdentifier(CModHandler::scopeMap(), getTown()->getBuildingScope(), identifier);

			if(rawId)
				return rawId.get();
			else
				return -1;
		};

		auto encodeBuilding = [this](si32 index) -> std::string
		{
			return getTown()->buildings.at(BuildingID(index))->getJsonKey();
		};

		const std::set<si32> standard = getTown()->getAllBuildings();//by default all buildings are allowed
		JsonSerializeFormat::LICSet buildingsLIC(standard, decodeBuilding, encodeBuilding);

		if(handler.saving)
		{
			bool customBuildings = false;

			boost::logic::tribool hasFort(false);

			for(const BuildingID & id : forbiddenBuildings)
			{
				buildingsLIC.none.insert(id);
				customBuildings = true;
			}

			for(const BuildingID & id : builtBuildings)
			{
				if(id == BuildingID::DEFAULT)
					continue;

				const CBuilding * building = getTown()->buildings.at(id);

				if(building->mode == CBuilding::BUILD_AUTO)
					continue;

				if(id == BuildingID::FORT)
					hasFort = true;

				buildingsLIC.all.insert(id);
				customBuildings = true;
			}

			if(customBuildings)
				handler.serializeLIC("buildings", buildingsLIC);
			else
				handler.serializeBool("hasFort",hasFort);
		}
		else
		{
			handler.serializeLIC("buildings", buildingsLIC);

			builtBuildings.insert(BuildingID::VILLAGE_HALL);

			if(buildingsLIC.none.empty() && buildingsLIC.all.empty())
			{
				builtBuildings.insert(BuildingID::DEFAULT);

				bool hasFort = false;
				handler.serializeBool("hasFort",hasFort);
				if(hasFort)
					builtBuildings.insert(BuildingID::FORT);
			}
			else
			{
				for(const si32 item : buildingsLIC.none)
					forbiddenBuildings.insert(BuildingID(item));
				for(const si32 item : buildingsLIC.all)
					builtBuildings.insert(BuildingID(item));
			}
		}
	}

	{
		std::vector<bool> standard = VLC->spellh->getDefaultAllowed();
		JsonSerializeFormat::LIC spellsLIC(standard, SpellID::decode, SpellID::encode);

		if(handler.saving)
		{
			for(const SpellID & id : possibleSpells)
				spellsLIC.any[id.num] = true;

			for(const SpellID & id : obligatorySpells)
				spellsLIC.all[id.num] = true;
		}

		handler.serializeLIC("spells", spellsLIC);

		if(!handler.saving)
		{
			possibleSpells.clear();
			for(si32 idx = 0; idx < spellsLIC.any.size(); idx++)
			{
				if(spellsLIC.any[idx])
					possibleSpells.emplace_back(idx);
			}

			obligatorySpells.clear();
			for(si32 idx = 0; idx < spellsLIC.all.size(); idx++)
			{
				if(spellsLIC.all[idx])
					obligatorySpells.emplace_back(idx);
			}
		}
	}
}

PlayerColor CGTownBuilding::getOwner() const
{
	return town->getOwner();
}

int32_t CGTownBuilding::getObjGroupIndex() const
{
	return -1;
}

int32_t CGTownBuilding::getObjTypeIndex() const
{
	return 0;
}

int3 CGTownBuilding::visitablePos() const
{
	return town->visitablePos();
}

int3 CGTownBuilding::getPosition() const
{
	return town->getPosition();
}

COPWBonus::COPWBonus(const BuildingID & bid, BuildingSubID::EBuildingSubID subId, CGTownInstance * cgTown)
{
	bID = bid;
	bType = subId;
	town = cgTown;
	indexOnTV = static_cast<si32>(town->bonusingBuildings.size());
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
	if(town->hasBuilt(bID))
	{
		InfoWindow iw;
		iw.player = h->tempOwner;

		switch (this->bType)
		{
		case BuildingSubID::STABLES:
			if(!h->hasBonusFrom(Bonus::OBJECT, Obj::STABLES)) //does not stack with advMap Stables
			{
				GiveBonus gb;
				gb.bonus = Bonus(Bonus::ONE_WEEK, Bonus::LAND_MOVEMENT, Bonus::OBJECT, 600, 94, VLC->generaltexth->arraytxt[100]);
				gb.id = heroID.getNum();
				cb->giveHeroBonus(&gb);

				SetMovePoints mp;
				mp.val = 600;
				mp.absolute = false;
				mp.hid = heroID;
				cb->setMovePoints(&mp);

				iw.text << VLC->generaltexth->allTexts[580];
				cb->showInfoDialog(&iw);
			}
			break;

		case BuildingSubID::MANA_VORTEX:
			if(visitors.empty())
			{
				if(h->mana < h->manaLimit() * 2)
					cb->setManaPoints (heroID, 2 * h->manaLimit());
				//TODO: investigate line below
				//cb->setObjProperty (town->id, ObjProperty::VISITED, true);
				iw.text << getVisitingBonusGreeting();
				cb->showInfoDialog(&iw);
				//extra visit penalty if hero alredy had double mana points (or even more?!)
				town->addHeroToStructureVisitors(h, indexOnTV);
			}
			break;
		}
	}
}
CTownBonus::CTownBonus(const BuildingID & index, BuildingSubID::EBuildingSubID subId, CGTownInstance * cgTown)
{
	bID = index;
	bType = subId;
	town = cgTown;
	indexOnTV = static_cast<si32>(town->bonusingBuildings.size());
}

void CTownBonus::setProperty (ui8 what, ui32 val)
{
	if(what == ObjProperty::VISITORS)
		visitors.insert(ObjectInstanceID(val));
}

void CTownBonus::onHeroVisit (const CGHeroInstance * h) const
{
	ObjectInstanceID heroID = h->id;
	if(town->hasBuilt(bID) && visitors.find(heroID) == visitors.end())
	{
		si64 val = 0;
		InfoWindow iw;
		PrimarySkill::PrimarySkill what = PrimarySkill::NONE;

		switch(bType)
		{
		case BuildingSubID::KNOWLEDGE_VISITING_BONUS: //wall of knowledge
			what = PrimarySkill::KNOWLEDGE;
			val = 1;
			iw.components.emplace_back(Component::PRIM_SKILL, 3, 1, 0);
			break;

		case BuildingSubID::SPELL_POWER_VISITING_BONUS: //order of fire
			what = PrimarySkill::SPELL_POWER;
			val = 1;
			iw.components.emplace_back(Component::PRIM_SKILL, 2, 1, 0);
			break;

		case BuildingSubID::ATTACK_VISITING_BONUS: //hall of Valhalla
			what = PrimarySkill::ATTACK;
			val = 1;
			iw.components.emplace_back(Component::PRIM_SKILL, 0, 1, 0);
			break;

		case BuildingSubID::EXPERIENCE_VISITING_BONUS: //academy of battle scholars
			what = PrimarySkill::EXPERIENCE;
			val = static_cast<int>(h->calculateXp(1000));
			iw.components.emplace_back(Component::EXPERIENCE, 0, val, 0);
			break;

		case BuildingSubID::DEFENSE_VISITING_BONUS: //cage of warlords
			what = PrimarySkill::DEFENSE;
			val = 1;
			iw.components.emplace_back(Component::PRIM_SKILL, 1, 1, 0);
			break;

		case BuildingSubID::CUSTOM_VISITING_BONUS:
			const auto building = town->town->buildings.at(bID);
			if(!h->hasBonusFrom(Bonus::TOWN_STRUCTURE, Bonus::getSid32(building->town->faction->getIndex(), building->bid)))
			{
				const auto & bonuses = building->onVisitBonuses;
				applyBonuses(const_cast<CGHeroInstance *>(h), bonuses);
			}
			break;
		}

		if(what != PrimarySkill::NONE)
		{
			iw.player = cb->getOwner(heroID);
				iw.text << getVisitingBonusGreeting();
			cb->showInfoDialog(&iw);
			cb->changePrimSkill (cb->getHero(heroID), what, val);
				town->addHeroToStructureVisitors(h, indexOnTV);
		}
	}
}

void CTownBonus::applyBonuses(CGHeroInstance * h, const BonusList & bonuses) const
{
	auto addToVisitors = false;

	for(const auto & bonus : bonuses)
	{
		GiveBonus gb;
		InfoWindow iw;

		if(bonus->type == Bonus::TOWN_MAGIC_WELL)
		{
			if(h->mana >= h->manaLimit())
				return;
			cb->setManaPoints(h->id, h->manaLimit());
			bonus->duration = Bonus::ONE_DAY;
		}
		gb.bonus = * bonus;
		gb.id = h->id.getNum();
		cb->giveHeroBonus(&gb);

		if(bonus->duration == Bonus::PERMANENT)
			addToVisitors = true;

		iw.player = cb->getOwner(h->id);
		iw.text << getCustomBonusGreeting(gb.bonus);
		cb->showInfoDialog(&iw);
	}
	if(addToVisitors)
		town->addHeroToStructureVisitors(h, indexOnTV);
}

GrowthInfo::Entry::Entry(const std::string &format, int _count)
	: count(_count)
{
	description = boost::str(boost::format(format) % count);
}

GrowthInfo::Entry::Entry(int subID, const BuildingID & building, int _count): count(_count)
{
	description = boost::str(boost::format("%s %+d") % (*VLC->townh)[subID]->town->buildings.at(building)->getNameTranslated() % count);
}

GrowthInfo::Entry::Entry(int _count, std::string fullDescription):
	count(_count),
	description(std::move(fullDescription))
{
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

std::string CGTownBuilding::getVisitingBonusGreeting() const
{
	auto bonusGreeting = town->town->getGreeting(bType);

	if(!bonusGreeting.empty())
		return bonusGreeting;

	switch(bType)
	{
	case BuildingSubID::MANA_VORTEX:
		bonusGreeting = std::string(VLC->generaltexth->translate("vcmi.townHall.greetingManaVortex"));
		break;
	case BuildingSubID::KNOWLEDGE_VISITING_BONUS:
		bonusGreeting = std::string(VLC->generaltexth->translate("vcmi.townHall.greetingKnowledge"));
		break;
	case BuildingSubID::SPELL_POWER_VISITING_BONUS:
		bonusGreeting = std::string(VLC->generaltexth->translate("vcmi.townHall.greetingSpellPower"));
		break;
	case BuildingSubID::ATTACK_VISITING_BONUS:
		bonusGreeting = std::string(VLC->generaltexth->translate("vcmi.townHall.greetingAttack"));
		break;
	case BuildingSubID::EXPERIENCE_VISITING_BONUS:
		bonusGreeting = std::string(VLC->generaltexth->translate("vcmi.townHall.greetingExperience"));
		break;
	case BuildingSubID::DEFENSE_VISITING_BONUS:
		bonusGreeting = std::string(VLC->generaltexth->translate("vcmi.townHall.greetingDefence"));
		break;
	}
	auto buildingName = town->town->getSpecialBuilding(bType)->getNameTranslated();

	if(bonusGreeting.empty())
	{
		bonusGreeting = "Error: Bonus greeting for '%s' is not localized.";
		logGlobal->error("'%s' building of '%s' faction has not localized bonus greeting.", buildingName, town->town->faction->getNameTranslated());
	}
	boost::algorithm::replace_first(bonusGreeting, "%s", buildingName);
	town->town->setGreeting(bType, bonusGreeting);
	return bonusGreeting;
}

std::string CGTownBuilding::getCustomBonusGreeting(const Bonus & bonus) const
{
	if(bonus.type == Bonus::TOWN_MAGIC_WELL)
	{
		auto bonusGreeting = std::string(VLC->generaltexth->translate("vcmi.townHall.greetingInTownMagicWell"));
		auto buildingName = town->town->getSpecialBuilding(bType)->getNameTranslated();
		boost::algorithm::replace_first(bonusGreeting, "%s", buildingName);
		return bonusGreeting;
	}
	auto bonusGreeting = std::string(VLC->generaltexth->translate("vcmi.townHall.greetingCustomBonus")); //"%s gives you +%d %s%s"
	std::string param;
	std::string until;

	if(bonus.type == Bonus::MORALE)
		param = VLC->generaltexth->allTexts[384];
	else if(bonus.type == Bonus::LUCK)
		param = VLC->generaltexth->allTexts[385];

	until = bonus.duration == static_cast<ui16>(Bonus::ONE_BATTLE) 
			? VLC->generaltexth->translate("vcmi.townHall.greetingCustomUntil")
			: ".";

	boost::format fmt = boost::format(bonusGreeting) % bonus.description % bonus.val % param % until;
	std::string greeting = fmt.str();
	return greeting;
}

VCMI_LIB_NAMESPACE_END
