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

#include "../NetPacks.h"
#include "../CGeneralTextHandler.h"
#include "../CHeroHandler.h"
#include "../TerrainHandler.h"
#include "../RoadHandler.h"
#include "../CModHandler.h"
#include "../CSoundBase.h"
#include "../spells/CSpellHandler.h"
#include "../CSkillHandler.h"
#include "CObjectClassesHandler.h"
#include "../IGameCallback.h"
#include "../CGameState.h"
#include "../CCreatureHandler.h"
#include "../CTownHandler.h"
#include "../mapping/CMap.h"
#include "CGTownInstance.h"
#include "../serializer/JsonSerializeFormat.h"
#include "../StringConstants.h"
#include "../battle/Unit.h"

VCMI_LIB_NAMESPACE_BEGIN

static int lowestSpeed(const CGHeroInstance * chi)
{
	static const CSelector selectorSTACKS_SPEED = Selector::type()(Bonus::STACKS_SPEED);
	static const std::string keySTACKS_SPEED = "type_" + std::to_string(static_cast<si32>(Bonus::STACKS_SPEED));

	if(!chi->stacksCount())
	{
		if(chi->commander && chi->commander->alive)
		{
			return chi->commander->valOfBonuses(selectorSTACKS_SPEED, keySTACKS_SPEED);
		}

		logGlobal->error("Hero %d (%s) has no army!", chi->id.getNum(), chi->getNameTranslated());
		return 20;
	}

	auto i = chi->Slots().begin();
	//TODO? should speed modifiers (eg from artifacts) affect hero movement?

	int ret = (i++)->second->valOfBonuses(selectorSTACKS_SPEED, keySTACKS_SPEED);
	for(; i != chi->Slots().end(); i++)
		ret = std::min(ret, i->second->valOfBonuses(selectorSTACKS_SPEED, keySTACKS_SPEED));
	return ret;
}

ui32 CGHeroInstance::getTileCost(const TerrainTile & dest, const TerrainTile & from, const TurnInfo * ti) const
{
	int64_t ret = GameConstants::BASE_MOVEMENT_COST;

	//if there is road both on dest and src tiles - use road movement cost
	if(dest.roadType->getId() != Road::NO_ROAD && from.roadType->getId() != Road::NO_ROAD)
	{
		ret = std::max(dest.roadType->movementCost, from.roadType->movementCost);
	}
	else if(ti->nativeTerrain != from.terType->getId() &&//the terrain is not native
			ti->nativeTerrain != ETerrainId::ANY_TERRAIN && //no special creature bonus
			!ti->hasBonusOfType(Bonus::NO_TERRAIN_PENALTY, from.terType->getIndex())) //no special movement bonus
	{

		ret = VLC->heroh->terrCosts[from.terType->getId()];
		ret -= ti->valOfBonuses(Bonus::ROUGH_TERRAIN_DISCOUNT);
		if(ret < GameConstants::BASE_MOVEMENT_COST)
			ret = GameConstants::BASE_MOVEMENT_COST;
	}
	return static_cast<ui32>(ret);
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
		TerrainId stackNativeTerrain = stack.second->type->getNativeTerrain(); //consider terrain bonuses e.g. Lodestar.

		if(stackNativeTerrain == ETerrainId::NONE)
			continue;

		if(nativeTerrain == ETerrainId::ANY_TERRAIN)
			nativeTerrain = stackNativeTerrain;
		else if(nativeTerrain != stackNativeTerrain)
			return ETerrainId::NONE;
	}
	return nativeTerrain;
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
	if(getSecSkillLevel(which) == 0)
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

	if (!cb->isAllowed(2, which))
		return false;

	if (getSecSkillLevel(which) > 0)
		return false;

	if (type->heroClass->secSkillProbability[which] == 0)
		return false;

	return true;
}

int CGHeroInstance::maxMovePoints(bool onLand) const
{
	TurnInfo ti(this);
	return maxMovePointsCached(onLand, &ti);
}

int CGHeroInstance::maxMovePointsCached(bool onLand, const TurnInfo * ti) const
{
	int base = 0;

	if(onLand)
	{
		// used function is f(x) = 66.6x + 1300, rounded to second digit, where x is lowest speed in army
		static constexpr int baseSpeed = 1300; // base speed from creature with 0 speed

		int armySpeed = lowestSpeed(this) * 20 / 3;

		base = armySpeed * 10 + baseSpeed; // separate *10 is intentional to receive same rounding as in h3
		vstd::abetween(base, 1500, 2000); // base speed is limited by these values
	}
	else
	{
		base = 1500; //on water base movement is always 1500 (speed of army doesn't matter)
	}

	const Bonus::BonusType bt = onLand ? Bonus::LAND_MOVEMENT : Bonus::SEA_MOVEMENT;
	const int bonus = ti->valOfBonuses(Bonus::MOVEMENT) + ti->valOfBonuses(bt);

	const int subtype = onLand ? SecondarySkill::LOGISTICS : SecondarySkill::NAVIGATION;
	const double modifier = ti->valOfBonuses(Bonus::SECONDARY_SKILL_PREMY, subtype) / 100.0;

	return static_cast<int>(base * (1 + modifier)) + bonus;
}

CGHeroInstance::CGHeroInstance():
	IBoatGenerator(this),
	tacticFormationEnabled(false),
	inTownGarrison(false),
	moveDir(4),
	mana(UNINITIALIZED_MANA),
	movement(UNINITIALIZED_MOVEMENT),
	portrait(UNINITIALIZED_PORTRAIT),
	level(1),
	exp(UNINITIALIZED_EXPERIENCE),
	sex(std::numeric_limits<ui8>::max())
{
	setNodeType(HERO);
	ID = Obj::HERO;
	secSkills.emplace_back(SecondarySkill::DEFAULT, -1);
}

PlayerColor CGHeroInstance::getOwner() const
{
	return tempOwner;
}

