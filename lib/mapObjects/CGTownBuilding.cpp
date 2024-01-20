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
#include "../IGameCallback.h"
#include "../gameState/CGameState.h"
#include "../mapObjects/CGHeroInstance.h"
#include "../networkPacks/PacksForClient.h"

VCMI_LIB_NAMESPACE_BEGIN

CGTownBuilding::CGTownBuilding(IGameCallback * cb)
	: IObjectInterface(cb)
	, town(nullptr)
{}

CGTownBuilding::CGTownBuilding(CGTownInstance * town)
	: IObjectInterface(town->cb)
	, town(town)
{}

PlayerColor CGTownBuilding::getOwner() const
{
	return town->getOwner();
}

MapObjectID CGTownBuilding::getObjGroupIndex() const
{
	return -1;
}

MapObjectSubID CGTownBuilding::getObjTypeIndex() const
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
	if(bonus.type == BonusType::TOWN_MAGIC_WELL)
	{
		auto bonusGreeting = std::string(VLC->generaltexth->translate("vcmi.townHall.greetingInTownMagicWell"));
		auto buildingName = town->getTown()->getSpecialBuilding(bType)->getNameTranslated();
		boost::algorithm::replace_first(bonusGreeting, "%s", buildingName);
		return bonusGreeting;
	}
	auto bonusGreeting = std::string(VLC->generaltexth->translate("vcmi.townHall.greetingCustomBonus")); //"%s gives you +%d %s%s"
	std::string param;
	std::string until;

	if(bonus.type == BonusType::MORALE)
		param = VLC->generaltexth->allTexts[384];
	else if(bonus.type == BonusType::LUCK)
		param = VLC->generaltexth->allTexts[385];

	until = bonus.duration == BonusDuration::ONE_BATTLE
			? VLC->generaltexth->translate("vcmi.townHall.greetingCustomUntil")
			: ".";

	boost::format fmt = boost::format(bonusGreeting) % bonus.description % bonus.val % param % until;
	std::string greeting = fmt.str();
	return greeting;
}

COPWBonus::COPWBonus(IGameCallback *cb)
	: CGTownBuilding(cb)
{}

COPWBonus::COPWBonus(const BuildingID & bid, BuildingSubID::EBuildingSubID subId, CGTownInstance * cgTown)
	: CGTownBuilding(cgTown)
{
	bID = bid;
	bType = subId;
	indexOnTV = static_cast<si32>(town->bonusingBuildings.size());
}

void COPWBonus::setProperty(ObjProperty what, ObjPropertyID identifier)
{
	switch (what)
	{
		case ObjProperty::VISITORS:
			visitors.insert(identifier.as<ObjectInstanceID>());
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
			if(!h->hasBonusFrom(BonusSource::OBJECT_TYPE, BonusSourceID(Obj(Obj::STABLES)))) //does not stack with advMap Stables
			{
				GiveBonus gb;
				gb.bonus = Bonus(BonusDuration::ONE_WEEK, BonusType::MOVEMENT, BonusSource::OBJECT_TYPE, 600, BonusSourceID(Obj(Obj::STABLES)), BonusCustomSubtype::heroMovementLand, VLC->generaltexth->arraytxt[100]);
				gb.id = heroID;
				cb->giveHeroBonus(&gb);

				cb->setMovePoints(heroID, 600, false);

				iw.text.appendRawString(VLC->generaltexth->allTexts[580]);
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
				iw.text.appendRawString(getVisitingBonusGreeting());
				cb->showInfoDialog(&iw);
				//extra visit penalty if hero alredy had double mana points (or even more?!)
				town->addHeroToStructureVisitors(h, indexOnTV);
			}
			break;
		}
	}
}

CTownBonus::CTownBonus(IGameCallback *cb)
	: CGTownBuilding(cb)
{}

CTownBonus::CTownBonus(const BuildingID & index, BuildingSubID::EBuildingSubID subId, CGTownInstance * cgTown)
	: CGTownBuilding(cgTown)
{
	bID = index;
	bType = subId;
	indexOnTV = static_cast<si32>(town->bonusingBuildings.size());
}

void CTownBonus::setProperty(ObjProperty what, ObjPropertyID identifier)
{
	if(what == ObjProperty::VISITORS)
		visitors.insert(identifier.as<ObjectInstanceID>());
}

