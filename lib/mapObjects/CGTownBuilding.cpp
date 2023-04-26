/*
 * CGTownBuilding.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "CGTownBuilding.h"
#include "CGTownInstance.h"
#include "../CGeneralTextHandler.h"
#include "../NetPacks.h"
#include "../IGameCallback.h"

VCMI_LIB_NAMESPACE_BEGIN

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

std::string CGTownBuilding::getVisitingBonusGreeting() const
{
	auto bonusGreeting = town->getTown()->getGreeting(bType);

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
	auto buildingName = town->getTown()->getSpecialBuilding(bType)->getNameTranslated();

	if(bonusGreeting.empty())
	{
		bonusGreeting = "Error: Bonus greeting for '%s' is not localized.";
		logGlobal->error("'%s' building of '%s' faction has not localized bonus greeting.", buildingName, town->getTown()->faction->getNameTranslated());
	}
	boost::algorithm::replace_first(bonusGreeting, "%s", buildingName);
	town->getTown()->setGreeting(bType, bonusGreeting);
	return bonusGreeting;
}

std::string CGTownBuilding::getCustomBonusGreeting(const Bonus & bonus) const
{
	if(bonus.type == Bonus::TOWN_MAGIC_WELL)
	{
		auto bonusGreeting = std::string(VLC->generaltexth->translate("vcmi.townHall.greetingInTownMagicWell"));
		auto buildingName = town->getTown()->getSpecialBuilding(bType)->getNameTranslated();
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
				gb.bonus = Bonus(Bonus::ONE_WEEK, Bonus::MOVEMENT, Bonus::OBJECT, 600, 94, VLC->generaltexth->arraytxt[100], 1);
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
			iw.components.emplace_back(Component::EComponentType::PRIM_SKILL, 3, 1, 0);
			break;

		case BuildingSubID::SPELL_POWER_VISITING_BONUS: //order of fire
			what = PrimarySkill::SPELL_POWER;
			val = 1;
			iw.components.emplace_back(Component::EComponentType::PRIM_SKILL, 2, 1, 0);
			break;

		case BuildingSubID::ATTACK_VISITING_BONUS: //hall of Valhalla
			what = PrimarySkill::ATTACK;
			val = 1;
			iw.components.emplace_back(Component::EComponentType::PRIM_SKILL, 0, 1, 0);
			break;

		case BuildingSubID::EXPERIENCE_VISITING_BONUS: //academy of battle scholars
			what = PrimarySkill::EXPERIENCE;
			val = static_cast<int>(h->calculateXp(1000));
			iw.components.emplace_back(Component::EComponentType::EXPERIENCE, 0, val, 0);
			break;

		case BuildingSubID::DEFENSE_VISITING_BONUS: //cage of warlords
			what = PrimarySkill::DEFENSE;
			val = 1;
			iw.components.emplace_back(Component::EComponentType::PRIM_SKILL, 1, 1, 0);
			break;

		case BuildingSubID::CUSTOM_VISITING_BONUS:
			const auto building = town->getTown()->buildings.at(bID);
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


VCMI_LIB_NAMESPACE_END