void CGHeroInstance::initHero(CRandomGenerator & rand, const HeroTypeID & SUBID)
{
	subID = SUBID.getNum();
	initHero(rand);
}

void CGHeroInstance::setType(si32 ID, si32 subID)
{
	assert(ID == Obj::HERO); // just in case
	type = VLC->heroh->objects[subID];
	portrait = type->imageIndex;
	CGObjectInstance::setType(ID, type->heroClass->getIndex()); // to find object handler we must use heroClass->id
	this->subID = subID; // after setType subID used to store unique hero identify id. Check issue 2277 for details
	randomizeArmy(type->heroClass->faction);
}

void CGHeroInstance::initHero(CRandomGenerator & rand)
{
	assert(validTypes(true));
	if(!type)
		type = VLC->heroh->objects[subID];

	if (ID == Obj::HERO)
		appearance = VLC->objtypeh->getHandlerFor(Obj::HERO, type->heroClass->getIndex())->getTemplates().front();

	if(!vstd::contains(spells, SpellID::PRESET)) //hero starts with a spell
	{
		for(const auto & spellID : type->spells)
			spells.insert(spellID);
	}
	else //remove placeholder
		spells -= SpellID::PRESET;

	if(!getArt(ArtifactPosition::MACH4) && !getArt(ArtifactPosition::SPELLBOOK) && type->haveSpellBook) //no catapult means we haven't read pre-existent set -> use default rules for spellbook
		putArtifact(ArtifactPosition::SPELLBOOK, CArtifactInstance::createNewArtifactInstance(ArtifactID::SPELLBOOK));

	if(!getArt(ArtifactPosition::MACH4))
		putArtifact(ArtifactPosition::MACH4, CArtifactInstance::createNewArtifactInstance(ArtifactID::CATAPULT)); //everyone has a catapult

	if(portrait < 0 || portrait == 255)
		portrait = type->imageIndex;
	if(!hasBonus(Selector::sourceType()(Bonus::HERO_BASE_SKILL)))
	{
		for(int g=0; g<GameConstants::PRIMARY_SKILLS; ++g)
		{
			pushPrimSkill(static_cast<PrimarySkill::PrimarySkill>(g), type->heroClass->primarySkillInitial[g]);
		}
	}
	if(secSkills.size() == 1 && secSkills[0] == std::pair<SecondarySkill,ui8>(SecondarySkill::DEFAULT, -1)) //set secondary skills to default
		secSkills = type->secSkillsInit;

	if (sex == 0xFF)//sex is default
		sex = type->sex;

	setFormation(false);
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
	// e.g. MANA_PER_KNOWLEDGE for correct preview and initial state after recruit	for(const auto & ob : VLC->modh->heroBaseBonuses)
	// or MOVEMENT to compute initial movement before recruiting is finished
	for(const auto & ob : VLC->modh->heroBaseBonuses)
	{
		auto bonus = ob;
		bonus->source = Bonus::HERO_BASE_SKILL;
		bonus->sid = id.getNum();
		bonus->duration = Bonus::PERMANENT;
		addNewBonus(bonus);
	}

	if (VLC->modh->modules.COMMANDERS && !commander)
	{
		commander = new CCommanderInstance(type->heroClass->commander->idNumber);
		commander->setArmyObj (castToArmyObj()); //TODO: separate function for setting commanders
		commander->giveStackExp (exp); //after our exp is set
	}

	if (mana < 0)
		mana = manaLimit();
}

