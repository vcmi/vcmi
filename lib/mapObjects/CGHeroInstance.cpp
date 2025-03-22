/*
 * CGHeroInstance.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "CGHeroInstance.h"

#include <vcmi/ServerCallback.h>
#include <vcmi/spells/Spell.h>
#include <vstd/RNG.h>

#include "../texts/CGeneralTextHandler.h"
#include "../ArtifactUtils.h"
#include "../TerrainHandler.h"
#include "../RoadHandler.h"
#include "../IGameSettings.h"
#include "../CSoundBase.h"
#include "../spells/CSpellHandler.h"
#include "../CSkillHandler.h"
#include "../IGameCallback.h"
#include "../gameState/CGameState.h"
#include "../gameState/UpgradeInfo.h"
#include "../CCreatureHandler.h"
#include "../mapping/CMap.h"
#include "../StartInfo.h"
#include "CGTownInstance.h"
#include "../entities/faction/CTownHandler.h"
#include "../entities/hero/CHeroHandler.h"
#include "../entities/hero/CHeroClass.h"
#include "../battle/CBattleInfoEssentials.h"
#include "../campaign/CampaignState.h"
#include "../json/JsonBonus.h"
#include "../pathfinder/TurnInfo.h"
#include "../serializer/JsonSerializeFormat.h"
#include "../mapObjectConstructors/AObjectTypeHandler.h"
#include "../mapObjectConstructors/CObjectClassesHandler.h"
#include "../mapObjects/MiscObjects.h"
#include "../modding/ModScope.h"
#include "../networkPacks/PacksForClient.h"
#include "../networkPacks/PacksForClientBattle.h"
#include "../constants/StringConstants.h"
#include "../battle/Unit.h"
#include "CConfigHandler.h"

VCMI_LIB_NAMESPACE_BEGIN

const ui32 CGHeroInstance::NO_PATROLLING = std::numeric_limits<ui32>::max();

void CGHeroPlaceholder::serializeJsonOptions(JsonSerializeFormat & handler)
{
	serializeJsonOwner(handler);
	
	bool isHeroType = heroType.has_value();
	handler.serializeBool("placeholderType", isHeroType, false);
	
	if(!handler.saving)
	{
		if(isHeroType)
			heroType = HeroTypeID::NONE;
		else
			powerRank = 0;
	}
	
	if(isHeroType)
		handler.serializeId("heroType", heroType.value(), HeroTypeID::NONE);
	else
		handler.serializeInt("powerRank", powerRank.value());
}

FactionID CGHeroInstance::getFactionID() const
{
	return getHeroClass()->faction;
}

const IBonusBearer* CGHeroInstance::getBonusBearer() const
{
	return this;
}

TerrainId CGHeroInstance::getNativeTerrain() const
{
	// NOTE: in H3 neutral stacks will ignore terrain penalty only if placed as topmost stack(s) in hero army.
	// This is clearly bug in H3 however intended behaviour is not clear.
	// Current VCMI behaviour will ignore neutrals in calculations so army in VCMI
	// will always have best penalty without any influence from player-defined stacks order
	// and army that consist solely from neutral will always be considered to be on native terrain

	TerrainId nativeTerrain = ETerrainId::ANY_TERRAIN;

	for(const auto & stack : stacks)
	{
		TerrainId stackNativeTerrain = stack.second->getNativeTerrain(); //consider terrain bonuses e.g. Lodestar.

		if(stackNativeTerrain == ETerrainId::NONE)
			continue;

		if(nativeTerrain == ETerrainId::ANY_TERRAIN)
			nativeTerrain = stackNativeTerrain;
		else if(nativeTerrain != stackNativeTerrain)
			return ETerrainId::NONE;
	}
	return nativeTerrain;
}

bool CGHeroInstance::isCoastVisitable() const
{
	return true;
}

bool CGHeroInstance::isBlockedVisitable() const
{
	return true;
}

BattleField CGHeroInstance::getBattlefield() const
{
	return BattleField::NONE;
}

ui8 CGHeroInstance::getSecSkillLevel(const SecondarySkill & skill) const
{
	for(const auto & elem : secSkills)
		if(elem.first == skill)
			return elem.second;
	return 0;
}

void CGHeroInstance::setSecSkillLevel(const SecondarySkill & which, int val, bool abs)
{
	if (val == 0)      // skill removal
	{
		vstd::erase_if(secSkills,  [which](const std::pair<SecondarySkill, ui8>& pair) { return pair.first == which; });
		updateSkillBonus(which, val);
	}
	else if(getSecSkillLevel(which) == 0)
	{
		secSkills.emplace_back(which, val);
		updateSkillBonus(which, val);
	}
	else
	{
		for (auto & elem : secSkills)
		{
			if(elem.first == which)
			{
				if(abs)
					elem.second = val;
				else
					elem.second += val;

				if(elem.second > 3) //workaround to avoid crashes when same sec skill is given more than once
				{
					logGlobal->warn("Skill %d increased over limit! Decreasing to Expert.", static_cast<int>(which.toEnum()));
					elem.second = 3;
				}
				updateSkillBonus(which, elem.second); //when we know final value
			}
		}
	}
}

int3 CGHeroInstance::convertToVisitablePos(const int3 & position) const
{
	return position - getVisitableOffset();
}

int3 CGHeroInstance::convertFromVisitablePos(const int3 & position) const
{
	return position + getVisitableOffset();
}

bool CGHeroInstance::canLearnSkill() const
{
	return secSkills.size() < GameConstants::SKILL_PER_HERO;
}

bool CGHeroInstance::canLearnSkill(const SecondarySkill & which) const
{
	if ( !canLearnSkill())
		return false;

	if (!cb->isAllowed(which))
		return false;

	if (getSecSkillLevel(which) > 0)
		return false;

	if (getHeroClass()->secSkillProbability.count(which) == 0)
		return false;

	if (getHeroClass()->secSkillProbability.at(which) == 0)
		return false;

	return true;
}

int CGHeroInstance::movementPointsRemaining() const
{
	return movement;
}

void CGHeroInstance::setMovementPoints(int points)
{
	if(getBonusBearer()->hasBonusOfType(BonusType::UNLIMITED_MOVEMENT))
		movement = 1000000;
	else
		movement = std::max(0, points);
}

int CGHeroInstance::movementPointsLimit(bool onLand) const
{
	auto ti = getTurnInfo(0);
	return onLand ? ti->getMovePointsLimitLand() : ti->getMovePointsLimitWater();
}

int CGHeroInstance::getLowestCreatureSpeed() const
{
	if(stacksCount() != 0)
	{
		int minimalSpeed = std::numeric_limits<int>::max();
		//TODO? should speed modifiers (eg from artifacts) affect hero movement?
		for(const auto & slot : Slots())
			minimalSpeed = std::min(minimalSpeed, slot.second->getInitiative());

		return minimalSpeed;
	}
	else
	{
		if(commander && commander->alive)
			return commander->getInitiative();
	}

	return 10;
}

std::unique_ptr<TurnInfo> CGHeroInstance::getTurnInfo(int days) const
{
	return std::make_unique<TurnInfo>(turnInfoCache.get(), this, days);
}

int CGHeroInstance::movementPointsLimitCached(bool onLand, const TurnInfo * ti) const
{
	if (onLand)
		return ti->getMovePointsLimitLand();
	else
		return ti->getMovePointsLimitWater();
}

CGHeroInstance::CGHeroInstance(IGameCallback * cb)
	: CArmedInstance(cb),
	tacticFormationEnabled(false),
	inTownGarrison(false),
	moveDir(4),
	mana(UNINITIALIZED_MANA),
	movement(UNINITIALIZED_MOVEMENT),
	level(1),
	exp(UNINITIALIZED_EXPERIENCE),
	gender(EHeroGender::DEFAULT),
	primarySkills(this),
	magicSchoolMastery(this),
	turnInfoCache(std::make_unique<TurnInfoCache>(this)),
	manaPerKnowledgeCached(this, Selector::type()(BonusType::MANA_PER_KNOWLEDGE_PERCENTAGE))
{
	setNodeType(HERO);
	ID = Obj::HERO;
	secSkills.emplace_back(SecondarySkill::NONE, -1);
}

PlayerColor CGHeroInstance::getOwner() const
{
	return tempOwner;
}

const CHeroClass * CGHeroInstance::getHeroClass() const
{
	return getHeroType()->heroClass;
}

HeroClassID CGHeroInstance::getHeroClassID() const
{
	auto heroType = getHeroTypeID();
	if (heroType.hasValue())
		return getHeroType()->heroClass->getId();
	else
		return HeroClassID();
}

const CHero * CGHeroInstance::getHeroType() const
{
	return getHeroTypeID().toHeroType();
}

HeroTypeID CGHeroInstance::getHeroTypeID() const
{
	if (ID == Obj::RANDOM_HERO)
		return HeroTypeID::NONE;
	return HeroTypeID(getObjTypeIndex().getNum());
}

void CGHeroInstance::setHeroType(HeroTypeID heroType)
{
	subID = heroType;
}

void CGHeroInstance::initObj(vstd::RNG & rand)
{
	if (ID == Obj::HERO)
		updateAppearance();
}

void CGHeroInstance::initHero(vstd::RNG & rand, const HeroTypeID & SUBID)
{
	subID = SUBID.getNum();
	initHero(rand);
}

TObjectTypeHandler CGHeroInstance::getObjectHandler() const
{
	if (ID == Obj::HERO)
		return LIBRARY->objtypeh->getHandlerFor(ID, getHeroClass()->getIndex());
	else // prison or random hero
		return LIBRARY->objtypeh->getHandlerFor(ID, 0);
}

void CGHeroInstance::updateAppearance()
{
	auto handler = LIBRARY->objtypeh->getHandlerFor(Obj::HERO, getHeroClass()->getIndex());
	auto terrain = cb->gameState()->getTile(visitablePos())->getTerrainID();
	auto app = handler->getOverride(terrain, this);
	if (app)
		appearance = app;
}

void CGHeroInstance::initHero(vstd::RNG & rand)
{
	assert(validTypes(true));
	
	if (gender == EHeroGender::DEFAULT)
		gender = getHeroType()->gender;

	if (ID == Obj::HERO)
	{
		auto handler = LIBRARY->objtypeh->getHandlerFor(Obj::HERO, getHeroClass()->getIndex());
		appearance = handler->getTemplates().front();
	}

	if(!vstd::contains(spells, SpellID::PRESET))
	{
		// hero starts with default spells
		for(const auto & spellID : getHeroType()->spells)
			spells.insert(spellID);
	}
	else //remove placeholder
		spells -= SpellID::PRESET;

	if(!vstd::contains(spells, SpellID::SPELLBOOK_PRESET))
	{
		// hero starts with default spellbook presence status
		if(!getArt(ArtifactPosition::SPELLBOOK) && getHeroType()->haveSpellBook)
		{
			auto artifact = ArtifactUtils::createArtifact(ArtifactID::SPELLBOOK);
			putArtifact(ArtifactPosition::SPELLBOOK, artifact);
		}
	}
	else
		spells -= SpellID::SPELLBOOK_PRESET;

	if(!getArt(ArtifactPosition::MACH4))
	{
		auto artifact = ArtifactUtils::createArtifact(ArtifactID::CATAPULT);
		putArtifact(ArtifactPosition::MACH4, artifact); //everyone has a catapult
	}

	if(!hasBonusFrom(BonusSource::HERO_BASE_SKILL))
	{
		for(int g=0; g<GameConstants::PRIMARY_SKILLS; ++g)
		{
			pushPrimSkill(static_cast<PrimarySkill>(g), getHeroClass()->primarySkillInitial[g]);
		}
	}
	if(secSkills.size() == 1 && secSkills[0] == std::pair<SecondarySkill,ui8>(SecondarySkill::NONE, -1)) //set secondary skills to default
		secSkills = getHeroType()->secSkillsInit;

	setFormation(EArmyFormation::LOOSE);
	if (!stacksCount()) //standard army//initial army
	{
		initArmy(rand);
	}
	assert(validTypes());

	if (patrol.patrolling)
		patrol.initialPos = visitablePos();

	if(exp == UNINITIALIZED_EXPERIENCE)
	{
		initExp(rand);
	}
	else
	{
		levelUpAutomatically(rand);
	}

	// load base hero bonuses, TODO: per-map loading of base hero bonuses
	// must be done separately from global bonuses since recruitable heroes in taverns 
	// are not attached to global bonus node but need access to some global bonuses
	// e.g. MANA_PER_KNOWLEDGE_PERCENTAGE for correct preview and initial state after recruit	for(const auto & ob : LIBRARY->modh->heroBaseBonuses)
	// or MOVEMENT to compute initial movement before recruiting is finished
	const JsonNode & baseBonuses = cb->getSettings().getValue(EGameSettings::BONUSES_PER_HERO);
	for(const auto & b : baseBonuses.Struct())
	{
		auto bonus = JsonUtils::parseBonus(b.second);
		bonus->source = BonusSource::HERO_BASE_SKILL;
		bonus->sid = BonusSourceID(id);
		bonus->duration = BonusDuration::PERMANENT;
		addNewBonus(bonus);
	}

	if (cb->getSettings().getBoolean(EGameSettings::MODULE_COMMANDERS) && !commander && getHeroClass()->commander.hasValue())
	{
		commander = new CCommanderInstance(getHeroClass()->commander);
		commander->setArmyObj (castToArmyObj()); //TODO: separate function for setting commanders
		commander->giveStackExp (exp); //after our exp is set
	}

	skillsInfo = SecondarySkillsInfo();

	//copy active (probably growing) bonuses from hero prototype to hero object
	for(const std::shared_ptr<Bonus> & b : getHeroType()->specialty)
		addNewBonus(b);

	//initialize bonuses
	recreateSecondarySkillsBonuses();

	movement = movementPointsLimit(true);
	mana = manaLimit(); //after all bonuses are taken into account, make sure this line is the last one
}

void CGHeroInstance::initArmy(vstd::RNG & rand, IArmyDescriptor * dst)
{
	if(!dst)
		dst = this;

	int warMachinesGiven = 0;

	auto stacksCountChances = cb->getSettings().getVector(EGameSettings::HEROES_STARTING_STACKS_CHANCES);
	int stacksCountInitRandomNumber = rand.nextInt(1, 100);

	size_t maxStacksCount = std::min(stacksCountChances.size(), getHeroType()->initialArmy.size());

	for(int stackNo=0; stackNo < maxStacksCount; stackNo++)
	{
		if (stacksCountInitRandomNumber > stacksCountChances[stackNo])
			continue;

		auto & stack = getHeroType()->initialArmy[stackNo];

		int count = rand.nextInt(stack.minAmount, stack.maxAmount);

		if(stack.creature == CreatureID::NONE)
		{
			logGlobal->error("Hero %s has invalid creature in initial army", getNameTranslated());
			continue;
		}

		const CCreature * creature = stack.creature.toCreature();

		if(creature->warMachine != ArtifactID::NONE) //war machine
		{
			warMachinesGiven++;
			if(dst != this)
				continue;

			ArtifactID aid = creature->warMachine;
			const CArtifact * art = aid.toArtifact();

			if(art != nullptr && !art->getPossibleSlots().at(ArtBearer::HERO).empty())
			{
				//TODO: should we try another possible slots?
				ArtifactPosition slot = art->getPossibleSlots().at(ArtBearer::HERO).front();

				if(!getArt(slot))
				{
					auto artifact = ArtifactUtils::createArtifact(aid);
					putArtifact(slot, artifact);
				}
				else
					logGlobal->warn("Hero %s already has artifact at %d, omitting giving artifact %d", getNameTranslated(), slot.toEnum(), aid.toEnum());
			}
			else
			{
				logGlobal->error("Hero %s has invalid war machine in initial army", getNameTranslated());
			}
		}
		else
		{
			dst->setCreature(SlotID(stackNo-warMachinesGiven), stack.creature, count);
		}
	}
}

CGHeroInstance::~CGHeroInstance()
{
	commander.dellNull();
}

bool CGHeroInstance::needsLastStack() const
{
	return true;
}

void CGHeroInstance::onHeroVisit(const CGHeroInstance * h) const
{
	if(h == this) return; //exclude potential self-visiting

	if (ID == Obj::HERO)
	{
		if( cb->gameState()->getPlayerRelations(tempOwner, h->tempOwner) != PlayerRelations::ENEMIES)
		{
			//exchange
			cb->heroExchange(h->id, id);
		}
		else //battle
		{
			if(visitedTown) //we're in town
				visitedTown->onHeroVisit(h); //town will handle attacking
			else
				cb->startBattle(h,	this);
		}
	}
	else if(ID == Obj::PRISON)
	{
		if (cb->getHeroCount(h->tempOwner, false) < cb->getSettings().getInteger(EGameSettings::HEROES_PER_PLAYER_ON_MAP_CAP))//free hero slot
		{
			//update hero parameters
			SetMovePoints smp;
			smp.hid = id;
			
			cb->setManaPoints (id, manaLimit());
			
			ObjectInstanceID boatId;
			const auto boatPos = visitablePos();
			if (cb->gameState()->getMap().getTile(boatPos).isWater())
			{
				smp.val = movementPointsLimit(false);
				if (!boat)
				{
					//Create a new boat for hero
					cb->createBoat(boatPos, getBoatType(), h->getOwner());
					boatId = cb->getTopObj(boatPos)->id;
				}
			}
			else
			{
				smp.val = movementPointsLimit(true);
			}
			cb->giveHero(id, h->tempOwner, boatId); //recreates def and adds hero to player
			cb->setObjPropertyID(id, ObjProperty::ID, Obj(Obj::HERO)); //set ID to 34 AFTER hero gets correct flag color
			cb->setMovePoints (&smp);

			h->showInfoDialog(102);
		}
		else //already 8 wandering heroes
		{
			h->showInfoDialog(103);
		}
	}
}

std::string CGHeroInstance::getObjectName() const
{
	if(ID != Obj::PRISON)
	{
		std::string hoverName = LIBRARY->generaltexth->allTexts[15];
		boost::algorithm::replace_first(hoverName,"%s",getNameTranslated());
		boost::algorithm::replace_first(hoverName,"%s", getClassNameTranslated());
		return hoverName;
	}
	else
		return LIBRARY->objtypeh->getObjectName(ID, 0);
}

std::string CGHeroInstance::getHoverText(PlayerColor player) const
{
	std::string hoverText = CArmedInstance::getHoverText(player) + getMovementPointsTextIfOwner(player);
	return hoverText;
}

std::string CGHeroInstance::getMovementPointsTextIfOwner(PlayerColor player) const
{
	std::string output = "";
	if(player == getOwner())
	{
		output += " " + LIBRARY->generaltexth->translate("vcmi.adventureMap.movementPointsHeroInfo");
		boost::replace_first(output, "%POINTS", std::to_string(movementPointsLimit(!boat)));
		boost::replace_first(output, "%REMAINING", std::to_string(movementPointsRemaining()));
	}

	return output;
}

ui8 CGHeroInstance::maxlevelsToMagicSchool() const
{
	return getHeroClass()->isMagicHero() ? 3 : 4;
}
ui8 CGHeroInstance::maxlevelsToWisdom() const
{
	return getHeroClass()->isMagicHero() ? 3 : 6;
}

CGHeroInstance::SecondarySkillsInfo::SecondarySkillsInfo():
	magicSchoolCounter(1),
	wisdomCounter(1)
{}

void CGHeroInstance::SecondarySkillsInfo::resetMagicSchoolCounter()
{
	magicSchoolCounter = 0;
}
void CGHeroInstance::SecondarySkillsInfo::resetWisdomCounter()
{
	wisdomCounter = 0;
}

void CGHeroInstance::pickRandomObject(vstd::RNG & rand)
{
	assert(ID == Obj::HERO || ID == Obj::PRISON || ID == Obj::RANDOM_HERO);

	if (ID == Obj::RANDOM_HERO)
	{
		auto selectedHero = cb->gameState()->pickNextHeroType(getOwner());

		ID = Obj::HERO;
		subID = selectedHero;
		randomizeArmy(getHeroClass()->faction);
	}

	auto oldSubID = subID;

	// to find object handler we must use heroClass->id
	// after setType subID used to store unique hero identify id. Check issue 2277 for details
	// exclude prisons which should use appearance as set in map, via map editor or RMG
	if (ID != Obj::PRISON)
		setType(ID, getHeroClass()->getIndex());

	this->subID = oldSubID;
}

void CGHeroInstance::recreateSecondarySkillsBonuses()
{
	auto secondarySkillsBonuses = getBonusesFrom(BonusSource::SECONDARY_SKILL);
	for(const auto & bonus : *secondarySkillsBonuses)
		removeBonus(bonus);

	for(const auto & skill_info : secSkills)
		if(skill_info.second > 0)
			updateSkillBonus(SecondarySkill(skill_info.first), skill_info.second);
}

void CGHeroInstance::updateSkillBonus(const SecondarySkill & which, int val)
{
	removeBonuses(Selector::source(BonusSource::SECONDARY_SKILL, BonusSourceID(which)));

	if(val > 0)
	{
		auto skillBonus = (*LIBRARY->skillh)[which]->at(val).effects;
		for(const auto& b : skillBonus)
			addNewBonus(std::make_shared<Bonus>(*b));
	}
}

void CGHeroInstance::setPropertyDer(ObjProperty what, ObjPropertyID identifier)
{
	if(what == ObjProperty::PRIMARY_STACK_COUNT)
		setStackCount(SlotID(0), identifier.getNum());
}

int CGHeroInstance::getPrimSkillLevel(PrimarySkill id) const
{
	return primarySkills.getSkills()[id];
}

double CGHeroInstance::getFightingStrength() const
{
	const auto & skillValues = primarySkills.getSkills();
	return sqrt((1.0 + 0.05*skillValues[PrimarySkill::ATTACK]) * (1.0 + 0.05*skillValues[PrimarySkill::DEFENSE]));
}

double CGHeroInstance::getMagicStrength() const
{
	const auto & skillValues = primarySkills.getSkills();
	if (!hasSpellbook())
		return 1;
	bool atLeastOneCombatSpell = false;
	for (auto spell : spells)
	{
		if (spell.toSpell()->isCombat())
		{
			atLeastOneCombatSpell = true;
			break;
		}
	}
	if (!atLeastOneCombatSpell)
		return 1;
	return sqrt((1.0 + 0.05*skillValues[PrimarySkill::KNOWLEDGE] * mana / manaLimit()) * (1.0 + 0.05*skillValues[PrimarySkill::SPELL_POWER] * mana / manaLimit()));
}

double CGHeroInstance::getHeroStrength() const
{
	return getFightingStrength() * getMagicStrength();
}

uint64_t CGHeroInstance::getValueForDiplomacy() const
{
	// H3 formula for hero strength when considering diplomacy skill
	uint64_t armyStrength = getArmyStrength();
	double heroStrength = sqrt(
		(1.0 + 0.05 * getPrimSkillLevel(PrimarySkill::ATTACK)) *
		(1.0 + 0.05 * getPrimSkillLevel(PrimarySkill::DEFENSE))
		);

	return heroStrength * armyStrength;
}

uint32_t CGHeroInstance::getValueForCampaign() const
{
	/// Determined by testing H3: hero is preferred for transfer in campaigns if total sum of his primary skills and his secondary skill levels is greatest
	uint32_t score = 0;
	score += getPrimSkillLevel(PrimarySkill::ATTACK);
	score += getPrimSkillLevel(PrimarySkill::DEFENSE);
	score += getPrimSkillLevel(PrimarySkill::SPELL_POWER);
	score += getPrimSkillLevel(PrimarySkill::DEFENSE);

	for (const auto& secondary : secSkills)
		score += secondary.second;

	return score;
}

ui64 CGHeroInstance::getTotalStrength() const
{
	double ret = getHeroStrength() * getArmyStrength();
	return static_cast<ui64>(ret);
}

TExpType CGHeroInstance::calculateXp(TExpType exp) const
{
	return static_cast<TExpType>(exp * (valOfBonuses(BonusType::HERO_EXPERIENCE_GAIN_PERCENT)) / 100.0);
}

int32_t CGHeroInstance::getCasterUnitId() const
{
	return id.getNum();
}

int32_t CGHeroInstance::getSpellSchoolLevel(const spells::Spell * spell, SpellSchool * outSelectedSchool) const
{
	int32_t skill = -1; //skill level

	spell->forEachSchool([&, this](const SpellSchool & cnf, bool & stop)
	{
		int32_t thisSchool = magicSchoolMastery.getMastery(cnf); //FIXME: Bonus shouldn't be additive (Witchking Artifacts : Crown of Skies)
		if(thisSchool > skill)
		{
			skill = thisSchool;
			if(outSelectedSchool)
				*outSelectedSchool = cnf;
		}
	});

	vstd::amax(skill, magicSchoolMastery.getMastery(SpellSchool::ANY)); //any school bonus
	vstd::amax(skill, valOfBonuses(BonusType::SPELL, BonusSubtypeID(spell->getId()))); //given by artifact or other effect

	vstd::amax(skill, 0); //in case we don't know any school
	vstd::amin(skill, 3);
	return skill;
}

int64_t CGHeroInstance::getSpellBonus(const spells::Spell * spell, int64_t base, const battle::Unit * affectedStack) const
{
	//applying sorcery secondary skill

	if(spell->isMagical())
		base = static_cast<int64_t>(base * (valOfBonuses(BonusType::SPELL_DAMAGE, BonusSubtypeID(SpellSchool::ANY))) / 100.0);

	base = static_cast<int64_t>(base * (100 + valOfBonuses(BonusType::SPECIFIC_SPELL_DAMAGE, BonusSubtypeID(spell->getId()))) / 100.0);

	int maxSchoolBonus = 0;

	spell->forEachSchool([&maxSchoolBonus, this](const SpellSchool & cnf, bool & stop)
	{
		vstd::amax(maxSchoolBonus, valOfBonuses(BonusType::SPELL_DAMAGE, BonusSubtypeID(cnf)));
	});

	base = static_cast<int64_t>(base * (100 + maxSchoolBonus) / 100.0);

	if(affectedStack && affectedStack->creatureLevel() > 0) //Hero specials like Solmyr, Deemer
		base = static_cast<int64_t>(base * static_cast<double>(100 + valOfBonuses(BonusType::SPECIAL_SPELL_LEV, BonusSubtypeID(spell->getId())) / affectedStack->creatureLevel()) / 100.0);

	return base;
}

int64_t CGHeroInstance::getSpecificSpellBonus(const spells::Spell * spell, int64_t base) const
{
	base = static_cast<int64_t>(base * (100 + valOfBonuses(BonusType::SPECIFIC_SPELL_DAMAGE, BonusSubtypeID(spell->getId()))) / 100.0);
	return base;
}

int32_t CGHeroInstance::getEffectLevel(const spells::Spell * spell) const
{
	return getSpellSchoolLevel(spell);
}

int32_t CGHeroInstance::getEffectPower(const spells::Spell * spell) const
{
	return getPrimSkillLevel(PrimarySkill::SPELL_POWER);
}

int32_t CGHeroInstance::getEnchantPower(const spells::Spell * spell) const
{
	int32_t spellpower = getPrimSkillLevel(PrimarySkill::SPELL_POWER);
	int32_t durationCommon = valOfBonuses(BonusType::SPELL_DURATION, BonusSubtypeID());
	int32_t durationSpecific = valOfBonuses(BonusType::SPELL_DURATION, BonusSubtypeID(spell->getId()));

	return spellpower + durationCommon + durationSpecific;
}

int64_t CGHeroInstance::getEffectValue(const spells::Spell * spell) const
{
	return 0;
}

PlayerColor CGHeroInstance::getCasterOwner() const
{
	return tempOwner;
}

void CGHeroInstance::getCasterName(MetaString & text) const
{
	//FIXME: use local name, MetaString need access to gamestate as hero name is part of map object
	text.replaceRawString(getNameTranslated());
}

void CGHeroInstance::getCastDescription(const spells::Spell * spell, const battle::Units & attacked, MetaString & text) const
{
	const bool singleTarget = attacked.size() == 1;
	const int textIndex = singleTarget ? 195 : 196;

	text.appendLocalString(EMetaText::GENERAL_TXT, textIndex);
	getCasterName(text);
	text.replaceName(spell->getId());
	if(singleTarget)
		attacked.at(0)->addNameReplacement(text, true);
}

const CGHeroInstance * CGHeroInstance::getHeroCaster() const
{
	return this;
}

void CGHeroInstance::spendMana(ServerCallback * server, const int spellCost) const
{
	if(spellCost != 0)
	{
		SetMana sm;
		sm.absolute = false;
		sm.hid = id;
		sm.val = -spellCost;

		server->apply(sm);
	}
}

bool CGHeroInstance::canCastThisSpell(const spells::Spell * spell) const
{
	const bool isAllowed = cb->isAllowed(spell->getId());

	const bool inSpellBook = vstd::contains(spells, spell->getId()) && hasSpellbook();
	const bool specificBonus = hasBonusOfType(BonusType::SPELL, BonusSubtypeID(spell->getId()));

	bool schoolBonus = false;

	spell->forEachSchool([this, &schoolBonus](const SpellSchool & cnf, bool & stop)
	{
		if(hasBonusOfType(BonusType::SPELLS_OF_SCHOOL, BonusSubtypeID(cnf)))
		{
			schoolBonus = stop = true;
		}
	});

	const bool levelBonus = hasBonusOfType(BonusType::SPELLS_OF_LEVEL, BonusCustomSubtype::spellLevel(spell->getLevel()));

	if(spell->isSpecial())
	{
		if(inSpellBook)
		{//hero has this spell in spellbook
			logGlobal->error("Special spell %s in spellbook.", spell->getNameTranslated());
		}
		return specificBonus;
	}
	else if(!isAllowed)
	{
		if(inSpellBook)
		{
			//hero has this spell in spellbook
			//it is normal if set in map editor, but trace it to possible debug of magic guild
			logGlobal->trace("Banned spell %s in spellbook.", spell->getNameTranslated());
		}
		return inSpellBook || specificBonus || schoolBonus || levelBonus;
	}
	else
	{
		return inSpellBook || schoolBonus || specificBonus || levelBonus;
	}
}

bool CGHeroInstance::canLearnSpell(const spells::Spell * spell, bool allowBanned) const
{
	if(!hasSpellbook())
		return false;

	if(spell->getLevel() > maxSpellLevel()) //not enough wisdom
		return false;

	if(vstd::contains(spells, spell->getId()))//already known
		return false;

	if(spell->isSpecial())
	{
		logGlobal->warn("Hero %s try to learn special spell %s", nodeName(), spell->getNameTranslated());
		return false;//special spells can not be learned
	}

	if(spell->isCreatureAbility())
	{
		logGlobal->warn("Hero %s try to learn creature spell %s", nodeName(), spell->getNameTranslated());
		return false;//creature abilities can not be learned
	}

	if(!allowBanned && !cb->isAllowed(spell->getId()))
	{
		logGlobal->warn("Hero %s try to learn banned spell %s", nodeName(), spell->getNameTranslated());
		return false;//banned spells should not be learned
	}

	return true;
}

/**
 * Calculates what creatures and how many to be raised from a battle.
 * @param battleResult The results of the battle.
 * @return Returns a pair with the first value indicating the ID of the creature
 * type and second value the amount. Both values are returned as -1 if necromancy
 * could not be applied.
 */
