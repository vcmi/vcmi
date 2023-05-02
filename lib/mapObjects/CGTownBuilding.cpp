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
#include "../CGameState.h"

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

CTownRewardableBuilding::CTownRewardableBuilding(const BuildingID & index, BuildingSubID::EBuildingSubID subId, CGTownInstance * cgTown, CRandomGenerator & rand)
{
	bID = index;
	bType = subId;
	town = cgTown;
	indexOnTV = static_cast<si32>(town->bonusingBuildings.size());
	initObj(rand);
}

void CTownRewardableBuilding::initObj(CRandomGenerator & rand)
{
	assert(town && town->town);
	town->town->buildings.at(bID)->rewardableObjectInfo.configureObject(configuration, rand);
	for(auto & rewardInfo : configuration.info)
	{
		for (auto & bonus : rewardInfo.reward.bonuses)
		{
			bonus.source = Bonus::TOWN_STRUCTURE;
			bonus.sid = bID;
			if (bonus.type == Bonus::MORALE)
				rewardInfo.reward.extraComponents.emplace_back(Component::EComponentType::MORALE, 0, bonus.val, 0);
			if (bonus.type == Bonus::LUCK)
				rewardInfo.reward.extraComponents.emplace_back(Component::EComponentType::LUCK, 0, bonus.val, 0);
		}
	}
}

void CTownRewardableBuilding::newTurn(CRandomGenerator & rand) const
{
	if (configuration.resetParameters.period != 0 && cb->getDate(Date::DAY) > 1 && ((cb->getDate(Date::DAY)-1) % configuration.resetParameters.period) == 0)
	{
		if(configuration.resetParameters.rewards)
		{
			cb->setObjProperty(town->id, ObjProperty::REWARD_RANDOMIZE, indexOnTV);
		}
		if(configuration.resetParameters.visitors)
		{
			cb->setObjProperty(town->id, ObjProperty::REWARD_CLEARED, indexOnTV);
			cb->setObjProperty(town->id, ObjProperty::STRUCTURE_CLEAR_VISITORS, indexOnTV);
		}
	}
}

void CTownRewardableBuilding::setProperty(ui8 what, ui32 val)
{
	switch (what)
	{
		case ObjProperty::VISITORS:
			visitors.insert(ObjectInstanceID(val));
			break;
		case ObjProperty::STRUCTURE_CLEAR_VISITORS:
			visitors.clear();
			break;
		case ObjProperty::REWARD_RANDOMIZE:
			initObj(cb->gameState()->getRandomGenerator());
			break;
		case ObjProperty::REWARD_SELECT:
			selectedReward = val;
			break;
		case ObjProperty::REWARD_CLEARED:
			onceVisitableObjectCleared = val;
			break;
	}
}

void CTownRewardableBuilding::heroLevelUpDone(const CGHeroInstance *hero) const
{
	grantRewardAfterLevelup(cb, configuration.info.at(selectedReward), town, hero);
}

void CTownRewardableBuilding::blockingDialogAnswered(const CGHeroInstance *hero, ui32 answer) const
{
	if(visitors.find(hero->id) != visitors.end())
		return; // query not for this building
	
	if(answer == 0)
	{
		cb->setObjProperty(town->id, ObjProperty::STRUCTURE_CLEAR_VISITORS, indexOnTV);
		return; // player refused
	}

	if(answer > 0 && answer-1 < configuration.info.size())
	{
		auto list = getAvailableRewards(hero, Rewardable::EEventType::EVENT_FIRST_VISIT);
		grantReward(list[answer - 1], hero);
	}
	else
	{
		throw std::runtime_error("Unhandled choice");
	}
}

void CTownRewardableBuilding::grantReward(ui32 rewardID, const CGHeroInstance * hero) const
{
	town->addHeroToStructureVisitors(hero, indexOnTV);
	
	grantRewardBeforeLevelup(cb, configuration.info.at(rewardID), hero);
	
	// hero is not blocked by levelup dialog - grant remainer immediately
	if(!cb->isVisitCoveredByAnotherQuery(town, hero))
	{
		grantRewardAfterLevelup(cb, configuration.info.at(rewardID), town, hero);
	}
}

