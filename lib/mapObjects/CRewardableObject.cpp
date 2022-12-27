/*
 * CRewardableObject.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "CRewardableObject.h"
#include "../CHeroHandler.h"
#include "../CGeneralTextHandler.h"
#include "../CSoundBase.h"
#include "../NetPacks.h"
#include "../IGameCallback.h"
#include "../CGameState.h"
#include "../CPlayerState.h"

#include "CObjectClassesHandler.h"

VCMI_LIB_NAMESPACE_BEGIN

bool CRewardLimiter::heroAllowed(const CGHeroInstance * hero) const
{
	if(dayOfWeek != 0)
	{
		if (IObjectInterface::cb->getDate(Date::DAY_OF_WEEK) != dayOfWeek)
			return false;
	}

	for(auto & reqStack : creatures)
	{
		size_t count = 0;
		for (auto slot : hero->Slots())
		{
			const CStackInstance * heroStack = slot.second;
			if (heroStack->type == reqStack.type)
				count += heroStack->count;
		}
		if (count < reqStack.count) //not enough creatures of this kind
			return false;
	}

	if(!IObjectInterface::cb->getPlayerState(hero->tempOwner)->resources.canAfford(resources))
		return false;

	if(minLevel > (si32)hero->level)
		return false;

	for(size_t i=0; i<primary.size(); i++)
	{
		if (primary[i] > hero->getPrimSkillLevel(PrimarySkill::PrimarySkill(i)))
			return false;
	}

	for(auto & skill : secondary)
	{
		if (skill.second > hero->getSecSkillLevel(skill.first))
			return false;
	}

	for(auto & art : artifacts)
	{
		if (!hero->hasArt(art))
			return false;
	}

	return true;
}

std::vector<ui32> CRewardableObject::getAvailableRewards(const CGHeroInstance * hero) const
{
	std::vector<ui32> ret;

	for(size_t i=0; i<info.size(); i++)
	{
		const CVisitInfo & visit = info[i];

		if((visit.limiter.numOfGrants == 0 || visit.numOfGrants < visit.limiter.numOfGrants) // reward has unlimited uses or some are still available
			&& visit.limiter.heroAllowed(hero))
		{
			logGlobal->trace("Reward %d is allowed", i);
			ret.push_back(static_cast<ui32>(i));
		}
	}
	return ret;
}

CVisitInfo CRewardableObject::getVisitInfo(int index, const CGHeroInstance *) const
{
	return info[index];
}

void CRewardableObject::onHeroVisit(const CGHeroInstance *h) const
{
	auto grantRewardWithMessage = [&](int index) -> void
	{
		auto vi = getVisitInfo(index, h);
		logGlobal->debug("Granting reward %d. Message says: %s", index, vi.message.toString());
		// show message only if it is not empty
		if (!vi.message.toString().empty())
		{
			InfoWindow iw;
			iw.player = h->tempOwner;
			iw.text = vi.message;
			vi.reward.loadComponents(iw.components, h);
			cb->showInfoDialog(&iw);
		}
		// grant reward afterwards. Note that it may remove object
		grantReward(index, h);
	};
	auto selectRewardsMessage = [&](std::vector<ui32> rewards) -> void
	{
		BlockingDialog sd(canRefuse, rewards.size() > 1);
		sd.player = h->tempOwner;
		sd.text = onSelect;
		for (auto index : rewards)
			sd.components.push_back(getVisitInfo(index, h).reward.getDisplayedComponent(h));
		cb->showBlockingDialog(&sd);
	};

	if(!wasVisited(h))
	{
		auto rewards = getAvailableRewards(h);
		bool objectRemovalPossible = false;
		for(auto index : rewards)
		{
			if(getVisitInfo(index, h).reward.removeObject)
				objectRemovalPossible = true;
		}

		logGlobal->debug("Visiting object with %d possible rewards", rewards.size());
		switch (rewards.size())
		{
			case 0: // no available rewards, e.g. empty flotsam
			{
				InfoWindow iw;
				iw.player = h->tempOwner;
				if (!onEmpty.toString().empty())
					iw.text = onEmpty;
				else
					iw.text = onVisited;
				cb->showInfoDialog(&iw);
				break;
			}
			case 1: // one reward. Just give it with message
			{
				if (canRefuse)
					selectRewardsMessage(rewards);
				else
					grantRewardWithMessage(rewards[0]);
				break;
			}
			default: // multiple rewards. Act according to select mode
			{
				switch (selectMode) {
					case SELECT_PLAYER: // player must select
						selectRewardsMessage(rewards);
						break;
					case SELECT_FIRST: // give first available
						grantRewardWithMessage(rewards[0]);
						break;
					case SELECT_RANDOM: // select one randomly //TODO: use weights
						grantRewardWithMessage(rewards[CRandomGenerator::getDefault().nextInt((int)rewards.size()-1)]);
						break;
				}
				break;
			}
		}

		if(!objectRemovalPossible && getAvailableRewards(h).size() == 0)
		{
			ChangeObjectVisitors cov(ChangeObjectVisitors::VISITOR_ADD_TEAM, id, h->id);
			cb->sendAndApply(&cov);
		}
	}
	else
	{
		logGlobal->debug("Revisiting already visited object");
		InfoWindow iw;
		iw.player = h->tempOwner;
		if (!onVisited.toString().empty())
			iw.text = onVisited;
		else
			iw.text = onEmpty;
		cb->showInfoDialog(&iw);
	}
}

void CRewardableObject::heroLevelUpDone(const CGHeroInstance *hero) const
{
	grantRewardAfterLevelup(getVisitInfo(selectedReward, hero), hero);
}

void CRewardableObject::blockingDialogAnswered(const CGHeroInstance *hero, ui32 answer) const
{
	if(answer == 0)
		return; // player refused

	if(answer > 0 && answer-1 < info.size())
	{
		auto list = getAvailableRewards(hero);
		grantReward(list[answer - 1], hero);
	}
	else
	{
		throw std::runtime_error("Unhandled choice");
	}
}

void CRewardableObject::onRewardGiven(const CGHeroInstance * hero) const
{
	// no implementation, virtual function for overrides
}

void CRewardableObject::grantReward(ui32 rewardID, const CGHeroInstance * hero) const
{
	ChangeObjectVisitors cov(ChangeObjectVisitors::VISITOR_ADD, id, hero->id);
	cb->sendAndApply(&cov);
	cb->setObjProperty(id, ObjProperty::REWARD_SELECT, rewardID);

	grantRewardBeforeLevelup(getVisitInfo(rewardID, hero), hero);
}

void CRewardableObject::grantRewardBeforeLevelup(const CVisitInfo & info, const CGHeroInstance * hero) const
{
	assert(hero);
	assert(hero->tempOwner.isValidPlayer());
	assert(stacks.empty());
	assert(info.reward.creatures.size() <= GameConstants::ARMY_SIZE);
	assert(!cb->isVisitCoveredByAnotherQuery(this, hero));

	cb->giveResources(hero->tempOwner, info.reward.resources);

	for(auto & entry : info.reward.secondary)
	{
		int current = hero->getSecSkillLevel(entry.first);
		if( (current != 0 && current < entry.second) ||
			(hero->canLearnSkill() ))
		{
			cb->changeSecSkill(hero, entry.first, entry.second);
		}
	}

	for(int i=0; i< info.reward.primary.size(); i++)
		if(info.reward.primary[i] > 0)
			cb->changePrimSkill(hero, static_cast<PrimarySkill::PrimarySkill>(i), info.reward.primary[i], false);

	si64 expToGive = 0;
	expToGive += VLC->heroh->reqExp(hero->level+info.reward.gainedLevels) - VLC->heroh->reqExp(hero->level);
	expToGive += hero->calculateXp(info.reward.gainedExp);
	if(expToGive)
		cb->changePrimSkill(hero, PrimarySkill::EXPERIENCE, expToGive);

	// hero is not blocked by levelup dialog - grant remainer immediately
	if(!cb->isVisitCoveredByAnotherQuery(this, hero))
	{
		grantRewardAfterLevelup(info, hero);
	}
}

void CRewardableObject::grantRewardAfterLevelup(const CVisitInfo & info, const CGHeroInstance * hero) const
{
	if(info.reward.manaDiff || info.reward.manaPercentage >= 0)
	{
		si32 mana = hero->mana;
		if (info.reward.manaPercentage >= 0)
			mana = hero->manaLimit() * info.reward.manaPercentage / 100;

		cb->setManaPoints(hero->id, mana + info.reward.manaDiff);
	}

	if(info.reward.movePoints || info.reward.movePercentage >= 0)
	{
		SetMovePoints smp;
		smp.hid = hero->id;
		smp.val = hero->movement;

		if (info.reward.movePercentage >= 0) // percent from max
			smp.val = hero->maxMovePoints(hero->boat != nullptr) * info.reward.movePercentage / 100;
		smp.val = std::max<si32>(0, smp.val + info.reward.movePoints);

		cb->setMovePoints(&smp);
	}

	for(const Bonus & bonus : info.reward.bonuses)
	{
		assert(bonus.source == Bonus::OBJECT);
		assert(bonus.sid == ID);
		GiveBonus gb;
		gb.who = GiveBonus::HERO;
		gb.bonus = bonus;
		gb.id = hero->id.getNum();
		cb->giveHeroBonus(&gb);
	}

	for(ArtifactID art : info.reward.artifacts)
		cb->giveHeroNewArtifact(hero, VLC->arth->objects[art],ArtifactPosition::FIRST_AVAILABLE);

	if(!info.reward.spells.empty())
	{
		std::set<SpellID> spellsToGive(info.reward.spells.begin(), info.reward.spells.end());
		cb->changeSpells(hero, true, spellsToGive);
	}

	if(!info.reward.creatures.empty())
	{
		CCreatureSet creatures;
		for (auto & crea : info.reward.creatures)
			creatures.addToSlot(creatures.getFreeSlot(), new CStackInstance(crea.type, crea.count));

		cb->giveCreatures(this, hero, creatures, false);
	}

	onRewardGiven(hero);

	if(info.reward.removeObject)
		cb->removeObject(this);
}

bool CRewardableObject::wasVisited(PlayerColor player) const
{
	switch (visitMode)
	{
		case VISIT_UNLIMITED:
		case VISIT_BONUS:
			return false;
		case VISIT_ONCE:
			return vstd::contains(cb->getPlayerState(player)->visitedObjects, ObjectInstanceID(id));
		case VISIT_HERO:
			return false;
		case VISIT_PLAYER:
			return vstd::contains(cb->getPlayerState(player)->visitedObjects, ObjectInstanceID(id));
		default:
			return false;
	}
}

bool CRewardableObject::wasVisited(const CGHeroInstance * h) const
{
	switch (visitMode)
	{
		case VISIT_UNLIMITED:
			return false;
		case VISIT_BONUS:
			return h->hasBonusFrom(Bonus::OBJECT, ID);
		case VISIT_HERO:
			return h->visitedObjects.count(ObjectInstanceID(id));
		default:
			return wasVisited(h->tempOwner);
	}
}

void CRewardInfo::loadComponents(std::vector<Component> & comps,
                                 const CGHeroInstance * h) const
{
	for (auto comp : extraComponents)
		comps.push_back(comp);

	if (gainedExp)
	{
		comps.push_back(Component(
			Component::EXPERIENCE, 0, (si32)h->calculateXp(gainedExp), 0));
	}
	if (gainedLevels) comps.push_back(Component(Component::EXPERIENCE, 0, gainedLevels, 0));

	if (manaDiff) comps.push_back(Component(Component::PRIM_SKILL, 5, manaDiff, 0));

	for (size_t i=0; i<primary.size(); i++)
	{
		if (primary[i] != 0)
			comps.push_back(Component(Component::PRIM_SKILL, (ui16)i, primary[i], 0));
	}

	for (auto & entry : secondary)
		comps.push_back(Component(Component::SEC_SKILL, entry.first, entry.second, 0));

	for (auto & entry : artifacts)
		comps.push_back(Component(Component::ARTIFACT, entry, 1, 0));

	for (auto & entry : spells)
		comps.push_back(Component(Component::SPELL, entry, 1, 0));

	for (auto & entry : creatures)
		comps.push_back(Component(Component::CREATURE, entry.type->idNumber, entry.count, 0));

	for (size_t i=0; i<resources.size(); i++)
	{
		if (resources[i] !=0)
			comps.push_back(Component(Component::RESOURCE, (ui16)i, resources[i], 0));
	}
}

Component CRewardInfo::getDisplayedComponent(const CGHeroInstance * h) const
{
	std::vector<Component> comps;
	loadComponents(comps, h);
	assert(!comps.empty());
	return comps.front();
}

// FIXME: copy-pasted from CObjectHandler
static const std::string & visitedTxt(const bool visited)
{
	int id = visited ? 352 : 353;
	return VLC->generaltexth->allTexts[id];
}

std::string CRewardableObject::getHoverText(PlayerColor player) const
{
	if(visitMode == VISIT_PLAYER || visitMode == VISIT_ONCE)
		return getObjectName() + " " + visitedTxt(wasVisited(player));
	return getObjectName();
}

std::string CRewardableObject::getHoverText(const CGHeroInstance * hero) const
{
	if(visitMode != VISIT_UNLIMITED)
		return getObjectName() + " " + visitedTxt(wasVisited(hero));
	return getObjectName();
}

void CRewardableObject::setPropertyDer(ui8 what, ui32 val)
{
	switch (what)
	{
		case ObjProperty::REWARD_RESET:
			for (auto & visit : info)
				visit.numOfGrants = 0;
			break;
		case ObjProperty::REWARD_SELECT:
			selectedReward = val;
			info[val].numOfGrants++;
			break;
	}
}

void CRewardableObject::triggerRewardReset() const
{
	cb->setObjProperty(id, ObjProperty::REWARD_RESET, 0);
}

void CRewardableObject::newTurn(CRandomGenerator & rand) const
{
	if (resetDuration != 0 && cb->getDate(Date::DAY) > 1 && (cb->getDate(Date::DAY) % resetDuration) == 1)
		triggerRewardReset();
}

void CRewardableObject::initObj(CRandomGenerator & rand)
{
	VLC->objtypeh->getHandlerFor(ID, subID)->configureObject(this, rand);
}

CRewardableObject::CRewardableObject():
	selectMode(0),
	visitMode(0),
	selectedReward(0),
	resetDuration(0),
	canRefuse(false)
{}

///////////////////////////////////////////////////////////////////////////////////////////////////
///               END OF CODE FOR CREWARDABLEOBJECT AND RELATED CLASSES                         ///
///////////////////////////////////////////////////////////////////////////////////////////////////

/// Helper, selects random art class based on weights
static int selectRandomArtClass(CRandomGenerator & rand, int treasure, int minor, int major, int relic)
{
	int total = treasure + minor + major + relic;
	assert(total != 0);
	int hlp = rand.nextInt(total - 1);

	if(hlp < treasure)
		return CArtifact::ART_TREASURE;
	if(hlp < treasure + minor)
		return CArtifact::ART_MINOR;
	if(hlp < treasure + minor + major)
		return CArtifact::ART_MAJOR;
	return CArtifact::ART_RELIC;
}

/// Helper, adds random artifact to reward selecting class based on weights
static void loadRandomArtifact(CRandomGenerator & rand, CVisitInfo & info, int treasure, int minor, int major, int relic)
{
	int artClass = selectRandomArtClass(rand, treasure, minor, major, relic);
	ArtifactID artID = VLC->arth->pickRandomArtifact(rand, artClass);
	info.reward.artifacts.push_back(artID);
}

CGPickable::CGPickable()
{
	visitMode = VISIT_UNLIMITED;
	selectMode = SELECT_PLAYER;
}

void CGPickable::initObj(CRandomGenerator & rand)
{
	blockVisit = true;
	switch(ID)
	{
	case Obj::CAMPFIRE:
		{
			int givenRes = rand.nextInt(5);
			int givenAmm = rand.nextInt(4, 6);

			info.resize(1);
			info[0].reward.resources[givenRes] = givenAmm;
			info[0].reward.resources[Res::GOLD]= givenAmm * 100;
			info[0].message.addTxt(MetaString::ADVOB_TXT,23);
			info[0].reward.removeObject = true;
			break;
		}
	case Obj::FLOTSAM:
		{
			int type = rand.nextInt(3);
			switch(type)
			{
			case 0:
					info.resize(1);
					info[0].message.addTxt(MetaString::ADVOB_TXT, 51);
					info[0].reward.removeObject = true;
					break;
			case 1:
				{
					info.resize(1);
					info[0].reward.resources[Res::WOOD] = 5;
					info[0].message.addTxt(MetaString::ADVOB_TXT, 52);
					info[0].reward.removeObject = true;
					break;
				}
			case 2:
				{
					info.resize(1);
					info[0].reward.resources[Res::WOOD] = 5;
					info[0].reward.resources[Res::GOLD] = 200;
					info[0].message.addTxt(MetaString::ADVOB_TXT, 53);
					info[0].reward.removeObject = true;
					break;
				}
			case 3:
				{
					info.resize(1);
					info[0].reward.resources[Res::WOOD] = 10;
					info[0].reward.resources[Res::GOLD] = 500;
					info[0].message.addTxt(MetaString::ADVOB_TXT, 54);
					info[0].reward.removeObject = true;
					break;
				}
			}
			break;
		}
	case Obj::SEA_CHEST:
		{
			int hlp = rand.nextInt(99);
			if(hlp < 20)
			{
				info.resize(1);
				info[0].message.addTxt(MetaString::ADVOB_TXT, 116);
				info[0].reward.removeObject = true;
			}
			else if(hlp < 90)
			{
				info.resize(1);
				info[0].reward.resources[Res::GOLD] = 1500;
				info[0].message.addTxt(MetaString::ADVOB_TXT, 118);
				info[0].reward.removeObject = true;
			}
			else
			{
				info.resize(1);
				loadRandomArtifact(rand, info[0], 100, 0, 0, 0);
				info[0].reward.resources[Res::GOLD] = 1000;
				info[0].message.addTxt(MetaString::ADVOB_TXT, 117);
				info[0].message.addReplacement(MetaString::ART_NAMES, info[0].reward.artifacts.back());
				info[0].reward.removeObject = true;
			}
		}
		break;
	case Obj::SHIPWRECK_SURVIVOR:
		{
			info.resize(1);
			loadRandomArtifact(rand, info[0], 55, 20, 20, 5);
			info[0].message.addTxt(MetaString::ADVOB_TXT, 125);
			info[0].message.addReplacement(MetaString::ART_NAMES, info[0].reward.artifacts.back());
			info[0].reward.removeObject = true;
		}
		break;
	case Obj::TREASURE_CHEST:
		{
			int hlp = rand.nextInt(99);
			if(hlp >= 95)
			{
				info.resize(1);
				loadRandomArtifact(rand, info[0], 100, 0, 0, 0);
				info[0].message.addTxt(MetaString::ADVOB_TXT,145);
				info[0].message.addReplacement(MetaString::ART_NAMES, info[0].reward.artifacts.back());
				info[0].reward.removeObject = true;
				return;
			}
			else if (hlp >= 65)
			{
				onSelect.addTxt(MetaString::ADVOB_TXT,146);
				info.resize(2);
				info[0].reward.resources[Res::GOLD] = 2000;
				info[1].reward.gainedExp = 1500;
				info[0].reward.removeObject = true;
				info[1].reward.removeObject = true;
			}
			else if(hlp >= 33)
			{
				onSelect.addTxt(MetaString::ADVOB_TXT,146);
				info.resize(2);
				info[0].reward.resources[Res::GOLD] = 1500;
				info[1].reward.gainedExp = 1000;
				info[0].reward.removeObject = true;
				info[1].reward.removeObject = true;
			}
			else
			{
				onSelect.addTxt(MetaString::ADVOB_TXT,146);
				info.resize(2);
				info[0].reward.resources[Res::GOLD] = 1000;
				info[1].reward.gainedExp = 500;
				info[0].reward.removeObject = true;
				info[1].reward.removeObject = true;
			}
		}
		break;
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////

CGBonusingObject::CGBonusingObject()
{
	visitMode = VISIT_BONUS;
	selectMode = SELECT_FIRST;
}

void CGBonusingObject::initObj(CRandomGenerator & rand)
{
	auto configureBonusDuration = [&](CVisitInfo & visit, Bonus::BonusDuration duration, Bonus::BonusType type, si32 value, si32 descrID)
	{
		Bonus b(duration, type, Bonus::OBJECT, value, ID, descrID != 0 ? VLC->generaltexth->advobtxt[descrID] : "");
		visit.reward.bonuses.push_back(b);
		if (type == Bonus::MORALE)
			visit.reward.extraComponents.push_back(Component(Component::MORALE, 0, value, 0));
		if (type == Bonus::LUCK)
			visit.reward.extraComponents.push_back(Component(Component::LUCK, 0, value, 0));
	};

	auto configureBonus = [&](CVisitInfo & visit, Bonus::BonusType type, si32 value, si32 descrID)
	{
		configureBonusDuration(visit, Bonus::ONE_BATTLE, type, value, descrID);
	};

	auto configureMessage = [&](CVisitInfo & visit, int onGrantID, int onVisitedID)
	{
		visit.message.addTxt(MetaString::ADVOB_TXT, onGrantID);
		onVisited.addTxt(MetaString::ADVOB_TXT, onVisitedID);
	};

	info.resize(1);

	switch(ID)
	{
	case Obj::BUOY:
			blockVisit = true;
		configureMessage(info[0], 21, 22);
		configureBonus(info[0], Bonus::MORALE, +1, 94);
		break;
	case Obj::SWAN_POND:
		configureMessage(info[0], 29, 30);
		configureBonus(info[0], Bonus::LUCK, 2, 67);
		info[0].reward.movePercentage = 0;
		break;
	case Obj::FAERIE_RING:
		configureMessage(info[0], 49, 50);
		configureBonus(info[0], Bonus::LUCK, 1, 71);
		break;
	case Obj::FOUNTAIN_OF_FORTUNE:
		selectMode = SELECT_RANDOM;
		info.resize(5);
		for (int i=0; i<5; i++)
		{
			configureBonus(info[i], Bonus::LUCK, i-1, 69); //NOTE: description have %d that should be replaced with value
			info[i].message.addTxt(MetaString::ADVOB_TXT, 55);
		}
		onVisited.addTxt(MetaString::ADVOB_TXT, 56);
		break;
	case Obj::IDOL_OF_FORTUNE:

		info.resize(7);
		for (int i=0; i<6; i++)
		{
			info[i].limiter.dayOfWeek = i+1;
			configureBonus(info[i], (i%2) ? Bonus::MORALE : Bonus::LUCK, 1, 68);
			info[i].message.addTxt(MetaString::ADVOB_TXT, 62);
		}
		info.back().limiter.dayOfWeek = 7;
		configureBonus(info.back(), Bonus::MORALE, 1, 68); // on last day of week
		configureBonus(info.back(), Bonus::LUCK,   1, 68);
		configureMessage(info.back(), 62, 63);

		break;
	case Obj::MERMAID:
		blockVisit = true;
		configureMessage(info[0], 83, 82);
		configureBonus(info[0], Bonus::LUCK, 1, 72);
		break;
	case Obj::RALLY_FLAG:
		configureMessage(info[0], 111, 110);
		configureBonus(info[0], Bonus::MORALE, 1, 102);
		configureBonus(info[0], Bonus::LUCK,   1, 102);
		info[0].reward.movePoints = 400;
		break;
	case Obj::OASIS:
		configureMessage(info[0], 95, 94);
		configureBonus(info[0], Bonus::MORALE, 1, 95);
		info[0].reward.movePoints = 800;
		break;
	case Obj::TEMPLE:
		info[0].limiter.dayOfWeek = 7;
		info.resize(2);
		configureBonus(info[0], Bonus::MORALE, 2, 96);
		configureBonus(info[1], Bonus::MORALE, 1, 97);

		info[0].message.addTxt(MetaString::ADVOB_TXT, 140);
		info[1].message.addTxt(MetaString::ADVOB_TXT, 140);
		onVisited.addTxt(MetaString::ADVOB_TXT, 141);
		break;
	case Obj::WATERING_HOLE:
		configureMessage(info[0], 166, 167);
		configureBonus(info[0], Bonus::MORALE, 1, 100);
		info[0].reward.movePoints = 400;
		break;
	case Obj::FOUNTAIN_OF_YOUTH:
		configureMessage(info[0], 57, 58);
		configureBonus(info[0], Bonus::MORALE, 1, 103);
		info[0].reward.movePoints = 400;
		break;
	case Obj::STABLES:
		configureMessage(info[0], 137, 136);
		configureBonusDuration(info[0], Bonus::ONE_WEEK, Bonus::LAND_MOVEMENT, 400, 0);
		info[0].reward.movePoints = 400;
		break;
	}
}

CVisitInfo CGBonusingObject::getVisitInfo(int index, const CGHeroInstance *h) const
{
	if(ID == Obj::STABLES)
	{
		assert(index == 0);
		for(auto& slot : h->Slots())
		{
			if(slot.second->type->idNumber == CreatureID::CAVALIER)
			{
				CVisitInfo vi(info[0]);
				vi.message.clear();
				vi.message.addTxt(MetaString::ADVOB_TXT, 138);
				vi.reward.extraComponents.push_back(Component(
					Component::CREATURE, CreatureID::CHAMPION, 0, 1));
				return vi;
			}
		}
	}
	return info[index];
}

void CGBonusingObject::onHeroVisit(const CGHeroInstance *h) const
{
	CRewardableObject::onHeroVisit(h);
	if(ID == Obj::STABLES)
	{
		//regardless of whether this hero visited stables or not, cavaliers must be upgraded
		for(auto& slot : h->Slots())
		{
			if(slot.second->type->idNumber == CreatureID::CAVALIER)
			{
				cb->changeStackType(StackLocation(h, slot.first),
									VLC->creh->objects[CreatureID::CHAMPION]);
			}
		}
	}
}

bool CGBonusingObject::wasVisited(const CGHeroInstance * h) const
{
	if(ID == Obj::STABLES)
	{
		for(auto& slot : h->Slots())
		{
			if(slot.second->type->idNumber == CreatureID::CAVALIER)
			{
				// always display the reward message if the hero got cavaliers
				return false;
			}
		}
	}
	return CRewardableObject::wasVisited(h);
}

void CGBonusingObject::grantReward(ui32 rewardID, const CGHeroInstance * hero) const
{
	if(ID == Obj::STABLES && CRewardableObject::wasVisited(hero))
	{
		// reward message has been displayed - do not give the actual bonus
		return;
	}
	CRewardableObject::grantReward(rewardID, hero);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

CGOnceVisitable::CGOnceVisitable()
{
	visitMode = VISIT_ONCE;
	selectMode = SELECT_FIRST;
}

void CGOnceVisitable::initObj(CRandomGenerator & rand)
{
	switch(ID)
	{
	case Obj::CORPSE:
		{
			onEmpty.addTxt(MetaString::ADVOB_TXT, 38);
			blockVisit = true;
			if(rand.nextInt(99) < 20)
			{
				info.resize(1);
				loadRandomArtifact(rand, info[0], 10, 10, 10, 0);
				info[0].message.addTxt(MetaString::ADVOB_TXT, 37);
				info[0].limiter.numOfGrants = 1;
			}
		}
		break;
	case Obj::LEAN_TO:
		{
			onEmpty.addTxt(MetaString::ADVOB_TXT, 65);
			info.resize(1);
			int type =  rand.nextInt(5); //any basic resource without gold
			int value = rand.nextInt(1, 4);
			info[0].reward.resources[type] = value;
			info[0].message.addTxt(MetaString::ADVOB_TXT, 64);
			info[0].limiter.numOfGrants = 1;
		}
		break;
	case Obj::WARRIORS_TOMB:
		{
			onSelect.addTxt(MetaString::ADVOB_TXT, 161);
			onVisited.addTxt(MetaString::ADVOB_TXT, 163);

			info.resize(1);
			loadRandomArtifact(rand, info[0], 30, 50, 25, 5);

			Bonus bonus(Bonus::ONE_BATTLE, Bonus::MORALE, Bonus::OBJECT, -3, ID);
			info[0].reward.bonuses.push_back(bonus);
			info[0].limiter.numOfGrants = 1;
			info[0].message.addTxt(MetaString::ADVOB_TXT, 162);
			info[0].message.addReplacement(VLC->arth->objects[info[0].reward.artifacts.back()]->getName());
		}
		break;
	case Obj::WAGON:
		{
			onVisited.addTxt(MetaString::ADVOB_TXT, 156);

			int hlp = rand.nextInt(99);

			if(hlp < 40) //minor or treasure art
			{
				info.resize(1);
				loadRandomArtifact(rand, info[0], 10, 10, 0, 0);
				info[0].limiter.numOfGrants = 1;
				info[0].message.addTxt(MetaString::ADVOB_TXT, 155);
				info[0].message.addReplacement(VLC->arth->objects[info[0].reward.artifacts.back()]->getName());
			}
			else if(hlp < 90) //2 - 5 of non-gold resource
			{
				info.resize(1);
				int type  = rand.nextInt(5);
				int value = rand.nextInt(2, 5);
				info[0].reward.resources[type] = value;
				info[0].limiter.numOfGrants = 1;
				info[0].message.addTxt(MetaString::ADVOB_TXT, 154);
			}
			// or nothing
		}
		break;
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////

CGVisitableOPH::CGVisitableOPH()
{
	visitMode = VISIT_HERO;
	selectMode = SELECT_PLAYER;
}

void CGVisitableOPH::initObj(CRandomGenerator & rand)
{
	switch(ID)
	{
		case Obj::ARENA:
			info.resize(2);
			info[0].reward.primary[PrimarySkill::ATTACK] = 2;
			info[1].reward.primary[PrimarySkill::DEFENSE] = 2;
			onSelect.addTxt(MetaString::ADVOB_TXT, 0);
			onVisited.addTxt(MetaString::ADVOB_TXT, 1);
			canRefuse = true;
			break;
		case Obj::MERCENARY_CAMP:
			info.resize(1);
			info[0].reward.primary[PrimarySkill::ATTACK] = 1;
			info[0].message.addTxt(MetaString::ADVOB_TXT, 80);
			onVisited.addTxt(MetaString::ADVOB_TXT, 81);
			break;
		case Obj::MARLETTO_TOWER:
			info.resize(1);
			info[0].reward.primary[PrimarySkill::DEFENSE] = 1;
			info[0].message.addTxt(MetaString::ADVOB_TXT, 39);
			onVisited.addTxt(MetaString::ADVOB_TXT, 40);
			break;
		case Obj::STAR_AXIS:
			info.resize(1);
			info[0].reward.primary[PrimarySkill::SPELL_POWER] = 1;
			info[0].message.addTxt(MetaString::ADVOB_TXT, 100);
			onVisited.addTxt(MetaString::ADVOB_TXT, 101);
			break;
		case Obj::GARDEN_OF_REVELATION:
			info.resize(1);
			info[0].reward.primary[PrimarySkill::KNOWLEDGE] = 1;
			info[0].message.addTxt(MetaString::ADVOB_TXT, 59);
			onVisited.addTxt(MetaString::ADVOB_TXT, 60);
			break;
		case Obj::LEARNING_STONE:
			info.resize(1);
			info[0].reward.gainedExp = 1000;
			info[0].message.addTxt(MetaString::ADVOB_TXT, 143);
			onVisited.addTxt(MetaString::ADVOB_TXT, 144);
			break;
		case Obj::TREE_OF_KNOWLEDGE:
			info.resize(1);
			canRefuse = true;
			info[0].reward.gainedLevels = 1;
			onVisited.addTxt(MetaString::ADVOB_TXT, 147);
			info.resize(1);
			switch (rand.nextInt(2))
			{
			case 0: // free
				onSelect.addTxt(MetaString::ADVOB_TXT, 148);
				break;
			case 1:
				info[0].limiter.resources[Res::GOLD] = 2000;
				info[0].reward.resources[Res::GOLD] = -2000;
				onSelect.addTxt(MetaString::ADVOB_TXT, 149);
				onEmpty.addTxt(MetaString::ADVOB_TXT, 150);
				break;
			case 2:
				info[0].limiter.resources[Res::GEMS] = 10;
				info[0].reward.resources[Res::GEMS] = -10;
				onSelect.addTxt(MetaString::ADVOB_TXT, 151);
				onEmpty.addTxt(MetaString::ADVOB_TXT, 152);
				break;
			}
			break;
		case Obj::LIBRARY_OF_ENLIGHTENMENT:
		{
			selectMode = SELECT_FIRST;
			onVisited.addTxt(MetaString::ADVOB_TXT, 67);
			onEmpty.addTxt(MetaString::ADVOB_TXT, 68);

			CVisitInfo visit;
			visit.reward.primary[PrimarySkill::ATTACK] = 2;
			visit.reward.primary[PrimarySkill::DEFENSE] = 2;
			visit.reward.primary[PrimarySkill::KNOWLEDGE] = 2;
			visit.reward.primary[PrimarySkill::SPELL_POWER] = 2;
			visit.message.addTxt(MetaString::ADVOB_TXT, 66);

			static_assert(SecSkillLevel::LEVELS_SIZE == 4, "Behavior of Library of Enlignment may not be correct");
			for (int i=0; i<SecSkillLevel::LEVELS_SIZE; i++)
			{
				visit.limiter.minLevel = 10 - i * 2;
				visit.limiter.secondary[SecondarySkill::DIPLOMACY] = i;
				info.push_back(visit);
			}
			break;
		}
		case Obj::SCHOOL_OF_MAGIC:
			info.resize(2);
			info[0].reward.primary[PrimarySkill::SPELL_POWER] = 1;
			info[1].reward.primary[PrimarySkill::KNOWLEDGE] = 1;
			info[0].limiter.resources[Res::GOLD] = 1000;
			info[0].reward.resources[Res::GOLD] = -1000;
			info[1].limiter.resources[Res::GOLD] = 1000;
			info[1].reward.resources[Res::GOLD] = -1000;
			onSelect.addTxt(MetaString::ADVOB_TXT, 71);
			onVisited.addTxt(MetaString::ADVOB_TXT, 72);
			onEmpty.addTxt(MetaString::ADVOB_TXT, 73);
			canRefuse = true;
			break;
		case Obj::SCHOOL_OF_WAR:
			info.resize(2);
			info[0].reward.primary[PrimarySkill::ATTACK] = 1;
			info[1].reward.primary[PrimarySkill::DEFENSE] = 1;
			info[0].limiter.resources[Res::GOLD] = 1000;
			info[0].reward.resources[Res::GOLD] = -1000;
			info[1].limiter.resources[Res::GOLD] = 1000;
			info[1].reward.resources[Res::GOLD] = -1000;
			onSelect.addTxt(MetaString::ADVOB_TXT, 158);
			onVisited.addTxt(MetaString::ADVOB_TXT, 159);
			onEmpty.addTxt(MetaString::ADVOB_TXT, 160);
			canRefuse = true;
			break;
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////

CGVisitableOPW::CGVisitableOPW()
{
	visitMode = VISIT_ONCE;
	resetDuration = 7;
}

void CGVisitableOPW::triggerRewardReset() const
{
	CRewardableObject::triggerRewardReset();

	ChangeObjectVisitors cov(ChangeObjectVisitors::VISITOR_CLEAR, id);
	cb->sendAndApply(&cov);
}

void CGVisitableOPW::initObj(CRandomGenerator & rand)
{
	setRandomReward(rand);

	switch (ID)
	{
	case Obj::MYSTICAL_GARDEN:
		onEmpty.addTxt(MetaString::ADVOB_TXT, 93);
		info[0].message.addTxt(MetaString::ADVOB_TXT, 92);
		break;
	case Obj::WINDMILL:
		onEmpty.addTxt(MetaString::ADVOB_TXT, 169);
		info[0].message.addTxt(MetaString::ADVOB_TXT, 170);
		break;
	case Obj::WATER_WHEEL:
		onEmpty.addTxt(MetaString::ADVOB_TXT, 165);
		info[0].message.addTxt(MetaString::ADVOB_TXT, 164);
		break;
	}
}

void CGVisitableOPW::setPropertyDer(ui8 what, ui32 val)
{
	if(what == ObjProperty::REWARD_RESET)
	{
		setRandomReward(cb->gameState()->getRandomGenerator());

		if (ID == Obj::WATER_WHEEL)
		{
			auto& reward = info[0].reward.resources[Res::GOLD];
			if(cb->getDate() > 7)
			{
				reward = 1000;
			}
			else
			{
				reward = 500;
			}
		}
	}

	CRewardableObject::setPropertyDer(what, val);
}

void CGVisitableOPW::setRandomReward(CRandomGenerator &rand)
{
	switch (ID)
	{
	case Obj::MYSTICAL_GARDEN:
		info.resize(1);
		info[0].limiter.numOfGrants = 1;
		info[0].reward.resources.amin(0);
		if (rand.nextInt(1) == 0)
		{
			info[0].reward.resources[Res::GEMS] = 5;
		}
		else
		{
			info[0].reward.resources[Res::GOLD] = 500;
		}
		break;
	case Obj::WINDMILL:
		info.resize(1);
		info[0].reward.resources.amin(0);
		// 3-6 of any resource but wood and gold
		info[0].reward.resources[rand.nextInt(Res::MERCURY, Res::GEMS)] = rand.nextInt(3, 6);
		info[0].limiter.numOfGrants = 1;
		break;
	case Obj::WATER_WHEEL:
		info.resize(1);
		info[0].reward.resources.amin(0);
		info[0].reward.resources[Res::GOLD] = 500;
		info[0].limiter.numOfGrants = 1;
		break;
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////

void CGMagicSpring::initObj(CRandomGenerator & rand)
{
	CVisitInfo visit; // TODO: "player above max mana" limiter. Use logical expressions for limiters?
	visit.reward.manaPercentage = 200;
	visit.message.addTxt(MetaString::ADVOB_TXT, 74);
	info.push_back(visit); // two rewards, one for each entrance
	info.push_back(visit);
	onEmpty.addTxt(MetaString::ADVOB_TXT, 75);
}

std::vector<int3> CGMagicSpring::getVisitableOffsets() const
{
	std::vector <int3> visitableTiles;

	for(int y = 0; y < 6; y++)
		for (int x = 0; x < 8; x++) //starting from left
			if (appearance->isVisitableAt(x, y))
				visitableTiles.push_back (int3(x, y , 0));

	return visitableTiles;
}

int3 CGMagicSpring::getVisitableOffset() const
{
	auto visitableTiles = getVisitableOffsets();

	if (visitableTiles.size() != info.size())
	{
		logGlobal->warn("Unexpected number of visitable tiles of Magic Spring at %s", pos.toString());
		return int3(-1,-1,-1);
	}

	for (size_t i=0; i<visitableTiles.size(); i++)
	{
		if (info[i].numOfGrants == 0)
			return visitableTiles[i];
	}
	return visitableTiles[0]; // return *something*. This is valid visitable tile but already used
}

std::vector<ui32> CGMagicSpring::getAvailableRewards(const CGHeroInstance * hero) const
{
	auto tiles = getVisitableOffsets();
	for (size_t i=0; i<tiles.size(); i++)
	{
		if (pos - tiles[i] == hero->visitablePos() && info[i].numOfGrants == 0)
		{
			return std::vector<ui32>(1, (ui32)i);
		}
	}
	// hero is either not on visitable tile (should not happen) or tile is already used
	return std::vector<ui32>();
}

VCMI_LIB_NAMESPACE_END
