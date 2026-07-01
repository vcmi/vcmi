/*
 * CSpell.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "CSpell.h"

#include "Problem.h"
#include "SpellSchoolHandler.h"
#include "ISpellMechanics.h"

#include "../CBonusTypeHandler.h"
#include "../battle/Unit.h"
#include "../bonuses/BonusSelector.h"
#include "../GameLibrary.h"
#include "../json/JsonBonus.h"
#include "../texts/CGeneralTextHandler.h"

#include <vcmi/spells/Caster.h>

VCMI_LIB_NAMESPACE_BEGIN

static constexpr std::array LEVEL_NAMES = {"none", "basic", "advanced", "expert"};

///CSpell
CSpell::CSpell():
	id(SpellID::NONE),
	level(0),
	power(0),
	combat(false),
	creatureAbility(false),
	castOnSelf(false),
	castOnlyOnSelf(false),
	castWithoutSkip(false),
	positiveness(ESpellPositiveness::NEUTRAL),
	defaultProbability(0),
	rising(false),
	damage(false),
	offensive(false),
	special(true),
	nonMagical(false),
	targetType(spells::AimType::NOTHING)
{
	levels.resize(GameConstants::SPELL_SCHOOL_LEVELS);
}

//must be instantiated in .cpp file for access to complete types of all member fields
CSpell::~CSpell() = default;

bool CSpell::adventureCast(SpellCastEnvironment * env, const AdventureSpellCastParameters & parameters) const
{
	assert(env);

	if(!adventureMechanics)
	{
		env->complain("Invalid adventure spell cast attempt!");
		return false;
	}
	return adventureMechanics->adventureCast(env, parameters);
}

const CSpell::LevelInfo & CSpell::getLevelInfo(const int32_t schoolLevel) const
{
	if(schoolLevel < 0 || schoolLevel >= GameConstants::SPELL_SCHOOL_LEVELS)
	{
		logGlobal->error("CSpell::getLevelInfo: invalid school mastery level %d", schoolLevel);
		return levels.at(MasteryLevel::EXPERT);
	}

	return levels.at(schoolLevel);
}

int64_t CSpell::calculateDamage(const spells::Caster * caster) const
{
	//check if spell really does damage - if not, return 0
	if(!isDamage())
		return 0;
	auto rawDamage = calculateRawEffectValue(caster->getEffectLevel(this), caster->getEffectPower(this), 1);

	return caster->getSpellBonus(this, rawDamage, nullptr);
}

bool CSpell::hasSchool(SpellSchool which) const
{
	return schools.count(which);
}

bool CSpell::canBeCast(const CBattleInfoCallback * cb, spells::Mode mode, const spells::Caster * caster) const
{
	//if caller do not interested in description just discard it and do not pollute even debug log
	spells::detail::ProblemImpl problem;
	return canBeCast(problem, cb, mode, caster);
}

bool CSpell::canBeCast(spells::Problem & problem, const CBattleInfoCallback * cb, spells::Mode mode, const spells::Caster * caster) const
{
	spells::BattleCast event(cb, caster, mode, this);
	auto mechanics = battleMechanics(&event);

	return mechanics->canBeCast(problem);
}

spells::AimType CSpell::getTargetType() const
{
	return targetType;
}

void CSpell::forEachSchool(const std::function<void(const SpellSchool &, bool &)>& cb) const
{
	bool stop = false;
	for(auto schoolID : LIBRARY->spellSchoolHandler->getAllObjects())
	{
		if(schools.count(schoolID))
		{
			cb(schoolID, stop);
			if(stop)
				break;
		}
	}
}

SpellID CSpell::getId() const
{
	return id;
}

std::string CSpell::getNameTextID() const
{
	TextIdentifier textId("spell", modScope, identifier, "name");
	return textId.get();
}

std::string CSpell::getNameTranslated() const
{
	return LIBRARY->generaltexth->translate(getNameTextID());
}

std::string CSpell::getDescriptionTextID(int32_t schoolLevel) const
{
	TextIdentifier textID("spell", modScope, identifier, "description", LEVEL_NAMES[schoolLevel]);
	return textID.get();
}

std::string CSpell::getDescriptionTranslated(int32_t schoolLevel) const
{
	return LIBRARY->generaltexth->translate(getDescriptionTextID(schoolLevel));
}

std::string CSpell::getAdventureEffectTextID(const std::string & effectType, const std::string & field) const
{
	TextIdentifier textID("spell", modScope, identifier, "adventureEffect", effectType, field);
	return textID.get();
}

std::string CSpell::getJsonKey() const
{
	return modScope + ':' + identifier;
}

std::string CSpell::getModScope() const
{
	return modScope;
}

int32_t CSpell::getIndex() const
{
	return id.toEnum();
}

int32_t CSpell::getIconIndex() const
{
	return getIndex();
}

int32_t CSpell::getLevel() const
{
	return level;
}

bool CSpell::isCombat() const
{
	return combat;
}

bool CSpell::isAdventure() const
{
	return !combat;
}

bool CSpell::isCreatureAbility() const
{
	return creatureAbility;
}

bool CSpell::isMagical() const
{
	return !nonMagical;
}

bool CSpell::isPositive() const
{
	return positiveness == POSITIVE;
}

bool CSpell::isNegative() const
{
	return positiveness == NEGATIVE;
}

bool CSpell::isNeutral() const
{
	return positiveness == NEUTRAL;
}

bool CSpell::isPersistent() const
{
	return persistent;
}

boost::logic::tribool CSpell::getPositiveness() const
{
	switch (positiveness)
	{
	case CSpell::POSITIVE:
		return true;
	case CSpell::NEGATIVE:
		return false;
	default:
		return boost::logic::indeterminate;
	}
}

bool CSpell::isDamage() const
{
	return damage;
}

bool CSpell::isOffensive() const
{
	return offensive;
}

bool CSpell::isSpecial() const
{
	return special;
}

bool CSpell::hasEffects() const
{
	return !levels[0].effects.Struct().empty() || !levels[0].cumulativeEffects.Struct().empty();
}

bool CSpell::hasBattleEffects() const
{
	return levels[0].battleEffects.getType() == JsonNode::JsonType::DATA_STRUCT && !levels[0].battleEffects.Struct().empty();
}

bool CSpell::canCastOnSelf() const
{
	return castOnSelf;
}

bool CSpell::canCastOnlyOnSelf() const
{
	return castOnlyOnSelf;
}

bool CSpell::canCastWithoutSkip() const
{
	return castWithoutSkip;
}

const ImagePath & CSpell::getIconImmune() const
{
	return iconImmune;
}

const std::string & CSpell::getIconBook() const
{
	return iconBook;
}

const std::string & CSpell::getIconEffect() const
{
	return iconEffect;
}

const std::string & CSpell::getIconScenarioBonus() const
{
	return iconScenarioBonus;
}

const std::string & CSpell::getIconScroll() const
{
	return iconScroll;
}

const AudioPath & CSpell::getCastSound() const
{
	return castSound;
}

int32_t CSpell::getCost(const int32_t skillLevel) const
{
	return getLevelInfo(skillLevel).cost;
}

int32_t CSpell::getBasePower() const
{
	return power;
}

int32_t CSpell::getLevelPower(const int32_t skillLevel) const
{
	return getLevelInfo(skillLevel).power;
}

si32 CSpell::getProbability(const FactionID & factionId) const
{
	if(!vstd::contains(probabilities, factionId))
	{
		return defaultProbability;
	}
	return probabilities.at(factionId);
}

void CSpell::getEffects(std::vector<Bonus> & lst, const int schoolLevel, const bool cumulative, const si32 duration, std::optional<si32 *> maxDuration) const
{
	if(schoolLevel < 0 || schoolLevel >= GameConstants::SPELL_SCHOOL_LEVELS)
	{
		logGlobal->error("invalid school level %d", schoolLevel);
		return;
	}

	const auto & levelObject = levels.at(schoolLevel);

	const auto & effectsJson = cumulative ? levelObject.cumulativeEffects : levelObject.effects;

	if(effectsJson.Struct().empty())
	{
		logGlobal->error("This spell (%s) has no effects for level %d", getNameTranslated(), schoolLevel);
		return;
	}

	for(const auto & [name, bonusNode] : effectsJson.Struct())
	{
		auto b = JsonUtils::parseBonus(bonusNode);
		if(!b)
			continue;
		const bool usePowerAsValue = bonusNode["val"].isNull();
		if(usePowerAsValue)
			b->val = levelObject.power;
		b->sid = BonusSourceID(id);
		b->source = BonusSource::SPELL_EFFECT;

		Bonus nb(*b);
		if(nb.turnsRemain == 0)
			nb.turnsRemain = duration;
		if(maxDuration)
			vstd::amax(*(maxDuration.value()), nb.turnsRemain);

		lst.push_back(nb);
	}
}

int64_t CSpell::adjustRawDamage(const spells::Caster * caster, const battle::Unit * affectedCreature, int64_t rawDamage) const
{
	auto ret = rawDamage;
	//affected creature-specific part
	if(nullptr != affectedCreature)
	{
		const auto * bearer = affectedCreature->getBonusBearer();
		//applying protections - when spell has more then one elements, only one protection should be applied (I think)
		forEachSchool([&](const SpellSchool & cnf, bool & stop)
		{
			if(bearer->hasBonusOfType(BonusType::SPELL_DAMAGE_REDUCTION, BonusSubtypeID(cnf)))
			{
				ret *= 100 - bearer->valOfBonuses(BonusType::SPELL_DAMAGE_REDUCTION, BonusSubtypeID(cnf));
				ret /= 100;
				stop = true; //only bonus from one school is used
			}
		});

		CSelector selector = Selector::typeSubtype(BonusType::SPELL_DAMAGE_REDUCTION, BonusSubtypeID(SpellSchool::ANY));
		auto cachingStr = "type_SPELL_DAMAGE_REDUCTION_s_ANY";

		//general spell dmg reduction, works only on magical effects
		if(bearer->hasBonus(selector, cachingStr) && isMagical())
		{
			ret *= 100 - bearer->valOfBonuses(selector, cachingStr);
			ret /= 100;
		}

		//dmg increasing
		if(bearer->hasBonusOfType(BonusType::MORE_DAMAGE_FROM_SPELL, BonusSubtypeID(id)))
		{
			ret *= 100 + bearer->valOfBonuses(BonusType::MORE_DAMAGE_FROM_SPELL, BonusSubtypeID(id));
			ret /= 100;
		}

		//invincible
		if(affectedCreature->isInvincible())
			ret = 0;
	}
	ret = caster->getSpellBonus(this, ret, affectedCreature);
	return ret;
}

int64_t CSpell::calculateRawEffectValue(int32_t effectLevel, int32_t basePowerMultiplier, int32_t levelPowerMultiplier) const
{
	return static_cast<int64_t>(basePowerMultiplier) * getBasePower() + levelPowerMultiplier * getLevelPower(effectLevel);
}

void CSpell::setIsOffensive(const bool val)
{
	offensive = val;

	if(val)
	{
		positiveness = CSpell::NEGATIVE;
		damage = true;
	}
}

void CSpell::setIsRising(const bool val)
{
	rising = val;

	if(val)
	{
		positiveness = CSpell::POSITIVE;
	}
}

JsonNode CSpell::convertTargetCondition(const BTVector & immunity, const BTVector & absImmunity, const BTVector & limit, const BTVector & absLimit) const
{
	static const std::string CONDITION_NORMAL = "normal";
	static const std::string CONDITION_ABSOLUTE = "absolute";

	JsonNode res;

	auto convertVector = [&](const std::string & targetName, const BTVector & source, const std::string & value)
	{
		for(auto bonusType : source)
		{
			std::string bonusName = LIBRARY->bth->bonusToString(bonusType);
			res[targetName][bonusName].String() = value;
		}
	};

	auto convertSection = [&](const std::string & targetName, const BTVector & normal, const BTVector & absolute)
	{
		convertVector(targetName, normal, CONDITION_NORMAL);
		convertVector(targetName, absolute, CONDITION_ABSOLUTE);
	};

	convertSection("allOf", limit, absLimit);
	convertSection("noneOf", immunity, absImmunity);

	return res;
}

void CSpell::setupMechanics()
{
	mechanics = spells::ISpellMechanicsFactory::get(this);
	adventureMechanics = IAdventureSpellMechanics::createMechanics(this);
}

const IAdventureSpellMechanics & CSpell::getAdventureMechanics() const
{
	return *adventureMechanics;
}

std::unique_ptr<spells::Mechanics> CSpell::battleMechanics(const spells::IBattleCast * event) const
{
	return mechanics->create(event);
}

void CSpell::registerIcons(const IconRegistar & cb) const
{
	cb(getIndex(), 0, "SPELLS", iconBook);
	cb(getIndex()+1, 0, "SPELLINT", iconEffect);
	cb(getIndex(), 0, "SPELLBON", iconScenarioBonus);
	cb(getIndex(), 0, "SPELLSCR", iconScroll);
}

///CSpell::AnimationInfo
AnimationPath CSpell::AnimationInfo::selectProjectile(const double angle) const
{
	AnimationPath res;
	double maximum = 0.0;

	for(const auto & info : projectile)
	{
		if(info.minimumAngle < angle && info.minimumAngle >= maximum)
		{
			maximum = info.minimumAngle;
			res = info.resourceName;
		}
	}
	return res;
}

///CSpell::TargetInfo
CSpell::TargetInfo::TargetInfo(const CSpell * spell, const int level, spells::Mode mode)
	: type(spell->getTargetType()),
	smart(false),
	massive(false),
	clearAffected(false)
{
	const auto & levelInfo = spell->getLevelInfo(level);

	smart = levelInfo.smartTarget;
	massive = levelInfo.range.empty();
	clearAffected = levelInfo.clearAffected;
}

VCMI_LIB_NAMESPACE_END