CStackBasicDescriptor CGHeroInstance::calculateNecromancy (const BattleResult &battleResult) const
{
	bool hasImprovedNecromancy = hasBonusOfType(BonusType::IMPROVED_NECROMANCY);

	// need skill or cloak of undead king - lesser artifacts don't work without skill
	if (hasImprovedNecromancy)
	{
		double necromancySkill = valOfBonuses(BonusType::UNDEAD_RAISE_PERCENTAGE) / 100.0;
		const ui8 necromancyLevel = valOfBonuses(BonusType::IMPROVED_NECROMANCY);
		vstd::amin(necromancySkill, 1.0); //it's impossible to raise more creatures than all...
		const std::map<CreatureID,si32> &casualties = battleResult.casualties[CBattleInfoEssentials::otherSide(battleResult.winner)];
		// figure out what to raise - pick strongest creature meeting requirements
		CreatureID creatureTypeRaised = CreatureID::NONE; //now we always have IMPROVED_NECROMANCY, no need for hardcode
		int requiredCasualtyLevel = 1;
		TConstBonusListPtr improvedNecromancy = getBonusesOfType(BonusType::IMPROVED_NECROMANCY);
		if(!improvedNecromancy->empty())
		{
			int maxCasualtyLevel = 1;
			for(const auto & casualty : casualties)
				vstd::amax(maxCasualtyLevel, LIBRARY->creatures()->getById(casualty.first)->getLevel());
			// pick best bonus available
			std::shared_ptr<Bonus> topPick;
			for(const std::shared_ptr<Bonus> & newPick : *improvedNecromancy)
			{
				// addInfo[0] = required necromancy skill, addInfo[1] = required casualty level
				if(newPick->additionalInfo[0] > necromancyLevel || newPick->additionalInfo[1] > maxCasualtyLevel)
					continue;
				if(!topPick)
				{
					topPick = newPick;
				}
				else
				{
					auto quality = [](const std::shared_ptr<Bonus> & pick) -> std::tuple<int, int, int>
					{
						const auto * c = pick->subtype.as<CreatureID>().toCreature();
						return std::tuple<int, int, int> {c->getLevel(), static_cast<int>(c->getFullRecruitCost().marketValue()), -pick->additionalInfo[1]};
					};
					if(quality(topPick) < quality(newPick))
						topPick = newPick;
				}
			}
			if(topPick)
			{
				creatureTypeRaised = topPick->subtype.as<CreatureID>();
				requiredCasualtyLevel = std::max(topPick->additionalInfo[1], 1);
			}
		}
		assert(creatureTypeRaised != CreatureID::NONE);
		// raise upgraded creature (at 2/3 rate) if no space available otherwise
		if(getSlotFor(creatureTypeRaised) == SlotID())
		{
			for(const CreatureID & upgraded : creatureTypeRaised.toCreature()->upgrades)
			{
				if(getSlotFor(upgraded) != SlotID())
				{
					creatureTypeRaised = upgraded;
					necromancySkill *= 2/3.0;
					break;
				}
			}
		}
		// calculate number of creatures raised - low level units contribute at 50% rate
		const double raisedUnitHealth = creatureTypeRaised.toCreature()->getMaxHealth();
		double raisedUnits = 0;
		for(const auto & casualty : casualties)
		{
			const CCreature * c = casualty.first.toCreature();
			double raisedFromCasualty = std::min(c->getMaxHealth() / raisedUnitHealth, 1.0) * casualty.second * necromancySkill;
			if(c->getLevel() < requiredCasualtyLevel)
				raisedFromCasualty *= 0.5;
			raisedUnits += raisedFromCasualty;
		}
		return CStackBasicDescriptor(creatureTypeRaised, std::max(static_cast<int>(raisedUnits), 1));
	}

	return CStackBasicDescriptor();
}