void CTownBonus::onHeroVisit (const CGHeroInstance * h) const
{
	ObjectInstanceID heroID = h->id;
	if(town->hasBuilt(bID) && visitors.find(heroID) == visitors.end())
	{
		si64 val = 0;
		InfoWindow iw;
		PrimarySkill what = PrimarySkill::NONE;

		switch(bType)
		{
		case BuildingSubID::KNOWLEDGE_VISITING_BONUS: //wall of knowledge
			what = PrimarySkill::KNOWLEDGE;
			val = 1;
			iw.components.emplace_back(ComponentType::PRIM_SKILL, PrimarySkill::KNOWLEDGE, 1);
			break;

		case BuildingSubID::SPELL_POWER_VISITING_BONUS: //order of fire
			what = PrimarySkill::SPELL_POWER;
			val = 1;
			iw.components.emplace_back(ComponentType::PRIM_SKILL, PrimarySkill::SPELL_POWER, 1);
			break;

		case BuildingSubID::ATTACK_VISITING_BONUS: //hall of Valhalla
			what = PrimarySkill::ATTACK;
			val = 1;
			iw.components.emplace_back(ComponentType::PRIM_SKILL, PrimarySkill::ATTACK, 1);
			break;

		case BuildingSubID::EXPERIENCE_VISITING_BONUS: //academy of battle scholars
			what = PrimarySkill::EXPERIENCE;
			val = static_cast<int>(h->calculateXp(1000));
			iw.components.emplace_back(ComponentType::EXPERIENCE, val);
			break;

		case BuildingSubID::DEFENSE_VISITING_BONUS: //cage of warlords
			what = PrimarySkill::DEFENSE;
			val = 1;
			iw.components.emplace_back(ComponentType::PRIM_SKILL, PrimarySkill::DEFENSE, 1);
			break;

		case BuildingSubID::CUSTOM_VISITING_BONUS:
			const auto building = town->getTown()->buildings.at(bID);
			if(!h->hasBonusFrom(BonusSource::TOWN_STRUCTURE, BonusSourceID(building->getUniqueTypeID())))
			{
				const auto & bonuses = building->onVisitBonuses;
				applyBonuses(const_cast<CGHeroInstance *>(h), bonuses);
			}
			break;
		}

		if(what != PrimarySkill::NONE)
		{
			iw.player = cb->getOwner(heroID);
				iw.text.appendRawString(getVisitingBonusGreeting());
			cb->showInfoDialog(&iw);
			if (what == PrimarySkill::EXPERIENCE)
				cb->giveExperience(cb->getHero(heroID), val);
			else
				cb->changePrimSkill(cb->getHero(heroID), what, val);

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

		if(bonus->type == BonusType::TOWN_MAGIC_WELL)
		{
			if(h->mana >= h->manaLimit())
				return;
			cb->setManaPoints(h->id, h->manaLimit());
			bonus->duration = BonusDuration::ONE_DAY;
		}
		gb.bonus = * bonus;
		gb.id = h->id;
		cb->giveHeroBonus(&gb);

		if(bonus->duration == BonusDuration::PERMANENT)
			addToVisitors = true;

		iw.player = cb->getOwner(h->id);
		iw.text.appendRawString(getCustomBonusGreeting(gb.bonus));
		cb->showInfoDialog(&iw);
	}
	if(addToVisitors)
		town->addHeroToStructureVisitors(h, indexOnTV);
}

CTownRewardableBuilding::CTownRewardableBuilding(IGameCallback *cb)
	: CGTownBuilding(cb)
{}

CTownRewardableBuilding::CTownRewardableBuilding(const BuildingID & index, BuildingSubID::EBuildingSubID subId, CGTownInstance * cgTown, CRandomGenerator & rand)
	: CGTownBuilding(cgTown)
{
	bID = index;
	bType = subId;
	indexOnTV = static_cast<si32>(town->bonusingBuildings.size());
	initObj(rand);
}

void CTownRewardableBuilding::initObj(CRandomGenerator & rand)
{
	assert(town && town->town);

	auto building = town->town->buildings.at(bID);

	building->rewardableObjectInfo.configureObject(configuration, rand, cb);
	for(auto & rewardInfo : configuration.info)
	{
		for (auto & bonus : rewardInfo.reward.bonuses)
		{
			bonus.source = BonusSource::TOWN_STRUCTURE;
			bonus.sid = BonusSourceID(building->getUniqueTypeID());
		}
	}
}

void CTownRewardableBuilding::newTurn(CRandomGenerator & rand) const
{
	if (configuration.resetParameters.period != 0 && cb->getDate(Date::DAY) > 1 && ((cb->getDate(Date::DAY)-1) % configuration.resetParameters.period) == 0)
	{
		if(configuration.resetParameters.rewards)
		{
			cb->setObjPropertyValue(town->id, ObjProperty::REWARD_RANDOMIZE, indexOnTV);
		}
		if(configuration.resetParameters.visitors)
		{
			cb->setObjPropertyValue(town->id, ObjProperty::STRUCTURE_CLEAR_VISITORS, indexOnTV);
		}
	}
}

void CTownRewardableBuilding::setProperty(ObjProperty what, ObjPropertyID identifier)
{
	switch (what)
	{
		case ObjProperty::VISITORS:
			visitors.insert(identifier.as<ObjectInstanceID>());
			break;
		case ObjProperty::STRUCTURE_CLEAR_VISITORS:
			visitors.clear();
			break;
		case ObjProperty::REWARD_RANDOMIZE:
			initObj(cb->gameState()->getRandomGenerator());
			break;
		case ObjProperty::REWARD_SELECT:
			selectedReward = identifier.getNum();
			break;
	}
}

void CTownRewardableBuilding::heroLevelUpDone(const CGHeroInstance *hero) const
{
	grantRewardAfterLevelup(cb, configuration.info.at(selectedReward), town, hero);
}

void CTownRewardableBuilding::blockingDialogAnswered(const CGHeroInstance *hero, ui32 answer) const
{
	if(answer == 0)
		return; // player refused
	
	if(visitors.find(hero->id) != visitors.end())
		return; // query not for this building

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
			return !visitors.empty();
		case Rewardable::VISIT_PLAYER:
			return false; //not supported
		case Rewardable::VISIT_BONUS:
		{
			const auto building = town->getTown()->buildings.at(bID);
			return contextHero->hasBonusFrom(BonusSource::TOWN_STRUCTURE, BonusSourceID(building->getUniqueTypeID()));
		}
		case Rewardable::VISIT_HERO:
			return visitors.find(contextHero->id) != visitors.end();
		case Rewardable::VISIT_LIMITER:
			return configuration.visitLimiter.heroAllowed(contextHero);
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
		iw.type = EInfoWindowMode::MODAL;
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
