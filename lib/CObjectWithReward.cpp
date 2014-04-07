/*
 * CObjectWithReward.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "CObjectWithReward.h"
#include "CHeroHandler.h"
#include "CGeneralTextHandler.h"
#include "../client/CSoundBase.h"
#include "NetPacks.h"

bool CRewardLimiter::heroAllowed(const CGHeroInstance * hero) const
{
	if (dayOfWeek != 0)
	{
		if (IObjectInterface::cb->getDate(Date::DAY_OF_WEEK) != dayOfWeek)
			return false;
	}

	for (auto & reqStack : creatures)
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

	if (!IObjectInterface::cb->getPlayer(hero->tempOwner)->resources.canAfford(resources))
		return false;

	if (hero->level < minLevel)
		return false;

	for (size_t i=0; i<primary.size(); i++)
	{
		if (primary[i] < hero->getPrimSkillLevel(PrimarySkill::PrimarySkill(i)))
			return false;
	}

	for (auto & skill : secondary)
	{
		if (skill.second < hero->getSecSkillLevel(skill.first))
			return false;
	}

	for (auto & art : artifacts)
	{
		if (!hero->hasArt(art))
			return false;
	}

	return true;
}

std::vector<ui32> CObjectWithReward::getAvailableRewards(const CGHeroInstance * hero) const
{
	std::vector<ui32> ret;

	for (size_t i=0; i<info.size(); i++)
	{
		const CVisitInfo & visit = info[i];

		if (visit.numOfGrants < visit.limiter.numOfGrants && visit.limiter.heroAllowed(hero))
		{
			ret.push_back(i);
		}
	}
	return ret;
}

void CObjectWithReward::onHeroVisit(const CGHeroInstance *h) const
{
	if (!wasVisited(h))
	{
		auto rewards = getAvailableRewards(h);
		switch (rewards.size())
		{
			case 0: // no rewards, e.g. empty flotsam
			{
				InfoWindow iw;
				iw.player = h->tempOwner;
				iw.soundID = soundID;
				iw.text = onEmpty;
				cb->showInfoDialog(&iw);
				onRewardGiven(h);
				break;
			}
			case 1: // one reward. Just give it with message
			{
				grantReward(info[rewards[0]], h);
				InfoWindow iw;
				iw.player = h->tempOwner;
				iw.soundID = soundID;
				iw.text = onGrant;
				info[rewards[0]].reward.loadComponents(iw.components);
				cb->showInfoDialog(&iw);
				onRewardGiven(h);
				break;
			}
			default: // multiple rewards. Let player select
			{
				BlockingDialog sd(false,true);
				sd.player = h->tempOwner;
				sd.soundID = soundID;
				sd.text = onGrant;
				for (auto index : rewards)
					sd.components.push_back(info[index].reward.getDisplayedComponent());
				cb->showBlockingDialog(&sd);
				return;
			}
		}
	}
	else
	{
		InfoWindow iw;
		iw.player = h->tempOwner;
		iw.soundID = soundID;
		iw.text = onVisited;
		cb->showInfoDialog(&iw);
	}
}

void CObjectWithReward::heroLevelUpDone(const CGHeroInstance *hero) const
{
	grantRewardAfterLevelup(info[selectedReward], hero);
}

void CObjectWithReward::blockingDialogAnswered(const CGHeroInstance *hero, ui32 answer) const
{
	if (answer > 0 && answer-1 < info.size())
	{
		auto list = getAvailableRewards(hero);
		grantReward(info[list[answer - 1]], hero);
	}
	else
	{
		throw std::runtime_error("Unhandled choice");
	}
}

void CObjectWithReward::onRewardGiven(const CGHeroInstance * hero) const
{
	// no implementation, virtual function for overrides
}

void CObjectWithReward::grantReward(const CVisitInfo & info, const CGHeroInstance * hero) const
{
	assert(hero);
	assert(hero->tempOwner.isValidPlayer());
	assert(stacks.empty());
	assert(info.reward.creatures.size() <= GameConstants::ARMY_SIZE);
	assert(!cb->isVisitCoveredByAnotherQuery(this, hero));

	cb->giveResources(hero->tempOwner, info.reward.resources);

	for (auto & entry : info.reward.secondary)
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
	if (expToGive)
	{
		cb->changePrimSkill(hero, PrimarySkill::EXPERIENCE, expToGive);
	}
	else
	{
		grantRewardAfterLevelup(info, hero);
	}
}

void CObjectWithReward::grantRewardAfterLevelup(const CVisitInfo & info, const CGHeroInstance * hero) const
{
	if (info.reward.manaDiff || info.reward.manaPercentage >= 0)
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

	for (const Bonus & bonus : info.reward.bonuses)
	{
		GiveBonus gb;
		gb.bonus = bonus;
		gb.id = hero->id.getNum();
		cb->giveHeroBonus(&gb);
	}

	for (ArtifactID art : info.reward.artifacts)
		cb->giveHeroNewArtifact(hero, VLC->arth->artifacts[art],ArtifactPosition::FIRST_AVAILABLE);

	if (!info.reward.spells.empty())
	{
		std::set<SpellID> spellsToGive(info.reward.spells.begin(), info.reward.spells.end());
		cb->changeSpells(hero, true, spellsToGive);
	}

	if (!info.reward.creatures.empty())
	{
		CCreatureSet creatures;
		for (auto & crea : info.reward.creatures)
			creatures.addToSlot(creatures.getFreeSlot(), new CStackInstance(crea.type, crea.count));

		cb->giveCreatures(this, hero, creatures, false);
	}

	onRewardGiven(hero);
}

bool CObjectWithReward::wasVisited (PlayerColor player) const
{
	switch (visitMode)
	{
		case VISIT_UNLIMITED:
			return false;
		case VISIT_ONCE:
			for (auto & visit : info)
			{
				if (visit.numOfGrants != 0)
					return true;
			}
		case VISIT_HERO:
			return false;
		case VISIT_PLAYER:
			return vstd::contains(cb->getPlayer(player)->visitedObjects, ObjectInstanceID(ID));
		default:
			return false;
	}
}

bool CObjectWithReward::wasVisited (const CGHeroInstance * h) const
{
	switch (visitMode)
	{
		case VISIT_HERO:
			return vstd::contains(h->visitedObjects, ObjectInstanceID(ID));
		default:
			return wasVisited(h->tempOwner);
	}
}

void CRewardInfo::loadComponents(std::vector<Component> & comps) const
{
	for (size_t i=0; i<resources.size(); i++)
	{
		if (resources[i] !=0)
			comps.push_back(Component(Component::RESOURCE, i, resources[i], 0));
	}

	if (gainedExp)    comps.push_back(Component(Component::EXPERIENCE, 0, gainedExp, 0));
	if (gainedLevels) comps.push_back(Component(Component::EXPERIENCE, 0, gainedLevels, 0));

	if (manaDiff)   comps.push_back(Component(Component::PRIM_SKILL, 5, manaDiff,   0));

	for (size_t i=0; i<primary.size(); i++)
	{
		if (primary[i] !=0)
			comps.push_back(Component(Component::PRIM_SKILL, i, primary[i], 0));
	}

	for (auto & entry : secondary)
		comps.push_back(Component(Component::SEC_SKILL, entry.first, entry.second, 0));

	for (auto & entry : artifacts)
		comps.push_back(Component(Component::ARTIFACT, entry, 1, 0));

	for (auto & entry : spells)
		comps.push_back(Component(Component::SPELL, entry, 1, 0));

	for (auto & entry : creatures)
		comps.push_back(Component(Component::CREATURE, entry.type->idNumber, entry.count, 0));
}

Component CRewardInfo::getDisplayedComponent() const
{
	std::vector<Component> comps;
	loadComponents(comps);
	assert(!comps.empty());
	return comps.front();
}

// FIXME: copy-pasted from CObjectHandler
static std::string & visitedTxt(const bool visited)
{
	int id = visited ? 352 : 353;
	return VLC->generaltexth->allTexts[id];
}

const std::string & CObjectWithReward::getHoverText() const
{
	const CGHeroInstance *h = cb->getSelectedHero(cb->getCurrentPlayer());
	hoverName = VLC->generaltexth->names[ID];
	if(visitMode != VISIT_UNLIMITED)
	{
		bool visited = wasVisited(cb->getCurrentPlayer());
		if (h)
			visited |= wasVisited(h) || h->hasBonusFrom(Bonus::OBJECT,ID);

		hoverName += " " + visitedTxt(visited);
	}
	return hoverName;
}

void CObjectWithReward::setPropertyDer(ui8 what, ui32 val)
{
	switch (what)
	{
		case ObjProperty::REWARD_RESET:
			for (auto & visit : info)
				visit.numOfGrants = 0;
			break;
		case ObjProperty::REWARD_SELECT:
			selectedReward = val;
			break;
		case ObjProperty::REWARD_ADD_VISITOR:
		{
			//FIXME: Not sure if modifying another object here is a good idea
			CGHeroInstance * hero = cb->gameState()->getHero(ObjectInstanceID(val));
			assert(hero && hero->tempOwner.isValidPlayer());
			hero->visitedObjects.insert(ObjectInstanceID(ID));
			cb->gameState()->getPlayer(hero->tempOwner)->visitedObjects.insert(ObjectInstanceID(ID));
			break;
		}
	}
}

void CObjectWithReward::newTurn() const
{
	if (resetDuration != 0 && cb->getDate(Date::DAY) % resetDuration == 0)
		cb->setObjProperty(id, ObjProperty::REWARD_RESET, 0);
}

CObjectWithReward::CObjectWithReward():
	soundID(soundBase::invalid),
	selectMode(0),
	selectedReward(0),
	resetDuration(0)
{}

///////////////////////////////////////////////////////////////////////////////////////////////////
///               END OF CODE FOR COBJECTWITHREWARD AND RELATED CLASSES                         ///
///////////////////////////////////////////////////////////////////////////////////////////////////

/// Helper, selects random art class based on weights
static int selectRandomArtClass(int treasure, int minor, int major, int relic)
{
	int total = treasure + minor + major + relic;
	assert(total != 0);
	int hlp = IObjectInterface::cb->gameState()->getRandomGenerator().nextInt(total - 1);

	if(hlp < treasure)
		return CArtifact::ART_TREASURE;
	if(hlp < treasure + minor)
		return CArtifact::ART_MINOR;
	if(hlp < treasure + minor + major)
		return CArtifact::ART_MAJOR;
	return CArtifact::ART_RELIC;
}

/// Helper, adds random artifact to reward selecting class based on weights
static void loadRandomArtifact(CVisitInfo & info, int treasure, int minor, int major, int relic)
{
	int artClass = selectRandomArtClass(treasure, minor, major, relic);
	ArtifactID artID = VLC->arth->pickRandomArtifact(IObjectInterface::cb->gameState()->getRandomGenerator(), artClass);
	info.reward.artifacts.push_back(artID);
}

CGPickable::CGPickable()
{
	visitMode = VISIT_ONCE;
	selectMode = SELECT_PLAYER;
}

void CGPickable::initObj()
{
	blockVisit = true;
	switch(ID)
	{
	case Obj::CAMPFIRE:
		{
			soundID = soundBase::experience;
			onGrant.addTxt(MetaString::ADVOB_TXT,23);
			int givenRes = cb->gameState()->getRandomGenerator().nextInt(5);
			int givenAmm = cb->gameState()->getRandomGenerator().nextInt(4, 6);

			info.resize(1);
			info[0].reward.resources[givenRes] = givenAmm;
			info[0].reward.resources[Res::GOLD]= givenAmm * 100;
			break;
		}
	case Obj::FLOTSAM:
		{
			int type = cb->gameState()->getRandomGenerator().nextInt(3);
			soundID = soundBase::GENIE;
			if (type == 0)
				onEmpty.addTxt(MetaString::ADVOB_TXT, 51+type);
			else
				onGrant.addTxt(MetaString::ADVOB_TXT, 51+type);
			switch(type)
			{
			//case 0:
			case 1:
				{
					info.resize(1);
					info[0].reward.resources[Res::WOOD] = 5;
					break;
				}
			case 2:
				{
					info.resize(1);
					info[0].reward.resources[Res::WOOD] = 5;
					info[0].reward.resources[Res::GOLD] = 200;
					break;
				}
			case 3:
				{
					info.resize(1);
					info[0].reward.resources[Res::WOOD] = 10;
					info[0].reward.resources[Res::GOLD] = 500;
					break;
				}
			}
			break;
		}
	case Obj::SEA_CHEST:
		{
			soundID = soundBase::chest;
			int hlp = cb->gameState()->getRandomGenerator().nextInt(99);
			if(hlp < 20)
			{
				onGrant.addTxt(MetaString::ADVOB_TXT, 116);
			}
			else if(hlp < 90)
			{
				info.resize(1);
				info[0].reward.resources[Res::GOLD] = 1500;
				onGrant.addTxt(MetaString::ADVOB_TXT, 118);
			}
			else
			{
				info.resize(1);
				loadRandomArtifact(info[0], 100, 0, 0, 0);
				info[0].reward.resources[Res::GOLD] = 1000;
				onGrant.addTxt(MetaString::ADVOB_TXT, 117);
				onGrant.addReplacement(MetaString::ART_NAMES, info[0].reward.artifacts.back());
			}
		}
		break;
	case Obj::SHIPWRECK_SURVIVOR:
		{
			soundID = soundBase::experience;
			info.resize(1);
			loadRandomArtifact(info[0], 55, 20, 20, 5);
			onGrant.addTxt(MetaString::ADVOB_TXT, 125);
			onGrant.addReplacement(MetaString::ART_NAMES, info[0].reward.artifacts.back());
		}
		break;
	case Obj::TREASURE_CHEST:
		{
			int hlp = cb->gameState()->getRandomGenerator().nextInt(99);
			if(hlp >= 95)
			{
				soundID = soundBase::treasure;
				info.resize(1);
				loadRandomArtifact(info[0], 100, 0, 0, 0);
				onGrant.addTxt(MetaString::ADVOB_TXT,145);
				onGrant.addReplacement(MetaString::ART_NAMES, info[0].reward.artifacts.back());
				return;
			}
			else if (hlp >= 65)
			{
				soundID = soundBase::chest;
				onGrant.addTxt(MetaString::ADVOB_TXT,146);
				info.resize(2);
				info[0].reward.resources[Res::GOLD] = 2000;
				info[1].reward.gainedExp = 1500;
			}
			else if(hlp >= 33)
			{
				soundID = soundBase::chest;
				onGrant.addTxt(MetaString::ADVOB_TXT,146);
				info.resize(2);
				info[0].reward.resources[Res::GOLD] = 1500;
				info[1].reward.gainedExp = 1000;
			}
			else
			{
				soundID = soundBase::chest;
				onGrant.addTxt(MetaString::ADVOB_TXT,146);
				info.resize(2);
				info[0].reward.resources[Res::GOLD] = 1000;
				info[1].reward.gainedExp = 500;
			}
		}
		break;
	}
}

void CGPickable::onRewardGiven(const CGHeroInstance * hero) const
{
	cb->removeObject(this);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

CGBonusingObject::CGBonusingObject()
{
	visitMode = VISIT_UNLIMITED;
	selectMode = SELECT_FIRST;
}

void CGBonusingObject::initObj()
{
	auto configureBonusDuration = [&](CVisitInfo & visit, Bonus::BonusDuration duration, Bonus::BonusType type, si32 value, si32 descrID)
	{
		Bonus b(duration, type, Bonus::OBJECT, value, ID, descrID != 0 ? VLC->generaltexth->advobtxt[descrID] : "");
		visit.reward.bonuses.push_back(b);
	};

	auto configureBonus = [&](CVisitInfo & visit, Bonus::BonusType type, si32 value, si32 descrID)
	{
		configureBonusDuration(visit, Bonus::ONE_BATTLE, type, value, descrID);
	};

	auto configureMessage = [&](int onGrantID, int onVisitedID, soundBase::soundID sound)
	{
		onGrant.addTxt(MetaString::ADVOB_TXT, onGrantID);
		onVisited.addTxt(MetaString::ADVOB_TXT, onVisitedID);
		soundID = sound;
	};

	if(ID == Obj::BUOY || ID == Obj::MERMAID)
		blockVisit = true;

	info.resize(1);
	CVisitInfo & visit = info[0];

	switch(ID)
	{
	case Obj::BUOY:
		configureMessage(21, 22, soundBase::MORALE);
		configureBonus(visit, Bonus::MORALE, +1, 94);
		break;
	case Obj::SWAN_POND:
		configureMessage(29, 30, soundBase::LUCK);
		configureBonus(visit, Bonus::LUCK, 2, 67);
		visit.reward.movePercentage = 0;
		break;
	case Obj::FAERIE_RING:
		configureMessage(49, 50, soundBase::LUCK);
		configureBonus(visit, Bonus::LUCK, 2, 71);
		break;
	case Obj::FOUNTAIN_OF_FORTUNE:
		selectMode = SELECT_RANDOM;
		configureMessage(55, 56, soundBase::LUCK);
		info.resize(5);
		for (int i=0; i<5; i++)
			configureBonus(info[i], Bonus::LUCK, i-1, 69); //NOTE: description have %d that should be replaced with value
		break;
	case Obj::IDOL_OF_FORTUNE:

		configureMessage(62, 63, soundBase::experience);
		info.resize(7);
		for (int i=0; i<6; i++)
		{
			info[i].limiter.dayOfWeek = i+1;
			configureBonus(info[i], i%2 ? Bonus::MORALE : Bonus::LUCK, 1, 68);
		}
		info.back().limiter.dayOfWeek = 7;
		configureBonus(info.back(), Bonus::MORALE, 1, 68); // on last day of week
		configureBonus(info.back(), Bonus::LUCK,   1, 68);

		break;
	case Obj::MERMAID:
		configureMessage(83, 82, soundBase::LUCK);
		configureBonus(visit, Bonus::LUCK, 1, 72);
		break;
	case Obj::RALLY_FLAG:
		configureMessage(111, 110, soundBase::MORALE);
		configureBonus(visit, Bonus::MORALE, 1, 102);
		configureBonus(visit, Bonus::LUCK,   1, 102);
		visit.reward.movePoints = 400;
		break;
	case Obj::OASIS:
		configureMessage(95, 94, soundBase::MORALE);
		onGrant.addTxt(MetaString::ADVOB_TXT, 95);
		configureBonus(visit, Bonus::MORALE, 1, 95);
		visit.reward.movePoints = 800;
		break;
	case Obj::TEMPLE:
		configureMessage(140, 141, soundBase::temple);
		info[0].limiter.dayOfWeek = 7;
		info.resize(2);
		configureBonus(info[0], Bonus::MORALE, 2, 96);
		configureBonus(info[1], Bonus::MORALE, 1, 97);
		break;
	case Obj::WATERING_HOLE:
		configureMessage(166, 167, soundBase::MORALE);
		configureBonus(visit, Bonus::MORALE, 1, 100);
		visit.reward.movePoints = 400;
		break;
	case Obj::FOUNTAIN_OF_YOUTH:
		configureMessage(57, 58, soundBase::MORALE);
		configureBonus(visit, Bonus::MORALE, 1, 103);
		visit.reward.movePoints = 400;
		break;
	case Obj::STABLES:
		configureMessage(137, 136, soundBase::STORE);

		configureBonusDuration(visit, Bonus::ONE_WEEK, Bonus::LAND_MOVEMENT, 600, 0);
		visit.reward.movePoints = 600;
		//TODO: upgrade champions to cavaliers
/*
		bool someUpgradeDone = false;

		for (auto i = h->Slots().begin(); i != h->Slots().end(); ++i)
		{
			if(i->second->type->idNumber == CreatureID::CAVALIER)
			{
				cb->changeStackType(StackLocation(h, i->first), VLC->creh->creatures[CreatureID::CHAMPION]);
				someUpgradeDone = true;
			}
		}
		if (someUpgradeDone)
		{
			grantMessage.addTxt(MetaString::ADVOB_TXT, 138);
			iw.components.push_back(Component(Component::CREATURE,11,0,1));
		}*/
		break;
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////