/**
 * Show the necromancy dialog with information about units raised.
 * @param raisedStack Pair where the first element represents ID of the raised creature
 * and the second element the amount.
 */
void CGHeroInstance::showNecromancyDialog(const CStackBasicDescriptor &raisedStack, vstd::RNG & rand) const
{
	InfoWindow iw;
	iw.type = EInfoWindowMode::AUTO;
	iw.soundID = soundBase::pickup01 + rand.nextInt(6);
	iw.player = tempOwner;
	iw.components.emplace_back(ComponentType::CREATURE, raisedStack.getId(), raisedStack.count);

	if (raisedStack.count > 1) // Practicing the dark arts of necromancy, ... (plural)
	{
		iw.text.appendLocalString(EMetaText::GENERAL_TXT, 145);
		iw.text.replaceNumber(raisedStack.count);
	}
	else // Practicing the dark arts of necromancy, ... (singular)
	{
		iw.text.appendLocalString(EMetaText::GENERAL_TXT, 146);
	}
	iw.text.replaceName(raisedStack);

	cb->showInfoDialog(&iw);
}
/*
int3 CGHeroInstance::getSightCenter() const
{
	return getPosition(false);
}*/

int CGHeroInstance::getSightRadius() const
{
	return valOfBonuses(BonusType::SIGHT_RADIUS); // scouting gives SIGHT_RADIUS bonus
}

