/*
 * AdventureSpellMechanics.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "AdventureSpellMechanics.h"

#include "AdventureSpellEffect.h"
#include "DimensionDoorEffect.h"
#include "RemoveObjectEffect.h"
#include "SummonBoatEffect.h"
#include "TownPortalEffect.h"
#include "ViewWorldEffect.h"

#include "../CSpellHandler.h"
#include "../Problem.h"

#include "../../json/JsonBonus.h"
#include "../../mapObjects/CGHeroInstance.h"
#include "../../networkPacks/PacksForClient.h"
#include "../../callback/IGameInfoCallback.h"

VCMI_LIB_NAMESPACE_BEGIN

std::unique_ptr<IAdventureSpellEffect> AdventureSpellMechanics::createAdventureEffect(const CSpell * s, const JsonNode & node)
{
	const std::string & typeID = node["type"].String();

	if(typeID == "generic")
		return std::make_unique<AdventureSpellEffect>();
	if(typeID == "dimensionDoor")
		return std::make_unique<DimensionDoorEffect>(s, node);
	if(typeID == "removeObject")
		return std::make_unique<RemoveObjectEffect>(s, node);
	if(typeID == "summonBoat")
		return std::make_unique<SummonBoatEffect>(s, node);
	if(typeID == "townPortal")
		return std::make_unique<TownPortalEffect>(s, node);
	if(typeID == "viewWorld")
		return std::make_unique<ViewWorldEffect>(s, node);

	return std::make_unique<AdventureSpellEffect>();
}

AdventureSpellMechanics::AdventureSpellMechanics(const CSpell * s)
	: IAdventureSpellMechanics(s)
{
	for(int level = 0; level < GameConstants::SPELL_SCHOOL_LEVELS; level++)
	{
		const JsonNode & config = s->getLevelInfo(level).adventureEffect;

		levelOptions[level].effect = createAdventureEffect(s, config);
		levelOptions[level].castsPerDay = config["castsPerDay"].Integer();
		levelOptions[level].castsPerDayXL = config["castsPerDayXL"].Integer();

		levelOptions[level].bonuses = s->getLevelInfo(level).effects;

		for(const auto & elem : config["bonuses"].Struct())
		{
			auto b = JsonUtils::parseBonus(elem.second);
			b->sid = BonusSourceID(s->id);
			b->source = BonusSource::SPELL_EFFECT;
			levelOptions[level].bonuses.push_back(b);
		}
	}
}

AdventureSpellMechanics::~AdventureSpellMechanics() = default;

const AdventureSpellMechanics::LevelOptions & AdventureSpellMechanics::getLevel(const spells::Caster * caster) const
{
	int schoolLevel = caster->getSpellSchoolLevel(owner);
	return levelOptions.at(schoolLevel);
}

const IAdventureSpellEffect * AdventureSpellMechanics::getEffect(const spells::Caster * caster) const
{
	return getLevel(caster).effect.get();
}

bool AdventureSpellMechanics::givesBonus(const spells::Caster * caster, BonusType which) const
{
	for (const auto & bonus : getLevel(caster).bonuses)
		if (bonus->type == which)
			return true;

	return false;
}

bool AdventureSpellMechanics::canBeCast(spells::Problem & problem, const IGameInfoCallback * cb, const spells::Caster * caster) const
{
	if(!owner->isAdventure())
		return false;

	const auto * heroCaster = dynamic_cast<const CGHeroInstance *>(caster);

	if(heroCaster)
	{
		if(heroCaster->isGarrisoned())
			return false;

		const auto level = heroCaster->getSpellSchoolLevel(owner);
		const auto cost = owner->getCost(level);

		if(!heroCaster->canCastThisSpell(owner))
			return false;

		if(heroCaster->mana < cost)
			return false;

		std::stringstream cachingStr;
		cachingStr << "source_" << vstd::to_underlying(BonusSource::SPELL_EFFECT) << "id_" << owner->id.num;
		int castsAlreadyPerformedThisTurn = caster->getHeroCaster()->getBonuses(Selector::source(BonusSource::SPELL_EFFECT, BonusSourceID(owner->id)), cachingStr.str())->size();
		int3 mapSize = cb->getMapSize();
		bool mapSizeIsAtLeastXL = mapSize.x * mapSize.y * mapSize.z >= GameConstants::TOURNAMENT_RULES_DD_MAP_TILES_THRESHOLD;
		bool useAlternativeLimit = mapSizeIsAtLeastXL && getLevel(caster).castsPerDayXL != 0;
		int castsLimit = useAlternativeLimit ? getLevel(caster).castsPerDayXL : getLevel(caster).castsPerDay;

		if(castsLimit > 0 && castsLimit <= castsAlreadyPerformedThisTurn ) //limit casts per turn
		{
			MetaString message = MetaString::createFromTextID("core.genrltxt.338");
			caster->getCasterName(message);
			problem.add(std::move(message));
			return false;
		}
	}

	return getLevel(caster).effect->canBeCastImpl(problem, cb, caster);
}

bool AdventureSpellMechanics::canBeCastAt(spells::Problem & problem, const IGameInfoCallback * cb, const spells::Caster * caster, const int3 & pos) const
{
	return canBeCast(problem, cb, caster) && getLevel(caster).effect->canBeCastAtImpl(problem, cb, caster, pos);
}

bool AdventureSpellMechanics::adventureCast(SpellCastEnvironment * env, const AdventureSpellCastParameters & parameters) const
{
	spells::detail::ProblemImpl problem;

	if(!canBeCastAt(problem, env->getCb(), parameters.caster, parameters.pos))
		return false;

	ESpellCastResult result = getLevel(parameters.caster).effect->beginCast(env, parameters, *this);

	if(result == ESpellCastResult::OK)
		performCast(env, parameters);

	return result != ESpellCastResult::ERROR;
}

void AdventureSpellMechanics::giveBonuses(SpellCastEnvironment * env, const AdventureSpellCastParameters & parameters) const
{
	for(const auto & b : getLevel(parameters.caster).bonuses)
	{
		GiveBonus gb;
		gb.id = ObjectInstanceID(parameters.caster->getCasterUnitId());
		gb.bonus = *b;
		env->apply(gb);
	}

	GiveBonus gb;
	gb.id = ObjectInstanceID(parameters.caster->getCasterUnitId());
	gb.bonus = Bonus(BonusDuration::ONE_DAY, BonusType::NONE, BonusSource::SPELL_EFFECT, 0, BonusSourceID(owner->id));
	env->apply(gb);
}

void AdventureSpellMechanics::performCast(SpellCastEnvironment * env, const AdventureSpellCastParameters & parameters) const
{
	const auto level = parameters.caster->getSpellSchoolLevel(owner);
	const auto cost = owner->getCost(level);

	AdvmapSpellCast asc;
	asc.casterID = ObjectInstanceID(parameters.caster->getCasterUnitId());
	asc.spellID = owner->id;
	env->apply(asc);

	ESpellCastResult result = getLevel(parameters.caster).effect->applyAdventureEffects(env, parameters);

	if (result == ESpellCastResult::OK)
	{
		giveBonuses(env, parameters);
		parameters.caster->spendMana(env, cost);
		getLevel(parameters.caster).effect->endCast(env, parameters);
	}
}

VCMI_LIB_NAMESPACE_END