bool CTownRewardableBuilding::wasVisitedBefore(const CGHeroInstance * contextHero) const
{
	switch (configuration.visitMode)
	{
		case Rewardable::VISIT_UNLIMITED:
			return false;
		case Rewardable::VISIT_ONCE:
			return onceVisitableObjectCleared;
		case Rewardable::VISIT_PLAYER:
			return false; //not supported
		case Rewardable::VISIT_BONUS:
			return contextHero->hasBonusFrom(Bonus::TOWN_STRUCTURE, Bonus::getSid32(town->town->faction->getIndex(), bID));
		case Rewardable::VISIT_HERO:
			return visitors.find(contextHero->id) != visitors.end();
		default:
			return false;
	}
}

void CTownRewardableBuilding::onHeroVisit(const CGHeroInstance *h) const
{
	auto grantRewardWithMessage = [&](int index) -> void
	{
		auto vi = configuration.info.at(index);
		logGlobal->debug("Granting reward %d. Message says: %s", index, vi.message.toString());
		
		town->addHeroToStructureVisitors(h, indexOnTV); //adding to visitors

		InfoWindow iw;
		iw.player = h->tempOwner;
		iw.text = vi.message;
		vi.reward.loadComponents(iw.components, h);
		iw.type = configuration.infoWindowType;
		if(!iw.components.empty() || !iw.text.toString().empty())
			cb->showInfoDialog(&iw);
		
		grantReward(index, h);
	};
	auto selectRewardsMessage = [&](const std::vector<ui32> & rewards, const MetaString & dialog) -> void
	{
		BlockingDialog sd(configuration.canRefuse, rewards.size() > 1);
		sd.player = h->tempOwner;
		sd.text = dialog;

		if (rewards.size() > 1)
			for (auto index : rewards)
				sd.components.push_back(configuration.info.at(index).reward.getDisplayedComponent(h));

		if (rewards.size() == 1)
			configuration.info.at(rewards.front()).reward.loadComponents(sd.components, h);

		cb->showBlockingDialog(&sd);
	};
	
	if(!town->hasBuilt(bID) || cb->isVisitCoveredByAnotherQuery(town, h))
		return;

	if(!wasVisitedBefore(h))
	{
		auto rewards = getAvailableRewards(h, Rewardable::EEventType::EVENT_FIRST_VISIT);

		logGlobal->debug("Visiting object with %d possible rewards", rewards.size());
		switch (rewards.size())
		{
			case 0: // no available rewards, e.g. visiting School of War without gold
			{
				auto emptyRewards = getAvailableRewards(h, Rewardable::EEventType::EVENT_NOT_AVAILABLE);
				if (!emptyRewards.empty())
					grantRewardWithMessage(emptyRewards[0]);
				else
					logMod->warn("No applicable message for visiting empty object!");
				break;
			}
			case 1: // one reward. Just give it with message
			{
				if (configuration.canRefuse)
					selectRewardsMessage(rewards, configuration.info.at(rewards.front()).message);
				else
					grantRewardWithMessage(rewards.front());
				break;
			}
			default: // multiple rewards. Act according to select mode
			{
				switch (configuration.selectMode) {
					case Rewardable::SELECT_PLAYER: // player must select
						selectRewardsMessage(rewards, configuration.onSelect);
						break;
					case Rewardable::SELECT_FIRST: // give first available
						grantRewardWithMessage(rewards.front());
						break;
					case Rewardable::SELECT_RANDOM: // give random
						grantRewardWithMessage(*RandomGeneratorUtil::nextItem(rewards, cb->gameState()->getRandomGenerator()));
						break;
				}
				break;
			}
		}
	}
	else
	{
		logGlobal->debug("Revisiting already visited object");

		auto visitedRewards = getAvailableRewards(h, Rewardable::EEventType::EVENT_ALREADY_VISITED);
		if (!visitedRewards.empty())
			grantRewardWithMessage(visitedRewards[0]);
		else
			logMod->debug("No applicable message for visiting already visited object!");
	}
}


VCMI_LIB_NAMESPACE_END