CGOnceVisitable::CGOnceVisitable()
{
	visitMode = VISIT_ONCE;
	selectMode = SELECT_FIRST;
}

void CGOnceVisitable::initObj()
{
	switch(ID)
	{
	case Obj::CORPSE:
		{
			onGrant.addTxt(MetaString::ADVOB_TXT, 37);
			onEmpty.addTxt(MetaString::ADVOB_TXT, 38);
			soundID = soundBase::MYSTERY;
			blockVisit = true;
			if(cb->gameState()->getRandomGenerator().nextInt(99) < 20)
			{
				info.resize(1);
				loadRandomArtifact(info[0], 10, 10, 10, 0);
			}
		}
		break;

	case Obj::LEAN_TO:
		{
			soundID = soundBase::GENIE;
			onGrant.addTxt(MetaString::ADVOB_TXT, 64);
			onEmpty.addTxt(MetaString::ADVOB_TXT, 65);
			info.resize(1);
			int type =  cb->gameState()->getRandomGenerator().nextInt(5); //any basic resource without gold
			int value = cb->gameState()->getRandomGenerator().nextInt(1, 4);
			info[0].reward.resources[type] = value;
		}
		break;

	case Obj::WARRIORS_TOMB:
		{
			// TODO: line 161 - ask if player wants to search the Tomb
			soundID = soundBase::GRAVEYARD;
			onGrant.addTxt(MetaString::ADVOB_TXT, 162);
			onVisited.addTxt(MetaString::ADVOB_TXT, 163);

			info.resize(2);
			loadRandomArtifact(info[0], 30, 50, 25, 5);

			Bonus bonus(Bonus::ONE_BATTLE, Bonus::MORALE, Bonus::OBJECT, -3, ID);
			info[0].reward.bonuses.push_back(bonus);
			info[1].reward.bonuses.push_back(bonus);
		}
		break;
	case Obj::WAGON:
		{
			soundID = soundBase::GENIE;
			onVisited.addTxt(MetaString::ADVOB_TXT, 156);

			int hlp = cb->gameState()->getRandomGenerator().nextInt(99);

			if(hlp < 40) //minor or treasure art
			{
				onGrant.addTxt(MetaString::ADVOB_TXT, 155);
				info.resize(1);
				loadRandomArtifact(info[0], 10, 10, 0, 0);
			}
			else if(hlp < 90) //2 - 5 of non-gold resource
			{
				onGrant.addTxt(MetaString::ADVOB_TXT, 154);
				info.resize(1);
				int type  = cb->gameState()->getRandomGenerator().nextInt(5);
				int value = cb->gameState()->getRandomGenerator().nextInt(2, 5);
				info[0].reward.resources[type] = value;
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

void CGVisitableOPH::initObj()
{
	switch(ID)
	{
		case Obj::ARENA:
			soundID = soundBase::NOMAD;
			onGrant.addTxt(MetaString::ADVOB_TXT, 0);
			info.resize(2);
			info[0].reward.primary[PrimarySkill::ATTACK] = 2;
			info[1].reward.primary[PrimarySkill::DEFENSE] = 2;
			break;
		case Obj::MERCENARY_CAMP:
			info.resize(1);
			info[0].reward.primary[PrimarySkill::ATTACK] = 1;
			soundID = soundBase::NOMAD;
			onGrant.addTxt(MetaString::ADVOB_TXT, 80);
			break;
		case Obj::MARLETTO_TOWER:
			info.resize(1);
			info[0].reward.primary[PrimarySkill::DEFENSE] = 1;
			soundID = soundBase::NOMAD;
			onGrant.addTxt(MetaString::ADVOB_TXT, 39);
			break;
		case Obj::STAR_AXIS:
			info.resize(1);
			info[0].reward.primary[PrimarySkill::SPELL_POWER] = 1;
			soundID = soundBase::gazebo;
			onGrant.addTxt(MetaString::ADVOB_TXT, 100);
			break;
		case Obj::GARDEN_OF_REVELATION:
			info.resize(1);
			info[0].reward.primary[PrimarySkill::KNOWLEDGE] = 1;
			soundID = soundBase::GETPROTECTION;
			onGrant.addTxt(MetaString::ADVOB_TXT, 59);
			break;
		case Obj::LEARNING_STONE:
			info.resize(1);
			info[0].reward.gainedExp = 1000;
			soundID = soundBase::gazebo;
			onGrant.addTxt(MetaString::ADVOB_TXT, 143);
			break;
		case Obj::TREE_OF_KNOWLEDGE:
			soundID = soundBase::gazebo;
			info.resize(1);
			info[0].reward.gainedLevels = 1;

			info.resize(1);
			switch (cb->gameState()->getRandomGenerator().nextInt(2))
			{
			case 0: // free
				break;
			case 1:
				info[0].limiter.resources[Res::GOLD] = 2000;
				info[0].reward.resources[Res::GOLD] = -2000;
				break;
			case 2:
				info[0].limiter.resources[Res::GEMS] = 10;
				info[0].reward.resources[Res::GEMS] = -10;
				break;
			}
			break;
		case Obj::LIBRARY_OF_ENLIGHTENMENT:
		{
			onGrant.addTxt(MetaString::ADVOB_TXT, 66);
			onVisited.addTxt(MetaString::ADVOB_TXT, 67);
			onEmpty.addTxt(MetaString::ADVOB_TXT, 68);

			// Don't like this one but don't see any easier approach
			CVisitInfo visit;
			visit.reward.primary[PrimarySkill::ATTACK] = 2;
			visit.reward.primary[PrimarySkill::DEFENSE] = 2;
			visit.reward.primary[PrimarySkill::KNOWLEDGE] = 2;
			visit.reward.primary[PrimarySkill::SPELL_POWER] = 2;

			static_assert(SecSkillLevel::LEVELS_SIZE == 4, "Behavior of Library of Enlignment may not be correct");
			for (int i=0; i<SecSkillLevel::LEVELS_SIZE; i++)
			{
				visit.limiter.minLevel = 10 - i * 2;
				visit.limiter.secondary[SecondarySkill::DIPLOMACY] = i;
				info.push_back(visit);
			}
			soundID = soundBase::gazebo;
			break;
		}
		case Obj::SCHOOL_OF_MAGIC:
			info.resize(2);
			info[0].reward.primary[PrimarySkill::SPELL_POWER] = 1;
			info[1].reward.primary[PrimarySkill::KNOWLEDGE] = 1;
			soundID = soundBase::faerie;
			onGrant.addTxt(MetaString::ADVOB_TXT, 71);
			break;
		case Obj::SCHOOL_OF_WAR:
			info.resize(2);
			info[0].reward.primary[PrimarySkill::ATTACK] = 1;
			info[1].reward.primary[PrimarySkill::DEFENSE] = 1;
			soundID = soundBase::MILITARY;
			onGrant.addTxt(MetaString::ADVOB_TXT, 158);
			break;
	}
}
/*
const std::string & CGVisitableOPH::getHoverText() const
{
	int pom = -1;
	switch(ID)
	{
	case Obj::ARENA:
		pom = -1;
		break;
	case Obj::MERCENARY_CAMP:
		pom = 8;
		break;
	case Obj::MARLETTO_TOWER:
		pom = 7;
		break;
	case Obj::STAR_AXIS:
		pom = 11;
		break;
	case Obj::GARDEN_OF_REVELATION:
		pom = 4;
		break;
	case Obj::LEARNING_STONE:
		pom = 5;
		break;
	case Obj::TREE_OF_KNOWLEDGE:
		pom = 18;
		break;
	case Obj::LIBRARY_OF_ENLIGHTENMENT:
		break;
	case Obj::SCHOOL_OF_MAGIC:
		pom = 9;
		break;
	case Obj::SCHOOL_OF_WAR:
		pom = 10;
		break;
	default:
		throw std::runtime_error("Wrong CGVisitableOPH object ID!\n");
	}
	hoverName = VLC->generaltexth->names[ID];
	if(pom >= 0)
		hoverName += ("\n" + VLC->generaltexth->xtrainfo[pom]);
	const CGHeroInstance *h = cb->getSelectedHero (cb->getCurrentPlayer());
	if(h)
	{
		hoverName += "\n\n";
		bool visited = vstd::contains (visitors, h->id);
		hoverName += visitedTxt (visited);
	}
	return hoverName;
}
*/

///////////////////////////////////////////////////////////////////////////////////////////////////

CGVisitableOPW::CGVisitableOPW()
{
	visitMode = VISIT_ONCE;
	selectMode = SELECT_RANDOM;
	resetDuration = 7;
}

void CGVisitableOPW::initObj()
{
	switch (ID)
	{
	case Obj::MYSTICAL_GARDEN:
		soundID = soundBase::experience;
		onGrant.addTxt(MetaString::ADVOB_TXT, 92);
		onEmpty.addTxt(MetaString::ADVOB_TXT, 93);
		info.resize(2);
		info[0].reward.resources[Res::GEMS] = 5;
		info[1].reward.resources[Res::GOLD] = 500;
		break;
	case Obj::WINDMILL:
		soundID = soundBase::GENIE;
		onGrant.addTxt(MetaString::ADVOB_TXT, 170);
		onEmpty.addTxt(MetaString::ADVOB_TXT, 169);
		// 3-6 of any resource but wood and gold
		// this is UGLY. TODO: find better way to describe this
		for (int resID = Res::MERCURY; resID < Res::GOLD; resID++)
		{
			for (int val = 3; val <=6; val++)
			{
				CVisitInfo visit;
				visit.reward.resources[resID] = val;
				info.push_back(visit);
			}
		}
		break;
	case Obj::WATER_WHEEL:
		soundID = soundBase::GENIE;
		onGrant.addTxt(MetaString::ADVOB_TXT, 164);
		onEmpty.addTxt(MetaString::ADVOB_TXT, 165);

		info.resize(2);
		info[0].limiter.dayOfWeek = 7; // double amount on sunday
		info[0].reward.resources[Res::GOLD] = 1000;
		info[1].reward.resources[Res::GOLD] = 500;
		break;
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////


std::vector<int3> CGMagicSpring::getVisitableOffsets() const
{
	std::vector <int3> visitableTiles;

	for(int y = 0; y < 6; y++)
		for (int x = 0; x < 8; x++) //starting from left
			if (appearance.isVisitableAt(x, y))
				visitableTiles.push_back (int3(x, y , 0));

	return visitableTiles;
}

int3 CGMagicSpring::getVisitableOffset() const
{
	auto visitableTiles = getVisitableOffsets();

	if (visitableTiles.size() != info.size())
	{
		logGlobal->warnStream() << "Unexpected number of visitable tiles of Magic Spring at " << pos << "!";
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
		if (pos - tiles[i] == hero->getPosition() && info[i].numOfGrants == 0)
		{
			return std::vector<ui32>(1, i);
		}
	}
	// hero is either not on visitable tile (should not happen) or tile is already used
	return std::vector<ui32>();
}
