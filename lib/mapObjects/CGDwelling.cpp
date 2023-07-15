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
#include "../serializer/JsonSerializeFormat.h"
#include "../mapObjectConstructors/AObjectTypeHandler.h"
#include "../mapObjectConstructors/CObjectClassesHandler.h"
#include "../CTownHandler.h"
#include "../IGameCallback.h"
#include "../gameState/CGameState.h"
#include "../CPlayerState.h"
#include "../NetPacks.h"
#include "../GameSettings.h"
#include "../CConfigHandler.h"

VCMI_LIB_NAMESPACE_BEGIN

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
	handler.serializeInt("minLevel", minLevel, static_cast<uint8_t>(1));
	handler.serializeInt("maxLevel", maxLevel, static_cast<uint8_t>(7));

	if(!handler.saving)
	{
		//todo: safely allow any level > 7
		vstd::abetween<uint8_t>(minLevel, 1, 7);
		vstd::abetween<uint8_t>(maxLevel, minLevel, 7);
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
		iw.type = EInfoWindowMode::AUTO;
		iw.player = h->tempOwner;
		iw.text.appendLocalString(EMetaText::ADVOB_TXT, 44); //{%s} \n\n The camp is deserted.  Perhaps you should try next week.
		iw.text.replaceLocalString(EMetaText::OBJ_NAMES, ID);
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
		bd.text.appendLocalString(EMetaText::GENERAL_TXT, 421); //Much to your dismay, the %s is guarded by %s %s. Do you wish to fight the guards?
		bd.text.replaceLocalString(ID == Obj::CREATURE_GENERATOR1 ? EMetaText::CREGENS : EMetaText::CREGENS4, subID);
		if(settings["gameTweaks"]["numericCreaturesQuantities"].Bool())
			bd.text.replaceRawString(CCreature::getQuantityRangeStringForId(Slots().begin()->second->getQuantityID()));
		else
			bd.text.replaceLocalString(EMetaText::ARRAY_TXT, 173 + (int)Slots().begin()->second->getQuantityID()*3);
		bd.text.replaceCreatureName(*Slots().begin()->second);
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
		bd.text.appendLocalString(EMetaText::ADVOB_TXT, ID == Obj::CREATURE_GENERATOR1 ? 35 : 36); //{%s} Would you like to recruit %s? / {%s} Would you like to recruit %s, %s, %s, or %s?
		bd.text.replaceLocalString(ID == Obj::CREATURE_GENERATOR1 ? EMetaText::CREGENS : EMetaText::CREGENS4, subID);
		for(const auto & elem : creatures)
			bd.text.replaceLocalString(EMetaText::CRE_PL_NAMES, elem.second[0]);
	}
	else if(ID == Obj::REFUGEE_CAMP)
	{
		bd.text.appendLocalString(EMetaText::ADVOB_TXT, 35); //{%s} Would you like to recruit %s?
		bd.text.replaceLocalString(EMetaText::OBJ_NAMES, ID);
		for(const auto & elem : creatures)
			bd.text.replaceLocalString(EMetaText::CRE_PL_NAMES, elem.second[0]);
	}
	else if(ID == Obj::WAR_MACHINE_FACTORY)
		bd.text.appendLocalString(EMetaText::ADVOB_TXT, 157); //{War Machine Factory} Would you like to purchase War Machines?
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
			bool creaturesAccumulate = false;

			if (tempOwner.isValidPlayer())
				creaturesAccumulate = VLC->settings()->getBoolean(EGameSettings::DWELLINGS_ACCUMULATE_WHEN_OWNED);
			else
				creaturesAccumulate = VLC->settings()->getBoolean(EGameSettings::DWELLINGS_ACCUMULATE_WHEN_NEUTRAL);

			CCreature *cre = VLC->creh->objects[creatures[i].second[0]];
			TQuantity amount = cre->getGrowth() * (1 + cre->valOfBonuses(BonusType::CREATURE_GROWTH_PERCENT)/100) + cre->valOfBonuses(BonusType::CREATURE_GROWTH);
			if (creaturesAccumulate && ID != Obj::REFUGEE_CAMP) //camp should not try to accumulate different kinds of creatures
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
		if (VLC->creatures()->getByIndex(creatureEntry.second.at(0))->getLevel() >= 5 && ID != Obj::REFUGEE_CAMP)
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
			SlotID slot = getSlotFor(crea->getId());

			if (hasStackAtSlot(slot)) //stack already exists, overwrite it
			{
				ChangeStackCount csc;
				csc.army = this->id;
				csc.slot = slot;
				csc.count = crea->getGrowth() * 3;
				csc.absoluteValue = true;
				cb->sendAndApply(&csc);
			}
			else //slot is empty, create whole new stack
			{
				InsertNewStack ns;
				ns.army = this->id;
				ns.slot = slot;
				ns.type = crea->getId();
				ns.count = crea->getGrowth() * 3;
				cb->sendAndApply(&ns);
			}
		}
	}
}

void CGDwelling::heroAcceptsCreatures( const CGHeroInstance *h) const
{
	CreatureID crid = creatures[0].second[0];
	auto *crs = crid.toCreature();
	TQuantity count = creatures[0].first;

	if(crs->getLevel() == 1  &&  ID != Obj::REFUGEE_CAMP) //first level - creatures are for free
	{
		if(count) //there are available creatures
		{

			if (VLC->settings()->getBoolean(EGameSettings::DWELLINGS_ACCUMULATE_WHEN_OWNED))
			{
				SlotID testSlot = h->getSlotFor(crid);
				if(!testSlot.validSlot()) //no available slot - try merging army of visiting hero
				{
					std::pair<SlotID, SlotID> toMerge;
					if (h->mergableStacks(toMerge))
					{
						cb->moveStack(StackLocation(h, toMerge.first), StackLocation(h, toMerge.second), -1); //merge toMerge.first into toMerge.second
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
				iw.text.replaceLocalString(EMetaText::CRE_PL_NAMES, crid);
				cb->showInfoDialog(&iw);
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
				iw.text.replaceLocalString(EMetaText::CRE_PL_NAMES, crid);

				cb->showInfoDialog(&iw);
				cb->sendAndApply(&sac);
				cb->addToSlot(StackLocation(h, slot), crs, count);
			}
		}
		else //there no creatures
		{
			InfoWindow iw;
			iw.type = EInfoWindowMode::AUTO;
			iw.text.appendLocalString(EMetaText::GENERAL_TXT, 422); //There are no %s here to recruit.
			iw.text.replaceLocalString(EMetaText::CRE_PL_NAMES, crid);
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
			? EOpenWindowMode::RECRUITMENT_FIRST
			: EOpenWindowMode::RECRUITMENT_ALL;
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

VCMI_LIB_NAMESPACE_END