si32 CGHeroInstance::manaRegain() const
{
	if (hasBonusOfType(BonusType::FULL_MANA_REGENERATION))
		return manaLimit();

	return valOfBonuses(BonusType::MANA_REGENERATION);
}

si32 CGHeroInstance::getManaNewTurn() const
{
	if(visitedTown && visitedTown->hasBuilt(BuildingID::MAGES_GUILD_1))
	{
		//if hero starts turn in town with mage guild - restore all mana
		return std::max(mana, manaLimit());
	}
	si32 res = mana + manaRegain();
	res = std::min(res, manaLimit());
	res = std::max(res, mana);
	res = std::max(res, 0);
	return res;
}

// /**
//  * Places an artifact in hero's backpack. If it's a big artifact equips it
//  * or discards it if it cannot be equipped.
//  */
// void CGHeroInstance::giveArtifact (ui32 aid) //use only for fixed artifacts
// {
// 	CArtifact * const artifact = LIBRARY->arth->objects[aid]; //pointer to constant object
// 	CArtifactInstance *ai = CArtifactInstance::createNewArtifactInstance(artifact);
// 	ai->putAt(this, ai->firstAvailableSlot(this));
// }

BoatId CGHeroInstance::getBoatType() const
{
	return BoatId(LIBRARY->townh->getById(getHeroClass()->faction)->getBoatType());
}