void CGHeroInstance::initArmy(CRandomGenerator & rand, IArmyDescriptor * dst)
{
	if(!dst)
		dst = this;

	int warMachinesGiven = 0;

	std::vector<int32_t> stacksCountChances = VLC->modh->settings.HERO_STARTING_ARMY_STACKS_COUNT_CHANCES;

	const int zeroStacksAllowingValue = -1;
	bool allowZeroStacksArmy = !stacksCountChances.empty() && stacksCountChances.back() == zeroStacksAllowingValue;
	if(allowZeroStacksArmy)
		stacksCountChances.pop_back();

	int stacksCountInitRandomNumber = rand.nextInt(1, 100);

	auto stacksCountElementIndex = vstd::find_pos_if(stacksCountChances, [stacksCountInitRandomNumber](int element){ return stacksCountInitRandomNumber < element; });
	if(stacksCountElementIndex == -1)
		stacksCountElementIndex = stacksCountChances.size();

	int howManyStacks = stacksCountElementIndex;
	if(!allowZeroStacksArmy)
		howManyStacks++;

	vstd::amin(howManyStacks, type->initialArmy.size());

	for(int stackNo=0; stackNo < howManyStacks; stackNo++)
	{
		auto & stack = type->initialArmy[stackNo];

		int count = rand.nextInt(stack.minAmount, stack.maxAmount);

		const CCreature * creature = stack.creature.toCreature();

		if(creature == nullptr)
		{
			logGlobal->error("Hero %s has invalid creature with id %d in initial army", getNameTranslated(), stack.creature.toEnum());
			continue;
		}

		if(creature->warMachine != ArtifactID::NONE) //war machine
		{
			warMachinesGiven++;
			if(dst != this)
				continue;

			ArtifactID aid = creature->warMachine;
			const CArtifact * art = aid.toArtifact();

			if(art != nullptr && !art->possibleSlots.at(ArtBearer::HERO).empty())
			{
				//TODO: should we try another possible slots?
				ArtifactPosition slot = art->possibleSlots.at(ArtBearer::HERO).front();

				if(!getArt(slot))
					putArtifact(slot, CArtifactInstance::createNewArtifactInstance(aid));
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
		if( cb->gameState()->getPlayerRelations(tempOwner, h->tempOwner)) //our or ally hero
		{
			//exchange
			cb->heroExchange(h->id, id);
		}
		else //battle
		{
			if(visitedTown) //we're in town
				visitedTown->onHeroVisit(h); //town will handle attacking
			else
				cb->startBattleI(h,	this);
		}
	}
	else if(ID == Obj::PRISON)
	{
		int txt_id;

		if (cb->getHeroCount(h->tempOwner, false) < VLC->modh->settings.MAX_HEROES_ON_MAP_PER_PLAYER)//free hero slot
		{
			//update hero parameters
			SetMovePoints smp;
			smp.hid = id;
			smp.val = maxMovePoints (true); //TODO: hota prison on water?
			cb->setMovePoints (&smp);
			cb->setManaPoints (id, manaLimit());

			cb->setObjProperty(id, ObjProperty::ID, Obj::HERO); //set ID to 34
			cb->giveHero(id,h->tempOwner); //recreates def and adds hero to player

			txt_id = 102;
		}
		else //already 8 wandering heroes
		{
			txt_id = 103;
		}

		h->showInfoDialog(txt_id);
	}
}

std::string CGHeroInstance::getObjectName() const
{
	if(ID != Obj::PRISON)
	{
		std::string hoverName = VLC->generaltexth->allTexts[15];
		boost::algorithm::replace_first(hoverName,"%s",getNameTranslated());
		boost::algorithm::replace_first(hoverName,"%s", type->heroClass->getNameTranslated());
		return hoverName;
	}
	else
		return VLC->objtypeh->getObjectName(ID, 0);
}

ui8 CGHeroInstance::maxlevelsToMagicSchool() const
{
	return type->heroClass->isMagicHero() ? 3 : 4;
}
ui8 CGHeroInstance::maxlevelsToWisdom() const
{
	return type->heroClass->isMagicHero() ? 3 : 6;
}

CGHeroInstance::SecondarySkillsInfo::SecondarySkillsInfo():
	magicSchoolCounter(1),
	wisdomCounter(1)
{
	rand.setSeed(0);
}

void CGHeroInstance::SecondarySkillsInfo::resetMagicSchoolCounter()
{
	magicSchoolCounter = 1;
}
void CGHeroInstance::SecondarySkillsInfo::resetWisdomCounter()
{
	wisdomCounter = 1;
}

void CGHeroInstance::initObj(CRandomGenerator & rand)
{
	blockVisit = true;

	if(!type)
		initHero(rand); //TODO: set up everything for prison before specialties are configured

	skillsInfo.rand.setSeed(rand.nextInt());
	skillsInfo.resetMagicSchoolCounter();
	skillsInfo.resetWisdomCounter();

	if (ID != Obj::PRISON)
	{
		auto terrain = cb->gameState()->getTile(visitablePos())->terType->getId();
		auto customApp = VLC->objtypeh->getHandlerFor(ID, type->heroClass->getIndex())->getOverride(terrain, this);
		if (customApp)
			appearance = customApp;
	}

	//copy active (probably growing) bonuses from hero prototype to hero object
	for(const std::shared_ptr<Bonus> & b : type->specialty)
		addNewBonus(b);
	for(SSpecialtyInfo & spec : type->specDeprecated)
		for(const std::shared_ptr<Bonus> & b : SpecialtyInfoToBonuses(spec, type->getIndex()))
			addNewBonus(b);

	//initialize bonuses
	recreateSecondarySkillsBonuses();

	mana = manaLimit(); //after all bonuses are taken into account, make sure this line is the last one
}

void CGHeroInstance::recreateSecondarySkillsBonuses()
{
	auto secondarySkillsBonuses = getBonuses(Selector::sourceType()(Bonus::SECONDARY_SKILL));
	for(const auto & bonus : *secondarySkillsBonuses)
		removeBonus(bonus);

	for(const auto & skill_info : secSkills)
		if(skill_info.second > 0)
			updateSkillBonus(SecondarySkill(skill_info.first), skill_info.second);
}

void CGHeroInstance::updateSkillBonus(const SecondarySkill & which, int val)
{
	removeBonuses(Selector::source(Bonus::SECONDARY_SKILL, which));
	auto skillBonus = (*VLC->skillh)[which]->at(val).effects;
	for(const auto & b : skillBonus)
		addNewBonus(std::make_shared<Bonus>(*b));
}

void CGHeroInstance::setPropertyDer( ui8 what, ui32 val )
{
	if(what == ObjProperty::PRIMARY_STACK_COUNT)
		setStackCount(SlotID(0), val);
}

double CGHeroInstance::getFightingStrength() const
{
	return sqrt((1.0 + 0.05*getPrimSkillLevel(PrimarySkill::ATTACK)) * (1.0 + 0.05*getPrimSkillLevel(PrimarySkill::DEFENSE)));
}

double CGHeroInstance::getMagicStrength() const
{
	return sqrt((1.0 + 0.05*getPrimSkillLevel(PrimarySkill::KNOWLEDGE)) * (1.0 + 0.05*getPrimSkillLevel(PrimarySkill::SPELL_POWER)));
}

double CGHeroInstance::getHeroStrength() const
{
	return sqrt(pow(getFightingStrength(), 2.0) * pow(getMagicStrength(), 2.0));
}

ui64 CGHeroInstance::getTotalStrength() const
{
	double ret = getFightingStrength() * getArmyStrength();
	return static_cast<ui64>(ret);
}

TExpType CGHeroInstance::calculateXp(TExpType exp) const
{
	return static_cast<TExpType>(exp * (100 + valOfBonuses(Bonus::SECONDARY_SKILL_PREMY, SecondarySkill::LEARNING)) / 100.0);
}

int32_t CGHeroInstance::getCasterUnitId() const
{
	return -1; //TODO: special value for attacker/defender hero
}

int32_t CGHeroInstance::getSpellSchoolLevel(const spells::Spell * spell, int32_t * outSelectedSchool) const
{
	int32_t skill = -1; //skill level

	spell->forEachSchool([&, this](const spells::SchoolInfo & cnf, bool & stop)
	{
		int32_t thisSchool = std::max<int32_t>(
			valOfBonuses(Bonus::SECONDARY_SKILL_PREMY, cnf.skill),
			valOfBonuses(Bonus::MAGIC_SCHOOL_SKILL, 1 << (static_cast<ui8>(cnf.id)))); //FIXME: Bonus shouldn't be additive (Witchking Artifacts : Crown of Skies)
		if(thisSchool > skill)
		{
			skill = thisSchool;
			if(outSelectedSchool)
				*outSelectedSchool = static_cast<ui8>(cnf.id);
		}
	});

	vstd::amax(skill, valOfBonuses(Bonus::MAGIC_SCHOOL_SKILL, 0)); //any school bonus
	vstd::amax(skill, valOfBonuses(Bonus::SPELL, spell->getIndex())); //given by artifact or other effect

	vstd::amax(skill, 0); //in case we don't know any school
	vstd::amin(skill, 3);
	return skill;
}

int64_t CGHeroInstance::getSpellBonus(const spells::Spell * spell, int64_t base, const battle::Unit * affectedStack) const
{
	//applying sorcery secondary skill

	base = static_cast<int64_t>(base * (100 + valOfBonuses(Bonus::SECONDARY_SKILL_PREMY, SecondarySkill::SORCERY)) / 100.0);
	base = static_cast<int64_t>(base * (100 + valOfBonuses(Bonus::SPELL_DAMAGE) + valOfBonuses(Bonus::SPECIFIC_SPELL_DAMAGE, spell->getIndex())) / 100.0);

	int maxSchoolBonus = 0;

	spell->forEachSchool([&maxSchoolBonus, this](const spells::SchoolInfo & cnf, bool & stop)
	{
		vstd::amax(maxSchoolBonus, valOfBonuses(cnf.damagePremyBonus));
	});

	base = static_cast<int64_t>(base * (100 + maxSchoolBonus) / 100.0);

	if(affectedStack && affectedStack->creatureLevel() > 0) //Hero specials like Solmyr, Deemer
		base = static_cast<int64_t>(base * static_cast<double>(100 + valOfBonuses(Bonus::SPECIAL_SPELL_LEV, spell->getIndex()) / affectedStack->creatureLevel()) / 100.0);

	return base;
}

int64_t CGHeroInstance::getSpecificSpellBonus(const spells::Spell * spell, int64_t base) const
{
	base = static_cast<int64_t>(base * (100 + valOfBonuses(Bonus::SPECIFIC_SPELL_DAMAGE, spell->getIndex())) / 100.0);
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
	return getPrimSkillLevel(PrimarySkill::SPELL_POWER) + valOfBonuses(Bonus::SPELL_DURATION);
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
	text.addReplacement(getNameTranslated());
}

void CGHeroInstance::getCastDescription(const spells::Spell * spell, const std::vector<const battle::Unit *> & attacked, MetaString & text) const
{
	const bool singleTarget = attacked.size() == 1;
	const int textIndex = singleTarget ? 195 : 196;

	text.addTxt(MetaString::GENERAL_TXT, textIndex);
	getCasterName(text);
	text.addReplacement(MetaString::SPELL_NAME, spell->getIndex());
	if(singleTarget)
		attacked.at(0)->addNameReplacement(text, true);
}

void CGHeroInstance::spendMana(ServerCallback * server, const int spellCost) const
{
	if(spellCost != 0)
	{
		SetMana sm;
		sm.absolute = false;
		sm.hid = id;
		sm.val = -spellCost;

		server->apply(&sm);
	}
}

bool CGHeroInstance::canCastThisSpell(const spells::Spell * spell) const
{
	const bool isAllowed = IObjectInterface::cb->isAllowed(0, spell->getIndex());

	const bool inSpellBook = vstd::contains(spells, spell->getId()) && hasSpellbook();
	const bool specificBonus = hasBonusOfType(Bonus::SPELL, spell->getIndex());

	bool schoolBonus = false;

	spell->forEachSchool([this, &schoolBonus](const spells::SchoolInfo & cnf, bool & stop)
	{
		if(hasBonusOfType(cnf.knoledgeBonus))
		{
			schoolBonus = stop = true;
		}
	});

	const bool levelBonus = hasBonusOfType(Bonus::SPELLS_OF_LEVEL, spell->getLevel());

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

bool CGHeroInstance::canLearnSpell(const spells::Spell * spell) const
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

	if(!IObjectInterface::cb->isAllowed(0, spell->getIndex()))
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
	const ui8 necromancyLevel = getSecSkillLevel(SecondarySkill::NECROMANCY);
	// need skill or cloak of undead king - lesser artifacts don't work without skill
	if (necromancyLevel > 0 || hasBonusOfType(Bonus::IMPROVED_NECROMANCY))
	{
		double necromancySkill = valOfBonuses(Bonus::SECONDARY_SKILL_PREMY, SecondarySkill::NECROMANCY) / 100.0;
		vstd::amin(necromancySkill, 1.0); //it's impossible to raise more creatures than all...
		const std::map<ui32,si32> &casualties = battleResult.casualties[!battleResult.winner];
		// figure out what to raise - pick strongest creature meeting requirements
		CreatureID creatureTypeRaised = CreatureID::SKELETON;
		int requiredCasualtyLevel = 1;
		TConstBonusListPtr improvedNecromancy = getBonuses(Selector::type()(Bonus::IMPROVED_NECROMANCY));
		if(!improvedNecromancy->empty())
		{
			auto getCreatureID = [necromancyLevel](const std::shared_ptr<Bonus> & bonus) -> CreatureID
			{
				const CreatureID legacyTypes[] = {CreatureID::SKELETON, CreatureID::WALKING_DEAD, CreatureID::WIGHTS, CreatureID::LICHES};
				return CreatureID(bonus->subtype >= 0 ? bonus->subtype : legacyTypes[necromancyLevel]);
			};
			int maxCasualtyLevel = 1;
			for(const auto & casualty : casualties)
				vstd::amax(maxCasualtyLevel, VLC->creh->objects[casualty.first]->level);
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
					auto quality = [getCreatureID](const std::shared_ptr<Bonus> & pick) -> std::tuple<int, int, int>
					{
						const CCreature * c = VLC->creh->objects[getCreatureID(pick)];
						return std::tuple<int, int, int> {c->level, static_cast<int>(c->cost.marketValue()), -pick->additionalInfo[1]};
					};
					if(quality(topPick) < quality(newPick))
						topPick = newPick;
				}
			}
			if(topPick)
			{
				creatureTypeRaised = getCreatureID(topPick);
				requiredCasualtyLevel = std::max(topPick->additionalInfo[1], 1);
			}
		}
		// raise upgraded creature (at 2/3 rate) if no space available otherwise
		if(getSlotFor(creatureTypeRaised) == SlotID())
		{
			for(const CreatureID & upgraded : VLC->creh->objects[creatureTypeRaised]->upgrades)
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
		const double raisedUnitHealth = VLC->creh->objects[creatureTypeRaised]->MaxHealth();
		double raisedUnits = 0;
		for(const auto & casualty : casualties)
		{
			const CCreature * c = VLC->creh->objects[casualty.first];
			double raisedFromCasualty = std::min(c->MaxHealth() / raisedUnitHealth, 1.0) * casualty.second * necromancySkill;
			if(c->level < requiredCasualtyLevel)
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
void CGHeroInstance::showNecromancyDialog(const CStackBasicDescriptor &raisedStack, CRandomGenerator & rand) const
{
	InfoWindow iw;
	iw.type = EInfoWindowMode::AUTO;
	iw.soundID = soundBase::pickup01 + rand.nextInt(6);
	iw.player = tempOwner;
	iw.components.emplace_back(raisedStack);

	if (raisedStack.count > 1) // Practicing the dark arts of necromancy, ... (plural)
	{
		iw.text.addTxt(MetaString::GENERAL_TXT, 145);
		iw.text.addReplacement(raisedStack.count);
	}
	else // Practicing the dark arts of necromancy, ... (singular)
	{
		iw.text.addTxt(MetaString::GENERAL_TXT, 146);
	}
	iw.text.addReplacement(raisedStack);

	cb->showInfoDialog(&iw);
}
/*
int3 CGHeroInstance::getSightCenter() const
{
	return getPosition(false);
}*/

int CGHeroInstance::getSightRadius() const
{
	return valOfBonuses(Bonus::SIGHT_RADIUS); // scouting gives SIGHT_RADIUS bonus
}

si32 CGHeroInstance::manaRegain() const
{
	if (hasBonusOfType(Bonus::FULL_MANA_REGENERATION))
		return manaLimit();

	return valOfBonuses(Bonus::SECONDARY_SKILL_PREMY, SecondarySkill::MYSTICISM) + valOfBonuses(Bonus::MANA_REGENERATION); //1 + Mysticism level
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
// 	CArtifact * const artifact = VLC->arth->objects[aid]; //pointer to constant object
// 	CArtifactInstance *ai = CArtifactInstance::createNewArtifactInstance(artifact);
// 	ai->putAt(this, ai->firstAvailableSlot(this));
// }

int CGHeroInstance::getBoatType() const
{
	switch(type->heroClass->getAlignment())
	{
	case EAlignment::GOOD:
		return 1;
	case EAlignment::EVIL:
		return 0;
	case EAlignment::NEUTRAL:
		return 2;
	default:
		throw std::runtime_error("Wrong alignment!");
	}
}

void CGHeroInstance::getOutOffsets(std::vector<int3> &offsets) const
{
	// FIXME: Offsets need to be fixed once we get rid of convertPosition
	// Check issue 515 for details
	offsets =
	{
		int3(-1,1,0), int3(-1,-1,0), int3(-2,0,0), int3(0,0,0), int3(0,1,0), int3(-2,1,0), int3(0,-1,0), int3(-2,-1,0)
	};
}

int32_t CGHeroInstance::getSpellCost(const spells::Spell * sp) const
{
	return sp->getCost(getSpellSchoolLevel(sp));
}

void CGHeroInstance::pushPrimSkill( PrimarySkill::PrimarySkill which, int val )
{
	assert(!hasBonus(Selector::typeSubtype(Bonus::PRIMARY_SKILL, which)
						.And(Selector::sourceType()(Bonus::HERO_BASE_SKILL))));
	addNewBonus(std::make_shared<Bonus>(Bonus::PERMANENT, Bonus::PRIMARY_SKILL, Bonus::HERO_BASE_SKILL, val, id.getNum(), which));
}

EAlignment::EAlignment CGHeroInstance::getAlignment() const
{
	return type->heroClass->getAlignment();
}

void CGHeroInstance::initExp(CRandomGenerator & rand)
{
	exp = rand.nextInt(40, 89);
}

std::string CGHeroInstance::nodeName() const
{
	return "Hero " + getNameTextID();
}

std::string CGHeroInstance::getNameTranslated() const
{
	if (!nameCustom.empty())
		return nameCustom;
	return VLC->generaltexth->translate(getNameTextID());
}

std::string CGHeroInstance::getNameTextID() const
{
	if (!nameCustom.empty())
		return nameCustom;
	if (type)
		return type->getNameTextID();

	// FIXME: called by logging from some specialties (mods?) before type is set on deserialization
	// assert(0);
	return "";
}

std::string CGHeroInstance::getBiographyTranslated() const
{
	if (!biographyCustom.empty())
		return biographyCustom;

	return VLC->generaltexth->translate(getBiographyTextID());
}

std::string CGHeroInstance::getBiographyTextID() const
{
	if (!biographyCustom.empty())
		return biographyCustom;
	if (type)
		return type->getBiographyTextID();

	assert(0);
	return "";
}

void CGHeroInstance::putArtifact(ArtifactPosition pos, CArtifactInstance *art)
{
	assert(!getArt(pos));
	art->putAt(ArtifactLocation(this, pos));
}

void CGHeroInstance::putInBackpack(CArtifactInstance *art)
{
	putArtifact(art->firstBackpackSlot(this), art);
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
		ArtifactLocation(this, ArtifactPosition(ArtifactPosition::SPELLBOOK)).removeArtifact();
	}
}

const std::set<SpellID> & CGHeroInstance::getSpellsInSpellbook() const
{
	return spells;
}

int CGHeroInstance::maxSpellLevel() const
{
	return std::min(GameConstants::SPELL_LEVELS, 2 + valOfBonuses(Selector::typeSubtype(Bonus::SECONDARY_SKILL_PREMY, SecondarySkill::WISDOM)));
}

void CGHeroInstance::deserializationFix()
{
	artDeserializationFix(this);
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
	int ret = 0; //take all MPs by default
	bool localTi = false;
	if(!ti)
	{
		localTi = true;
		ti = new TurnInfo(this);
	}

	int mp1 = ti->getMaxMovePoints(disembark ? EPathfindingLayer::LAND : EPathfindingLayer::SAIL);
	int mp2 = ti->getMaxMovePoints(disembark ? EPathfindingLayer::SAIL : EPathfindingLayer::LAND);
	if(ti->hasBonusOfType(Bonus::FREE_SHIP_BOARDING))
		ret = static_cast<int>((MPsBefore - basicCost) * static_cast<double>(mp1) / mp2);

	if(localTi)
		delete ti;

	return ret;
}

EDiggingStatus CGHeroInstance::diggingStatus() const
{
	if(static_cast<int>(movement) < maxMovePoints(true))
		return EDiggingStatus::LACK_OF_MOVEMENT;

	return cb->getTileDigStatus(visitablePos());
}

ArtBearer::ArtBearer CGHeroInstance::bearerType() const
{
	return ArtBearer::HERO;
}

std::vector<SecondarySkill> CGHeroInstance::getLevelUpProposedSecondarySkills() const
{
	std::vector<SecondarySkill> obligatorySkills; //hero is offered magic school or wisdom if possible

	auto getObligatorySkills = [](CSkill::Obligatory obl){
		std::vector<SecondarySkill> obligatory = {};
		for(int i = 0; i < VLC->skillh->size(); i++)
			if((*VLC->skillh)[SecondarySkill(i)]->obligatory(obl))
			{
				obligatory.emplace_back(i);
				break;
			}
		return obligatory;
	};

	auto selectObligatorySkill = [&](std::vector<SecondarySkill>& ss) -> void
	{
		std::shuffle(ss.begin(), ss.end(), skillsInfo.rand.getStdGenerator());

		for(const auto & skill : ss)
		{
			if (canLearnSkill(skill)) //only skills hero doesn't know yet
			{
				obligatorySkills.push_back(skill);
				break; //only one
			}
		}
	};

	if (!skillsInfo.wisdomCounter)
	{
		auto obligatory = getObligatorySkills(CSkill::Obligatory::MAJOR);
		selectObligatorySkill(obligatory);
	}
	if (!skillsInfo.magicSchoolCounter)
	{
		auto obligatory = getObligatorySkills(CSkill::Obligatory::MINOR);
		selectObligatorySkill(obligatory);
	}

	std::vector<SecondarySkill> skills;
	//picking sec. skills for choice
	std::set<SecondarySkill> basicAndAdv;
	std::set<SecondarySkill> expert;
	std::set<SecondarySkill> none;
	for(int i = 0; i < VLC->skillh->size(); i++)
		if (canLearnSkill(SecondarySkill(i)))
			none.insert(SecondarySkill(i));

	for(const auto & elem : secSkills)
	{
		if(elem.second < SecSkillLevel::EXPERT)
			basicAndAdv.insert(elem.first);
		else
			expert.insert(elem.first);
		none.erase(elem.first);
	}
	for(const auto & s : obligatorySkills) //don't duplicate them
	{
		none.erase (s);
		basicAndAdv.erase (s);
		expert.erase (s);
	}

	//first offered skill:
	// 1) give obligatory skill
	// 2) give any other new skill
	// 3) upgrade existing
	if(canLearnSkill() && !obligatorySkills.empty())
	{
		skills.push_back (obligatorySkills[0]);
	}
	else if(!none.empty() && canLearnSkill()) //hero have free skill slot
	{
		skills.push_back(type->heroClass->chooseSecSkill(none, skillsInfo.rand)); //new skill
		none.erase(skills.back());
	}
	else if(!basicAndAdv.empty())
	{
		skills.push_back(type->heroClass->chooseSecSkill(basicAndAdv, skillsInfo.rand)); //upgrade existing
		basicAndAdv.erase(skills.back());
	}

	//second offered skill:
	//1) upgrade existing
	//2) give obligatory skill
	//3) give any other new skill
	if(!basicAndAdv.empty())
	{
		SecondarySkill s = type->heroClass->chooseSecSkill(basicAndAdv, skillsInfo.rand);//upgrade existing
		skills.push_back(s);
		basicAndAdv.erase(s);
	}
	else if (canLearnSkill() && obligatorySkills.size() > 1)
	{
		skills.push_back (obligatorySkills[1]);
	}
	else if(!none.empty() && canLearnSkill())
	{
		skills.push_back(type->heroClass->chooseSecSkill(none, skillsInfo.rand)); //give new skill
		none.erase(skills.back());
	}

	if (skills.size() == 2) // Fix for #1868 to avoid changing logic (possibly causing bugs in process)
		std::swap(skills[0], skills[1]);
	return skills;
}

PrimarySkill::PrimarySkill CGHeroInstance::nextPrimarySkill(CRandomGenerator & rand) const
{
	assert(gainsLevel());
	int randomValue = rand.nextInt(99);
	int pom = 0;
	int primarySkill = 0;
	const auto isLowLevelHero = level < GameConstants::HERO_HIGH_LEVEL;
	const auto & skillChances = isLowLevelHero ? type->heroClass->primarySkillLowLevel : type->heroClass->primarySkillHighLevel;

	for(; primarySkill < GameConstants::PRIMARY_SKILLS; ++primarySkill)
	{
		pom += skillChances[primarySkill];
		if(randomValue < pom)
		{
			break;
		}
	}
	if(primarySkill >= GameConstants::PRIMARY_SKILLS)
	{
		primarySkill = rand.nextInt(GameConstants::PRIMARY_SKILLS - 1);
		logGlobal->error("Wrong values in primarySkill%sLevel for hero class %s", isLowLevelHero ? "Low" : "High", type->heroClass->getNameTranslated());
		randomValue = 100 / GameConstants::PRIMARY_SKILLS;
	}
	logGlobal->trace("The hero gets the primary skill %d with a probability of %d %%.", primarySkill, randomValue);
	return static_cast<PrimarySkill::PrimarySkill>(primarySkill);
}

boost::optional<SecondarySkill> CGHeroInstance::nextSecondarySkill(CRandomGenerator & rand) const
{
	assert(gainsLevel());

	boost::optional<SecondarySkill> chosenSecondarySkill;
	const auto proposedSecondarySkills = getLevelUpProposedSecondarySkills();
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
			chosenSecondarySkill = boost::make_optional(*RandomGeneratorUtil::nextItem(proposedSecondarySkills, rand));
		}
		else
		{
			// preferably upgrade a already learned secondary skill
			chosenSecondarySkill = boost::make_optional(*RandomGeneratorUtil::nextItem(learnedSecondarySkills, rand));
		}
	}
	return chosenSecondarySkill;
}

void CGHeroInstance::setPrimarySkill(PrimarySkill::PrimarySkill primarySkill, si64 value, ui8 abs)
{
	if(primarySkill < PrimarySkill::EXPERIENCE)
	{
		auto skill = getBonusLocalFirst(Selector::type()(Bonus::PRIMARY_SKILL)
			.And(Selector::subtype()(primarySkill))
			.And(Selector::sourceType()(Bonus::HERO_BASE_SKILL)));
		assert(skill);

		if(abs)
		{
			skill->val = static_cast<si32>(value);
		}
		else
		{
			skill->val += static_cast<si32>(value);
		}
		CBonusSystemNode::treeHasChanged();
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
	return exp >= static_cast<TExpType>(VLC->heroh->reqExp(level+1));
}

void CGHeroInstance::levelUp(const std::vector<SecondarySkill> & skills)
{
	++level;

	//deterministic secondary skills
	skillsInfo.magicSchoolCounter = (skillsInfo.magicSchoolCounter + 1) % maxlevelsToMagicSchool();
	skillsInfo.wisdomCounter = (skillsInfo.wisdomCounter + 1) % maxlevelsToWisdom();
	for(const auto & skill : skills)
	{
		if((*VLC->skillh)[skill]->obligatory(CSkill::Obligatory::MAJOR))
			skillsInfo.resetWisdomCounter();
		if((*VLC->skillh)[skill]->obligatory(CSkill::Obligatory::MINOR))
			skillsInfo.resetMagicSchoolCounter();
	}

	//update specialty and other bonuses that scale with level
	treeHasChanged();
}

void CGHeroInstance::levelUpAutomatically(CRandomGenerator & rand)
{
	while(gainsLevel())
	{
		const auto primarySkill = nextPrimarySkill(rand);
		setPrimarySkill(primarySkill, 1, false);

		auto proposedSecondarySkills = getLevelUpProposedSecondarySkills();

		const auto secondarySkill = nextSecondarySkill(rand);
		if(secondarySkill)
		{
			setSecSkillLevel(*secondarySkill, 1, false);
		}

		//TODO why has the secondary skills to be passed to the method?
		levelUp(proposedSecondarySkills);
	}
}

bool CGHeroInstance::hasVisions(const CGObjectInstance * target, const int subtype) const
{
	//VISIONS spell support

	const std::string cached = boost::to_string((boost::format("type_%d__subtype_%d") % Bonus::VISIONS % subtype));

	const int visionsMultiplier = valOfBonuses(Selector::typeSubtype(Bonus::VISIONS,subtype), cached);

	int visionsRange =  visionsMultiplier * getPrimSkillLevel(PrimarySkill::SPELL_POWER);

	if (visionsMultiplier > 0)
		vstd::amax(visionsRange, 3); //minimum range is 3 tiles, but only if VISIONS bonus present

	const int distance = static_cast<int>(target->pos.dist2d(visitablePos()));

	//logGlobal->debug(boost::to_string(boost::format("Visions: dist %d, mult %d, range %d") % distance % visionsMultiplier % visionsRange));

	return (distance < visionsRange) && (target->pos.z == pos.z);
}

std::string CGHeroInstance::getHeroTypeName() const
{
	if(ID == Obj::HERO || ID == Obj::PRISON)
	{
		if(type)
		{
			return type->getJsonKey();
		}
		else
		{
			return VLC->heroh->objects[subID]->getJsonKey();
		}
	}
	return "";
}

void CGHeroInstance::afterAddToMap(CMap * map)
{
	if(ID == Obj::HERO)
		map->heroesOnMap.emplace_back(this);
}
void CGHeroInstance::afterRemoveFromMap(CMap* map)
{
	if (ID == Obj::HERO)
		vstd::erase_if_present(map->heroesOnMap, this);
}

void CGHeroInstance::setHeroTypeName(const std::string & identifier)
{
	if(ID == Obj::HERO || ID == Obj::PRISON)
	{
		auto rawId = VLC->modh->identifiers.getIdentifier(CModHandler::scopeMap(), "hero", identifier);

		if(rawId)
			subID = rawId.get();
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
	handler.serializeString("biography", biographyCustom);
	handler.serializeInt("experience", exp, 0);

	if(!handler.saving && exp != UNINITIALIZED_EXPERIENCE) //do not gain levels if experience is not initialized
	{
		while (gainsLevel())
		{
			++level;
		}
	}

	handler.serializeString("name", nameCustom);
	handler.serializeBool<ui8>("female", sex, 1, 0, 0xFF);

	{
		const int legacyHeroes = static_cast<int>(VLC->modh->settings.data["textData"]["hero"].Integer());
		const int moddedStart = legacyHeroes + GameConstants::HERO_PORTRAIT_SHIFT;

		if(handler.saving)
		{
			if(portrait >= 0)
			{
				if(portrait < legacyHeroes || portrait >= moddedStart)
					handler.serializeId<si32, si32, HeroTypeID>("portrait", portrait, -1);
				else
					handler.serializeInt("portrait", portrait, -1);
			}
		}
		else
		{
			const JsonNode & portraitNode = handler.getCurrent()["portrait"];

			if(portraitNode.getType() == JsonNode::JsonType::DATA_STRING)
				handler.serializeId<si32, si32, HeroTypeID>("portrait", portrait, -1);
			else
				handler.serializeInt("portrait", portrait, -1);
		}
	}

	//primary skills
	if(handler.saving)
	{
		const bool haveSkills = hasBonus(Selector::type()(Bonus::PRIMARY_SKILL).And(Selector::sourceType()(Bonus::HERO_BASE_SKILL)));

		if(haveSkills)
		{
			auto primarySkills = handler.enterStruct("primarySkills");

			for(int i = 0; i < GameConstants::PRIMARY_SKILLS; ++i)
			{
				int value = valOfBonuses(Selector::typeSubtype(Bonus::PRIMARY_SKILL, i).And(Selector::sourceType()(Bonus::HERO_BASE_SKILL)));

				handler.serializeInt(PrimarySkill::names[i], value, 0);
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
				handler.serializeInt(PrimarySkill::names[i], value, 0);
				pushPrimSkill(static_cast<PrimarySkill::PrimarySkill>(i), value);
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
			if(p.first == SecondarySkill(SecondarySkill::DEFAULT))
				defaultSkills = true;
			else
				normalSkills = true;
		}

		if(defaultSkills && normalSkills)
			logGlobal->error("Mixed default and normal secondary skills");

		//in json default skills means no field/null
		if(!defaultSkills)
		{
			//enter structure here as handler initialize it
			auto secondarySkills = handler.enterStruct("secondarySkills");

			for(auto & p : secSkills)
			{
				const si32 rawId = p.first.num;

				if(rawId < 0 || rawId >= VLC->skillh->size())
					logGlobal->error("Invalid secondary skill %d", rawId);

				handler.serializeEnum((*VLC->skillh)[SecondarySkill(rawId)]->getJsonKey(), p.second, 0, NSecondarySkill::levels);
			}
		}
	}
	else
	{
		auto secondarySkills = handler.enterStruct("secondarySkills");
		const JsonNode & skillMap = handler.getCurrent();

		secSkills.clear();
		if(skillMap.getType() == JsonNode::JsonType::DATA_NULL)
		{
			secSkills.emplace_back(SecondarySkill::DEFAULT, -1);
		}
		else
		{
			for(const auto & p : skillMap.Struct())
			{
				const std::string skillId = p.first;
				const std::string levelId =  p.second.String();

				const int rawId = CSkillHandler::decodeSkill(skillId);
				if(rawId < 0)
				{
					logGlobal->error("Invalid secondary skill %s", skillId);
					continue;
				}

				const int level = vstd::find_pos(NSecondarySkill::levels, levelId);
				if(level < 0)
				{
					logGlobal->error("Invalid secondary skill level%s", levelId);
					continue;
				}

				secSkills.emplace_back(SecondarySkill(rawId), level);
			}
		}
	}

	handler.serializeIdArray("spellBook", spells);

	if(handler.saving)
		CArtifactSet::serializeJsonArtifacts(handler, "artifacts", nullptr);
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

	CCreatureSet::serializeJson(handler, "army", 7);
	handler.serializeBool<ui8>("tightFormation", formation, 1, 0, 0);

	{
		static constexpr int NO_PATROLING = -1;
		int rawPatrolRadius = NO_PATROLING;

		if(handler.saving)
		{
			rawPatrolRadius = patrol.patrolling ? patrol.patrolRadius : NO_PATROLING;
		}

		handler.serializeInt("patrolRadius", rawPatrolRadius, NO_PATROLING);

		if(!handler.saving)
		{
			if(!appearance)
			{
				// crossoverDeserialize
				type = VLC->heroh->objects[subID];
				appearance = VLC->objtypeh->getHandlerFor(Obj::HERO, type->heroClass->getIndex())->getTemplates().front();
			}

			patrol.patrolling = (rawPatrolRadius > NO_PATROLING);
			patrol.initialPos = visitablePos();
			patrol.patrolRadius = (rawPatrolRadius > NO_PATROLING) ? rawPatrolRadius : 0;
		}
	}
}

void CGHeroInstance::serializeJsonDefinition(JsonSerializeFormat & handler)
{
	serializeCommonOptions(handler);
}

bool CGHeroInstance::isMissionCritical() const
{
	for(const TriggeredEvent & event : IObjectInterface::cb->getMapHeader()->triggeredEvents)
	{
		if(event.trigger.test([&](const EventCondition & condition)
		{
			if ((condition.condition == EventCondition::CONTROL || condition.condition == EventCondition::HAVE_0) && condition.object)
			{
				const auto * hero = dynamic_cast<const CGHeroInstance *>(condition.object);
				return (hero != this);
			}
			else if(condition.condition == EventCondition::IS_HUMAN)
			{
				return true;
			}
			return false;
		}))
		{
			return true;
		}
	}
	return false;
}

VCMI_LIB_NAMESPACE_END
