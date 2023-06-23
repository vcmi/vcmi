/*
 * InfoAboutArmy.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "InfoAboutArmy.h"

#include "../mapObjects/CGHeroInstance.h"
#include "../mapObjects/CGTownInstance.h"
#include "../CHeroHandler.h"

VCMI_LIB_NAMESPACE_BEGIN

ArmyDescriptor::ArmyDescriptor(const CArmedInstance *army, bool detailed)
	: isDetailed(detailed)
{
	for(const auto & elem : army->Slots())
	{
		if(detailed)
			(*this)[elem.first] = *elem.second;
		else
			(*this)[elem.first] = CStackBasicDescriptor(elem.second->type, (int)elem.second->getQuantityID());
	}
}

ArmyDescriptor::ArmyDescriptor()
	: isDetailed(false)
{

}

int ArmyDescriptor::getStrength() const
{
	ui64 ret = 0;
	if(isDetailed)
	{
		for(const auto & elem : *this)
			ret += elem.second.type->getAIValue() * elem.second.count;
	}
	else
	{
		for(const auto & elem : *this)
			ret += elem.second.type->getAIValue() * CCreature::estimateCreatureCount(elem.second.count);
	}
	return static_cast<int>(ret);
}

InfoAboutArmy::InfoAboutArmy():
	owner(PlayerColor::NEUTRAL)
{}

InfoAboutArmy::InfoAboutArmy(const CArmedInstance *Army, bool detailed)
{
	initFromArmy(Army, detailed);
}

void InfoAboutArmy::initFromArmy(const CArmedInstance *Army, bool detailed)
{
	army = ArmyDescriptor(Army, detailed);
	owner = Army->tempOwner;
	name = Army->getObjectName();
}

void InfoAboutHero::assign(const InfoAboutHero & iah)
{
	vstd::clear_pointer(details);
	InfoAboutArmy::operator = (iah);

	details = (iah.details ? new Details(*iah.details) : nullptr);
	hclass = iah.hclass;
	portrait = iah.portrait;
}

InfoAboutHero::InfoAboutHero(): portrait(-1) {}

InfoAboutHero::InfoAboutHero(const InfoAboutHero & iah): InfoAboutArmy(iah)
{
	assign(iah);
}

InfoAboutHero::InfoAboutHero(const CGHeroInstance * h, InfoAboutHero::EInfoLevel infoLevel):
	portrait(-1)
{
	initFromHero(h, infoLevel);
}

InfoAboutHero::~InfoAboutHero()
{
	vstd::clear_pointer(details);
}

InfoAboutHero & InfoAboutHero::operator=(const InfoAboutHero & iah)
{
	assign(iah);
	return *this;
}

void InfoAboutHero::initFromHero(const CGHeroInstance *h, InfoAboutHero::EInfoLevel infoLevel)
{
	vstd::clear_pointer(details);
	if(!h)
		return;

	bool detailed = ( (infoLevel == EInfoLevel::DETAILED) || (infoLevel == EInfoLevel::INBATTLE) );

	initFromArmy(h, detailed);

	hclass = h->type->heroClass;
	name = h->getNameTranslated();
	portrait = h->portrait;

	if(detailed)
	{
		//include details about hero
		details = new Details();
		details->luck = h->luckVal();
		details->morale = h->moraleVal();
		details->mana = h->mana;
		details->primskills.resize(GameConstants::PRIMARY_SKILLS);

		for (int i = 0; i < GameConstants::PRIMARY_SKILLS ; i++)
		{
			details->primskills[i] = h->getPrimSkillLevel(static_cast<PrimarySkill::PrimarySkill>(i));
		}
		if (infoLevel == EInfoLevel::INBATTLE)
			details->manaLimit = h->manaLimit();
		else
			details->manaLimit = -1; //we do not want to leak max mana info outside battle so set to meaningless value
	}
}

InfoAboutTown::InfoAboutTown():
	details(nullptr),
	tType(nullptr),
	built(0),
	fortLevel(0)
{

}

InfoAboutTown::InfoAboutTown(const CGTownInstance *t, bool detailed):
	details(nullptr),
	tType(nullptr),
	built(0),
	fortLevel(0)
{
	initFromTown(t, detailed);
}

InfoAboutTown::~InfoAboutTown()
{
	vstd::clear_pointer(details);
}

void InfoAboutTown::initFromTown(const CGTownInstance *t, bool detailed)
{
	initFromArmy(t, detailed);
	army = ArmyDescriptor(t->getUpperArmy(), detailed);
	built = t->builded;
	fortLevel = t->fortLevel();
	name = t->getNameTranslated();
	tType = t->getTown();

	vstd::clear_pointer(details);

	if(detailed)
	{
		//include details about hero
		details = new Details();
		TResources income = t->dailyIncome();
		details->goldIncome = income[EGameResID::GOLD];
		details->customRes = t->hasBuilt(BuildingID::RESOURCE_SILO);
		details->hallLevel = t->hallLevel();
		details->garrisonedHero = t->garrisonHero;
	}
}

VCMI_LIB_NAMESPACE_END