void CGHeroInstance::getOutOffsets(std::vector<int3> &offsets) const
{
	offsets = {
		{0, -1, 0},
		{+1, -1, 0},
		{+1, 0, 0},
		{+1, +1, 0},
		{0, +1, 0},
		{-1, +1, 0},
		{-1, 0, 0},
		{-1, -1, 0},
	};
}

const IObjectInterface * CGHeroInstance::getObject() const
{
	return this;
}

int32_t CGHeroInstance::getSpellCost(const spells::Spell * sp) const
{
	return sp->getCost(getSpellSchoolLevel(sp));
}

void CGHeroInstance::pushPrimSkill( PrimarySkill which, int val )
{
	auto sel = Selector::typeSubtype(BonusType::PRIMARY_SKILL, BonusSubtypeID(which))
		.And(Selector::sourceType()(BonusSource::HERO_BASE_SKILL));

	removeBonuses(sel);
	addNewBonus(std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::PRIMARY_SKILL, BonusSource::HERO_BASE_SKILL, val, BonusSourceID(id), BonusSubtypeID(which)));
}

EAlignment CGHeroInstance::getAlignment() const
{
	return getHeroClass()->getAlignment();
}

void CGHeroInstance::initExp(vstd::RNG & rand)
{
	exp = rand.nextInt(40, 89);
}

std::string CGHeroInstance::nodeName() const
{
	return "Hero " + getNameTextID();
}

si32 CGHeroInstance::manaLimit() const
{
	return getPrimSkillLevel(PrimarySkill::KNOWLEDGE) * manaPerKnowledgeCached.getValue() / 100;
}

HeroTypeID CGHeroInstance::getPortraitSource() const
{
	if (customPortraitSource.isValid())
		return customPortraitSource;
	else
		return getHeroTypeID();
}

int32_t CGHeroInstance::getIconIndex() const
{
	return getPortraitSource().toEntity(LIBRARY)->getIconIndex();
}

std::string CGHeroInstance::getNameTranslated() const
{
	return LIBRARY->generaltexth->translate(getNameTextID());
}

std::string CGHeroInstance::getClassNameTranslated() const
{
	return LIBRARY->generaltexth->translate(getClassNameTextID());
}

std::string CGHeroInstance::getClassNameTextID() const
{
	if (isCampaignGem())
		return "core.genrltxt.735";
	return getHeroClass()->getNameTextID();
}

std::string CGHeroInstance::getNameTextID() const
{
	if (!nameCustomTextId.empty())
		return nameCustomTextId;
	if (getHeroTypeID().hasValue())
		return getHeroType()->getNameTextID();

	// FIXME: called by logging from some specialties (mods?) before type is set on deserialization
	// assert(0);
	return "";
}

std::string CGHeroInstance::getBiographyTranslated() const
{
	return LIBRARY->generaltexth->translate(getBiographyTextID());
}

std::string CGHeroInstance::getBiographyTextID() const
{
	if (!biographyCustomTextId.empty())
		return biographyCustomTextId;
	if (getHeroTypeID().hasValue())
		return getHeroType()->getBiographyTextID();
	
	return ""; //for random hero
}

CGHeroInstance::ArtPlacementMap CGHeroInstance::putArtifact(const ArtifactPosition & pos, CArtifactInstance * art)
{
	assert(art->canBePutAt(this, pos));

	if(ArtifactUtils::isSlotEquipment(pos))
		attachTo(*art);
	return CArtifactSet::putArtifact(pos, art);
}

void CGHeroInstance::removeArtifact(const ArtifactPosition & pos)
{
	auto art = getArt(pos);
	assert(art);

	CArtifactSet::removeArtifact(pos);
	if(ArtifactUtils::isSlotEquipment(pos))
		detachFrom(*art);
}

bool CGHeroInstance::hasSpellbook() const
{
	return getArt(ArtifactPosition::SPELLBOOK);
}

void CGHeroInstance::addSpellToSpellbook(const SpellID & spell)
{
	spells.insert(spell);
}

void CGHeroInstance::removeSpellFromSpellbook(const SpellID & spell)
{
	spells.erase(spell);
}

bool CGHeroInstance::spellbookContainsSpell(const SpellID & spell) const
{
	return vstd::contains(spells, spell);
}

void CGHeroInstance::removeSpellbook()
{
	spells.clear();

	if(hasSpellbook())
	{
		cb->gameState()->getMap().removeArtifactInstance(*this, ArtifactPosition::SPELLBOOK);
	}
}

const std::set<SpellID> & CGHeroInstance::getSpellsInSpellbook() const
{
	return spells;
}

int CGHeroInstance::maxSpellLevel() const
{
	return std::min(GameConstants::SPELL_LEVELS, valOfBonuses(BonusType::MAX_LEARNABLE_SPELL_LEVEL));
}

void CGHeroInstance::attachToBoat(CGBoat* newBoat)
{
	assert(newBoat);
	boat = newBoat;
	attachTo(const_cast<CGBoat&>(*boat));
	const_cast<CGBoat*>(boat)->hero = this;
}


void CGHeroInstance::deserializationFix()
{
	artDeserializationFix(this);
	boatDeserializationFix();
}

void CGHeroInstance::boatDeserializationFix()
{
	if (boat)
		attachTo(const_cast<CGBoat&>(*boat));
}

CBonusSystemNode * CGHeroInstance::whereShouldBeAttachedOnSiege(const bool isBattleOutsideTown) const
{
	if(!visitedTown)
		return nullptr;

	return isBattleOutsideTown ? (CBonusSystemNode *)(& visitedTown->townAndVis)
		: (CBonusSystemNode *)(visitedTown.get());

}

CBonusSystemNode * CGHeroInstance::whereShouldBeAttachedOnSiege(CGameState * gs)
{
	if(visitedTown)
		return whereShouldBeAttachedOnSiege(visitedTown->isBattleOutsideTown(this));

	return &CArmedInstance::whereShouldBeAttached(gs);
}

CBonusSystemNode & CGHeroInstance::whereShouldBeAttached(CGameState * gs)
{
	if(visitedTown)
	{
		if(inTownGarrison)
			return *visitedTown;
		else
			return visitedTown->townAndVis;
	}
	else
		return CArmedInstance::whereShouldBeAttached(gs);
}

int CGHeroInstance::movementPointsAfterEmbark(int MPsBefore, int basicCost, bool disembark, const TurnInfo * ti) const
{
	if(!ti->hasFreeShipBoarding())
		return 0; // take all MPs by default
	
	auto boatLayer = boat ? boat->layer : EPathfindingLayer::SAIL;

	int mp1 = ti->getMaxMovePoints(disembark ? EPathfindingLayer::LAND : boatLayer);
	int mp2 = ti->getMaxMovePoints(disembark ? boatLayer : EPathfindingLayer::LAND);
	int ret = static_cast<int>((MPsBefore - basicCost) * static_cast<double>(mp1) / mp2);
	return ret;
}

EDiggingStatus CGHeroInstance::diggingStatus() const
{
	if(static_cast<int>(movement) < movementPointsLimit(true))
		return EDiggingStatus::LACK_OF_MOVEMENT;
	if(!ArtifactID(ArtifactID::GRAIL).toArtifact()->canBePutAt(this))
		return EDiggingStatus::BACKPACK_IS_FULL;
	return cb->getTileDigStatus(visitablePos());
}

ArtBearer::ArtBearer CGHeroInstance::bearerType() const
{
	return ArtBearer::HERO;
}

std::vector<SecondarySkill> CGHeroInstance::getLevelUpProposedSecondarySkills(vstd::RNG & rand) const
{
	auto getObligatorySkills = [](CSkill::Obligatory obl){
		std::set<SecondarySkill> obligatory;
		for(auto i = 0; i < LIBRARY->skillh->size(); i++)
			if((*LIBRARY->skillh)[SecondarySkill(i)]->obligatory(obl))
				obligatory.insert(i); //Always return all obligatory skills

		return obligatory;
	};

	auto intersect = [](const std::set<SecondarySkill> & left, const std::set<SecondarySkill> & right)
	{
		std::set<SecondarySkill> intersect;
		std::set_intersection(left.begin(), left.end(), right.begin(), right.end(),
						 std::inserter(intersect, intersect.begin()));
		return intersect;
	};

	std::set<SecondarySkill> wisdomList = getObligatorySkills(CSkill::Obligatory::MAJOR);
	std::set<SecondarySkill> schoolList = getObligatorySkills(CSkill::Obligatory::MINOR);

	std::set<SecondarySkill> basicAndAdv;
	std::set<SecondarySkill> none;

	for(int i = 0; i < LIBRARY->skillh->size(); i++)
		if (canLearnSkill(SecondarySkill(i)))
			none.insert(SecondarySkill(i));

	for(const auto & elem : secSkills)
	{
		if(elem.second < MasteryLevel::EXPERT)
			basicAndAdv.insert(elem.first);
		none.erase(elem.first);
	}

	bool wantsWisdom = skillsInfo.wisdomCounter + 1 >= maxlevelsToWisdom();
	bool wantsSchool = skillsInfo.magicSchoolCounter + 1 >= maxlevelsToMagicSchool();

	std::vector<SecondarySkill> skills;

	auto chooseSkill = [&](std::set<SecondarySkill> & options)
	{
		bool selectWisdom = wantsWisdom && !intersect(options, wisdomList).empty();
		bool selectSchool = wantsSchool && !intersect(options, schoolList).empty();
		SecondarySkill selection;

		if (selectWisdom)
			selection = getHeroClass()->chooseSecSkill(intersect(options, wisdomList), rand);
		else if (selectSchool)
			selection = getHeroClass()->chooseSecSkill(intersect(options, schoolList), rand);
		else
			selection = getHeroClass()->chooseSecSkill(options, rand);

		skills.push_back(selection);
		options.erase(selection);

		if (wisdomList.count(selection))
			wisdomList.clear();

		if (schoolList.count(selection))
			schoolList.clear();
	};

	if (!basicAndAdv.empty())
		chooseSkill(basicAndAdv);

	if (canLearnSkill() && !none.empty())
		chooseSkill(none);

	if (!basicAndAdv.empty() && skills.size() < 2)
		chooseSkill(basicAndAdv);

	if (canLearnSkill() && !none.empty() && skills.size() < 2)
		chooseSkill(none);

	return skills;
}

PrimarySkill CGHeroInstance::nextPrimarySkill(vstd::RNG & rand) const
{
	assert(gainsLevel());
	const auto isLowLevelHero = level < GameConstants::HERO_HIGH_LEVEL;
	const auto & skillChances = isLowLevelHero ? getHeroClass()->primarySkillLowLevel : getHeroClass()->primarySkillHighLevel;

	if (isCampaignYog())
	{
		// Yog can only receive Attack or Defence on level-up
		std::vector<int> yogChances = { skillChances[0], skillChances[1]};
		return static_cast<PrimarySkill>(RandomGeneratorUtil::nextItemWeighted(yogChances, rand));
	}

	return static_cast<PrimarySkill>(RandomGeneratorUtil::nextItemWeighted(skillChances, rand));
}

std::optional<SecondarySkill> CGHeroInstance::nextSecondarySkill(vstd::RNG & rand) const
{
	assert(gainsLevel());

	std::optional<SecondarySkill> chosenSecondarySkill;
	const auto proposedSecondarySkills = getLevelUpProposedSecondarySkills(rand);
	if(!proposedSecondarySkills.empty())
	{
		std::vector<SecondarySkill> learnedSecondarySkills;
		for(const auto & secondarySkill : proposedSecondarySkills)
		{
			if(getSecSkillLevel(secondarySkill) > 0)
			{
				learnedSecondarySkills.push_back(secondarySkill);
			}
		}

		if(learnedSecondarySkills.empty())
		{
			// there are only new skills to learn, so choose anyone of them
			chosenSecondarySkill = std::make_optional(*RandomGeneratorUtil::nextItem(proposedSecondarySkills, rand));
		}
		else
		{
			// preferably upgrade a already learned secondary skill
			chosenSecondarySkill = std::make_optional(*RandomGeneratorUtil::nextItem(learnedSecondarySkills, rand));
		}
	}
	return chosenSecondarySkill;
}

void CGHeroInstance::setPrimarySkill(PrimarySkill primarySkill, si64 value, ui8 abs)
{
	if(primarySkill < PrimarySkill::EXPERIENCE)
	{
		auto skill = getLocalBonus(Selector::type()(BonusType::PRIMARY_SKILL)
			.And(Selector::subtype()(BonusSubtypeID(primarySkill)))
			.And(Selector::sourceType()(BonusSource::HERO_BASE_SKILL)));
		assert(skill);

		if(abs)
		{
			skill->val = static_cast<si32>(value);
		}
		else
		{
			skill->val += static_cast<si32>(value);
		}
		nodeHasChanged();
	}
	else if(primarySkill == PrimarySkill::EXPERIENCE)
	{
		if(abs)
		{
			exp = value;
		}
		else
		{
			exp += value;
		}
	}
}

bool CGHeroInstance::gainsLevel() const
{
	return level < LIBRARY->heroh->maxSupportedLevel() && exp >= static_cast<TExpType>(LIBRARY->heroh->reqExp(level+1));
}

void CGHeroInstance::levelUp(const std::vector<SecondarySkill> & skills)
{
	++level;

	//deterministic secondary skills
	++skillsInfo.magicSchoolCounter;
	++skillsInfo.wisdomCounter;

	for(const auto & skill : skills)
	{
		if((*LIBRARY->skillh)[skill]->obligatory(CSkill::Obligatory::MAJOR))
			skillsInfo.resetWisdomCounter();
		if((*LIBRARY->skillh)[skill]->obligatory(CSkill::Obligatory::MINOR))
			skillsInfo.resetMagicSchoolCounter();
	}

	//update specialty and other bonuses that scale with level
	nodeHasChanged();
}

void CGHeroInstance::levelUpAutomatically(vstd::RNG & rand)
{
	while(gainsLevel())
	{
		const auto primarySkill = nextPrimarySkill(rand);
		setPrimarySkill(primarySkill, 1, false);

		auto proposedSecondarySkills = getLevelUpProposedSecondarySkills(rand);

		const auto secondarySkill = nextSecondarySkill(rand);
		if(secondarySkill)
		{
			setSecSkillLevel(*secondarySkill, 1, false);
		}

		//TODO why has the secondary skills to be passed to the method?
		levelUp(proposedSecondarySkills);
	}
}

bool CGHeroInstance::hasVisions(const CGObjectInstance * target, BonusSubtypeID subtype) const
{
	//VISIONS spell support
	const int visionsMultiplier = valOfBonuses(BonusType::VISIONS, subtype);

	int visionsRange =  visionsMultiplier * getPrimSkillLevel(PrimarySkill::SPELL_POWER);

	if (visionsMultiplier > 0)
		vstd::amax(visionsRange, 3); //minimum range is 3 tiles, but only if VISIONS bonus present

	const int distance = static_cast<int>(target->anchorPos().dist2d(visitablePos()));

	//logGlobal->debug(boost::str(boost::format("Visions: dist %d, mult %d, range %d") % distance % visionsMultiplier % visionsRange));

	return (distance < visionsRange) && (target->anchorPos().z == anchorPos().z);
}

std::string CGHeroInstance::getHeroTypeName() const
{
	if(ID == Obj::HERO || ID == Obj::PRISON)
		return getHeroType()->getJsonKey();

	return "";
}

void CGHeroInstance::afterAddToMap(CMap * map)
{
	if(ID != Obj::PRISON)
		map->heroesOnMap.emplace_back(this);
}
void CGHeroInstance::afterRemoveFromMap(CMap* map)
{
	if (ID == Obj::PRISON)
		vstd::erase_if_present(map->heroesOnMap, this);
}

void CGHeroInstance::setHeroTypeName(const std::string & identifier)
{
	if(ID == Obj::HERO || ID == Obj::PRISON)
	{
		auto rawId = LIBRARY->identifiers()->getIdentifier(ModScope::scopeMap(), "hero", identifier);

		if(rawId)
			subID = rawId.value();
		else
		{
			throw std::runtime_error("Couldn't resolve hero identifier " + identifier);
		}
	}
}

void CGHeroInstance::updateFrom(const JsonNode & data)
{
	CGObjectInstance::updateFrom(data);
}

void CGHeroInstance::serializeCommonOptions(JsonSerializeFormat & handler)
{
	handler.serializeString("biography", biographyCustomTextId);
	handler.serializeInt("experience", exp, 0);

	if(!handler.saving && exp != UNINITIALIZED_EXPERIENCE) //do not gain levels if experience is not initialized
	{
		while (gainsLevel())
		{
			++level;
		}
	}

	handler.serializeString("name", nameCustomTextId);
	handler.serializeInt("gender", gender, 0);
	handler.serializeId("portrait", customPortraitSource, HeroTypeID::NONE);

	//primary skills
	if(handler.saving)
	{
		const bool haveSkills = hasBonus(Selector::type()(BonusType::PRIMARY_SKILL).And(Selector::sourceType()(BonusSource::HERO_BASE_SKILL)));

		if(haveSkills)
		{
			auto primarySkills = handler.enterStruct("primarySkills");

			for(auto skill : PrimarySkill::ALL_SKILLS())
			{
				int value = valOfBonuses(Selector::typeSubtype(BonusType::PRIMARY_SKILL, BonusSubtypeID(skill)).And(Selector::sourceType()(BonusSource::HERO_BASE_SKILL)));

				handler.serializeInt(NPrimarySkill::names[skill.getNum()], value, 0);
			}
		}
	}
	else
	{
		auto primarySkills = handler.enterStruct("primarySkills");

		if(handler.getCurrent().getType() == JsonNode::JsonType::DATA_STRUCT)
		{
			for(int i = 0; i < GameConstants::PRIMARY_SKILLS; ++i)
			{
				int value = 0;
				handler.serializeInt(NPrimarySkill::names[i], value, 0);
				pushPrimSkill(static_cast<PrimarySkill>(i), value);
			}
		}
	}

	//secondary skills
	if(handler.saving)
	{
		//does hero have default skills?
		bool defaultSkills = false;
		bool normalSkills = false;
		for(const auto & p : secSkills)
		{
			if(p.first == SecondarySkill(SecondarySkill::NONE))
				defaultSkills = true;
			else
				normalSkills = true;
		}

		if(defaultSkills && normalSkills)
			logGlobal->error("Mixed default and normal secondary skills");

		//in json default skills means no field/null
		if(!defaultSkills)
		{
			//enter array here as handler initialize it
			auto secondarySkills = handler.enterArray("secondarySkills");
			secondarySkills.syncSize(secSkills, JsonNode::JsonType::DATA_VECTOR);

			for(size_t skillIndex = 0; skillIndex < secondarySkills.size(); ++skillIndex)
			{
				JsonArraySerializer inner = secondarySkills.enterArray(skillIndex);
				SecondarySkill skillId = secSkills.at(skillIndex).first;

				handler.serializeId("skill", skillId);
				std::string skillLevel = NSecondarySkill::levels.at(secSkills.at(skillIndex).second);
				handler.serializeString("level", skillLevel);
			}
		}
	}
	else
	{
		auto secondarySkills = handler.getCurrent()["secondarySkills"];

		secSkills.clear();
		if(secondarySkills.getType() == JsonNode::JsonType::DATA_NULL)
		{
			secSkills.emplace_back(SecondarySkill::NONE, -1);
		}
		else
		{
			auto addSkill = [this](const std::string & skillId, const std::string & levelId)
			{
				const int rawId = SecondarySkill::decode(skillId);
				if(rawId < 0)
				{
					logGlobal->error("Invalid secondary skill %s", skillId);
					return;
				}

				const int level = vstd::find_pos(NSecondarySkill::levels, levelId);
				if(level < 0)
				{
					logGlobal->error("Invalid secondary skill level%s", levelId);
					return;
				}

				secSkills.emplace_back(SecondarySkill(rawId), level);
			};

			if(secondarySkills.getType() == JsonNode::JsonType::DATA_VECTOR)
			{
				for(const auto & p : secondarySkills.Vector())								
				{
					auto skillMap = p.Struct();
					addSkill(skillMap["skill"].String(), skillMap["level"].String());
				}
			}
			else if(secondarySkills.getType() == JsonNode::JsonType::DATA_STRUCT)
			{
				for(const auto & p : secondarySkills.Struct())
				{
					addSkill(p.first, p.second.String());
				};
			}
		}
	}

	handler.serializeIdArray("spellBook", spells);

	if(handler.saving)
		CArtifactSet::serializeJsonArtifacts(handler, "artifacts");
}

void CGHeroInstance::serializeJsonOptions(JsonSerializeFormat & handler)
{
	serializeCommonOptions(handler);

	serializeJsonOwner(handler);

	if(ID == Obj::HERO || ID == Obj::PRISON)
	{
		std::string typeName;
		if(handler.saving)
			typeName = getHeroTypeName();
		handler.serializeString("type", typeName);
		if(!handler.saving)
			setHeroTypeName(typeName);
	}

	if(!handler.saving)
	{
		if(!appearance)
		{
			// crossoverDeserialize
			appearance = LIBRARY->objtypeh->getHandlerFor(Obj::HERO, getHeroClassID())->getTemplates().front();
		}
	}

	CArmedInstance::serializeJsonOptions(handler);

	{
		ui32 rawPatrolRadius = NO_PATROLLING;

		if(handler.saving)
		{
			rawPatrolRadius = patrol.patrolling ? patrol.patrolRadius : NO_PATROLLING;
		}

		handler.serializeInt("patrolRadius", rawPatrolRadius, NO_PATROLLING);

		if(!handler.saving)
		{
			patrol.patrolling = (rawPatrolRadius != NO_PATROLLING);
			patrol.initialPos = visitablePos();
			patrol.patrolRadius = patrol.patrolling ? rawPatrolRadius : 0;
		}
	}
}

void CGHeroInstance::serializeJsonDefinition(JsonSerializeFormat & handler)
{
	serializeCommonOptions(handler);
}

bool CGHeroInstance::isMissionCritical() const
{
	for(const TriggeredEvent & event : cb->getMapHeader()->triggeredEvents)
	{
		if (event.effect.type != EventEffect::DEFEAT)
			continue;

		auto const & testFunctor = [&](const EventCondition & condition)
		{
			if ((condition.condition == EventCondition::CONTROL) && condition.objectID != ObjectInstanceID::NONE)
				return (id != condition.objectID);

			if (condition.condition == EventCondition::HAVE_ARTIFACT)
			{
				if(hasArt(condition.objectType.as<ArtifactID>()))
					return true;
			}

			if(condition.condition == EventCondition::IS_HUMAN)
				return true;

			return false;
		};

		if(event.trigger.test(testFunctor))
			return true;
	}
	return false;
}

void CGHeroInstance::fillUpgradeInfo(UpgradeInfo & info, const CStackInstance & stack) const
{
	TConstBonusListPtr lista = getBonusesOfType(BonusType::SPECIAL_UPGRADE, BonusSubtypeID(stack.getId()));
	for(const auto & it : *lista)
	{
		auto nid = CreatureID(it->additionalInfo[0]);
		if (nid != stack.getId()) //in very specific case the upgrade is available by default (?)
		{
			info.addUpgrade(nid, stack.getType());
		}
	}
}

bool CGHeroInstance::isCampaignYog() const
{
	const StartInfo *si = cb->getStartInfo();

	// it would be nice to find a way to move this hack to config/mapOverrides.json
	if(!si || !si->campState)
		return false;

	std::string campaign = si->campState->getFilename();
	if (!boost::starts_with(campaign, "DATA/YOG")) // "Birth of a Barbarian"
		return false;

	if (getHeroTypeID() != HeroTypeID::SOLMYR) // Yog (based on Solmyr)
		return false;

	return true;
}

bool CGHeroInstance::isCampaignGem() const
{
	const StartInfo *si = cb->getStartInfo();

	// it would be nice to find a way to move this hack to config/mapOverrides.json
	if(!si || !si->campState)
		return false;

	std::string campaign = si->campState->getFilename();
	if (!boost::starts_with(campaign, "DATA/GEM") &&  !boost::starts_with(campaign, "DATA/FINAL")) // "New Beginning" and "Unholy Alliance"
		return false;

	if (getHeroTypeID() != HeroTypeID::GEM) // Yog (based on Solmyr)
		return false;

	return true;
}

ResourceSet CGHeroInstance::dailyIncome() const
{
	ResourceSet income;

	for (GameResID k : GameResID::ALL_RESOURCES())
		income[k] += valOfBonuses(BonusType::GENERATE_RESOURCE, BonusSubtypeID(k));

	const auto & playerSettings = cb->getPlayerSettings(getOwner());
	income.applyHandicap(playerSettings->handicap.percentIncome);
	return income;
}

std::vector<CreatureID> CGHeroInstance::providedCreatures() const
{
	return {};
}

const IOwnableObject * CGHeroInstance::asOwnable() const
{
	return this;
}

int CGHeroInstance::getBasePrimarySkillValue(PrimarySkill which) const
{
	std::string cachingStr = "CGHeroInstance::getBasePrimarySkillValue" + std::to_string(static_cast<int>(which));
	auto selector = Selector::typeSubtype(BonusType::PRIMARY_SKILL, BonusSubtypeID(which)).And(Selector::sourceType()(BonusSource::HERO_BASE_SKILL));
	auto minSkillValue = LIBRARY->engineSettings()->getVectorValue(EGameSettings::HEROES_MINIMAL_PRIMARY_SKILLS, which.getNum());
	return std::max(valOfBonuses(selector, cachingStr), minSkillValue);
}

VCMI_LIB_NAMESPACE_END
