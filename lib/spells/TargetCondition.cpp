/*
 * TargetCondition.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "TargetCondition.h"

#include "../GameConstants.h"
#include "../CBonusTypeHandler.h"
#include "../battle/CBattleInfoCallback.h"
#include "../battle/Unit.h"
#include "../CCreatureHandler.h"
#include "../bonuses/BonusList.h"
#include "../bonuses/BonusMigration.h"
#include "../bonuses/BonusParameters.h"
#include "../bonuses/BonusSelector.h"
#include "../bonuses/Limiters.h"
#include "../json/JsonBonus.h"
#include "../modding/IdentifierResolver.h"
#include "../modding/IdentifierStorage.h"
#include "../modding/ModUtility.h"
#include "../serializer/JsonSerializeFormat.h"

#include <vcmi/spells/Spell.h>


namespace spells
{

class TargetConditionItemBase : public TargetConditionItem
{
public:
	bool inverted = false;
	bool exclusive = false;

	void setInverted(bool value) override
	{
		inverted = value;
	}

	void setExclusive(bool value) override
	{
		exclusive = value;
	}

	bool isExclusive() const override
	{
		return exclusive;
	}

	bool isReceptive(const Mechanics * m, const battle::Unit * target) const override
	{
		bool result = check(m, target);
		return inverted != result;
	}

protected:
	virtual bool check(const Mechanics * m, const battle::Unit * target) const = 0;
};

/// Same check as HAS_ANOTHER_BONUS_LIMITER, applied to the target unit
class BonusCondition : public TargetConditionItemBase
{
public:
	explicit BonusCondition(std::shared_ptr<const HasAnotherBonusLimiter> limiter_):
		limiter(std::move(limiter_)),
		selector(limiter->getSelector())
	{
	}

protected:
	bool check(const Mechanics * m, const battle::Unit * target) const override
	{
		if(!limiter->hasValueRange())
			return target->hasBonus(selector);

		auto bonuses = target->getBonuses(selector);
		if(bonuses->empty())
			return false;

		int value = bonuses->totalValue();
		return value >= limiter->minValue && value <= limiter->maxValue;
	}

private:
	std::shared_ptr<const HasAnotherBonusLimiter> limiter;
	CSelector selector;
};

class ResistanceCondition : public TargetConditionItemBase
{
public:
	ResistanceCondition()
	{
		exclusive = true;
	}

protected:
	bool check(const Mechanics * m, const battle::Unit * target) const override
	{
		if(m->isPositiveSpell()) //Always pass on positive
			return true;

		return target->magicResistance() < 100;
	}
};

class CreatureCondition : public TargetConditionItemBase
{
public:
	CreatureCondition(const CreatureID & type_, bool includeUpgrades_ = false): type(type_), includeUpgrades(includeUpgrades_) {}

protected:
	bool check(const Mechanics * m, const battle::Unit * target) const override
	{
		return target->creatureId() == type || (includeUpgrades && target->unitType()->isUpgradeOf(type));
	}

private:
	CreatureID type;
	bool includeUpgrades;
};

class FactionCondition : public TargetConditionItemBase
{
public:
	FactionCondition(const FactionID & faction_): faction(faction_) {}

protected:
	bool check(const Mechanics * m, const battle::Unit * target) const override
	{
		return target->getFactionID() == faction;
	}

private:
	FactionID faction;
};

class AbsoluteLevelCondition : public TargetConditionItemBase
{
public:
	AbsoluteLevelCondition()
	{
		inverted = false;
		exclusive = true;
	}

protected:
	bool check(const Mechanics * m, const battle::Unit * target) const override
	{

		if(!m->isMagicalEffect()) //Always pass on non-magical
			return true;

		if (m->getSpellLevel() <= 0)
			return true;

		bool hasAbsoluteImmunity = false;

		const auto & levelImmunities = target->getBonusesOfType(BonusType::LEVEL_SPELL_IMMUNITY);
		for (const auto & bonus : *levelImmunities)
			if (bonus->parameters && bonus->parameters->toNumber() == 1)
				hasAbsoluteImmunity = true;

		if (!hasAbsoluteImmunity)
			return true;

		return levelImmunities->totalValue() < m->getSpellLevel();

		return true;
	}
};

class AbsoluteSpellCondition : public TargetConditionItemBase
{
public:
	AbsoluteSpellCondition()
	{
		inverted = false;
		exclusive = true;
	}

protected:
	bool check(const Mechanics * m, const battle::Unit * target) const override
	{
		return !target->hasAbsoluteImmunity(m->getSpellId());
	}
};


class ElementalCondition : public TargetConditionItemBase
{
public:
	ElementalCondition()
	{
		inverted = true;
		exclusive = true;
	}

protected:
	bool check(const Mechanics * m, const battle::Unit * target) const override
	{
		bool elementalImmune = false;
		auto bearer = target->getBonusBearer();

		if(!m->isPositiveSpell() && bearer->hasBonusOfType(BonusType::NEGATIVE_EFFECTS_IMMUNITY, BonusSubtypeID(SpellSchool::ANY)))
			return true;

		m->getSpell()->forEachSchool([&](const SpellSchool & cnf, bool & stop)
		{
			if (bearer->hasBonusOfType(BonusType::SPELL_SCHOOL_IMMUNITY, BonusSubtypeID(cnf)))
			{
				elementalImmune = true;
				stop = true; //only bonus from one school is used
			}
			else if(!m->isPositiveSpell()) //negative or indifferent
			{
				if (bearer->hasBonusOfType(BonusType::NEGATIVE_EFFECTS_IMMUNITY, BonusSubtypeID(cnf)))
				{
					elementalImmune = true;
					stop = true; //only bonus from one school is used
				}
			}
		});

		return elementalImmune;
	}
};

class NormalLevelCondition : public TargetConditionItemBase
{
public:
	NormalLevelCondition()
	{
		inverted = false;
		exclusive = true;
	}

protected:
	bool check(const Mechanics * m, const battle::Unit * target) const override
	{
		if(!m->isMagicalEffect()) //Always pass on non-magical
			return true;
		TConstBonusListPtr levelImmunities = target->getBonusesOfType(BonusType::LEVEL_SPELL_IMMUNITY);
		return levelImmunities->size() == 0 ||
		levelImmunities->totalValue() < m->getSpellLevel() ||
		m->getSpellLevel() <= 0;
	}
};

class NormalSpellCondition : public TargetConditionItemBase
{
public:
	NormalSpellCondition()
	{
		inverted = false;
		exclusive = true;
	}

protected:
	bool check(const Mechanics * m, const battle::Unit * target) const override
	{
		return !target->hasImmunity(m->getSpellId());
	}
};

//for Hypnotize
class HealthValueCondition : public TargetConditionItemBase
{
protected:
	bool check(const Mechanics * m, const battle::Unit * target) const override
	{
		//todo: maybe do not resist on passive cast
		//TODO: what with other creatures casting hypnotize, Faerie Dragons style?
		int64_t subjectHealth = target->getAvailableHealth();
		//apply 'damage' bonus for hypnotize, including hero specialty
		auto maxHealth = m->applySpellBonus(m->getEffectValue(), target);
		return subjectHealth <= maxHealth;
	}
};

class SpellEffectCondition : public TargetConditionItemBase
{
public:
	SpellEffectCondition(const SpellID & spellID_): spellID(spellID_)
	{
		std::stringstream builder;
		builder << "source_" << vstd::to_underlying(BonusSource::SPELL_EFFECT) << "id_" << spellID.num;
		cachingString = builder.str();

		selector = Selector::source(BonusSource::SPELL_EFFECT, BonusSourceID(spellID));
	}

protected:
	bool check(const Mechanics * m, const battle::Unit * target) const override
	{
		return target->hasBonus(selector, cachingString);
	}

private:
	CSelector selector;
	std::string cachingString;
	SpellID spellID;
};

class ReceptiveFeatureCondition : public TargetConditionItemBase
{
protected:
	bool check(const Mechanics * m, const battle::Unit * target) const override
	{
		return m->isPositiveSpell() && target->hasBonus(selector, cachingString);
	}

private:
	CSelector selector = Selector::type()(BonusType::RECEPTIVE);
	std::string cachingString = "type_RECEPTIVE";
};

class ImmunityNegationCondition : public TargetConditionItemBase
{
protected:
	bool check(const Mechanics * m, const battle::Unit * target) const override
	{
		const bool battleWideNegation = target->hasBonusOfType(BonusType::NEGATE_ALL_NATURAL_IMMUNITIES, BonusCustomSubtype::immunityBattleWide);
		const bool heroNegation = target->hasBonusOfType(BonusType::NEGATE_ALL_NATURAL_IMMUNITIES, BonusCustomSubtype::immunityEnemyHero);
		//Non-magical effects is not affected by orb of vulnerability
		if(!m->isMagicalEffect())
			return false;

		//anyone can cast on artifact holder`s stacks
		if(heroNegation)
		{
			return true;
		}
		//this stack is from other player
		else if(battleWideNegation)
		{
			if(m->ownerMatches(target, false))
				return true;
		}
		return false;
	}
};

/// Retired bonus types never appear on units, so a condition on them would silently never match
static bool warnIfRetiredBonus(const std::string & bonusType)
{
	std::string_view script = BonusMigration::retiredAbilityScript(bonusType);

	if(script.empty())
		return false;

	logMod->warn("Bonus %s in spell target condition was retired. Use condition { \"type\" : \"bonus\", \"bonusType\" : \"COMBAT_EVENT_TRIGGER\", \"bonusSubtype\" : \"%s\" } instead", bonusType, script);
	return true;
}

class DefaultTargetConditionItemFactory : public TargetConditionItemFactory
{
	const CIdentifierStorage & identifiers;

	/// Parses condition as an equivalent limiter, so fields are named and resolved exactly like in that limiter
	template<typename LimiterType>
	std::shared_ptr<const LimiterType> parseAsLimiter(const JsonNode & config, const std::string & limiterType) const
	{
		InstantIdentifierResolver resolver(identifiers);
		JsonNode limiterConfig = config;
		limiterConfig["type"].String() = limiterType;
		return std::dynamic_pointer_cast<const LimiterType>(JsonUtils::parseLimiter(limiterConfig, resolver));
	}

public:
	explicit DefaultTargetConditionItemFactory(const CIdentifierStorage & identifiers)
		: identifiers(identifiers)
	{
	}

	Object createAbsoluteLevel() const override
	{
		static const auto antimagicCondition = std::make_shared<AbsoluteLevelCondition>();
        return antimagicCondition;
	}

	Object createAbsoluteSpell() const override
	{
		static const auto alCondition = std::make_shared<AbsoluteSpellCondition>();
		return alCondition;
	}

	Object createElemental() const override
	{
		static const auto elementalCondition = std::make_shared<ElementalCondition>();
		return elementalCondition;
	}

	Object createResistance() const override
	{
		static const auto elementalCondition = std::make_shared<ResistanceCondition>();
		return elementalCondition;
	}

	Object createNormalLevel() const override
	{
		static const auto nlCondition = std::make_shared<NormalLevelCondition>();
		return nlCondition;
	}

	Object createNormalSpell() const override
	{
		static const auto nsCondition = std::make_shared<NormalSpellCondition>();
		return nsCondition;
	}

	Object createConfigurable(std::string scope, std::string type, std::string identifier) const override
	{
		if(type == "bonus")
		{
			std::optional bonusID(identifiers.getIdentifier(scope, "bonus", identifier, true));
			if (bonusID)
				return std::make_shared<BonusCondition>(std::make_shared<HasAnotherBonusLimiter>(static_cast<BonusType>(*bonusID)));
			else if (!warnIfRetiredBonus(identifier))
				logMod->error("Invalid bonus %s type in spell target condition.", identifier);
		}
		else if(type == "creature")
		{
			auto rawId = identifiers.getIdentifier(scope, type, identifier, true);

			if(rawId)
				return std::make_shared<CreatureCondition>(CreatureID(rawId.value()));
			else
				logMod->error("Invalid creature %s type in spell target condition.", identifier);
		}
		else if(type == "spell")
		{
			auto rawId = identifiers.getIdentifier(scope, type, identifier, true);

			if(rawId)
				return std::make_shared<SpellEffectCondition>(SpellID(rawId.value()));
			else
				logMod->error("Invalid spell %s in spell target condition.", identifier);
		}
		else if(type == "healthValueSpecial")
		{
			return  std::make_shared<HealthValueCondition>();
		}
		else
		{
			logMod->error("Invalid type %s in spell target condition.", type);
		}

		return Object();
	}

	Object createFromJson(const JsonNode & config) const override
	{
		const std::string & type = config["type"].String();

		if(type == "bonus")
		{
			if(warnIfRetiredBonus(config["bonusType"].String()))
				return Object();

			auto limiter = parseAsLimiter<HasAnotherBonusLimiter>(config, "HAS_ANOTHER_BONUS_LIMITER");
			if(limiter && limiter->type != BonusType::NONE)
				return std::make_shared<BonusCondition>(limiter);
		}
		else if(type == "creature")
		{
			auto limiter = parseAsLimiter<CCreatureTypeLimiter>(config, "CREATURE_TYPE_LIMITER");
			if(limiter && limiter->creatureID.hasValue())
				return std::make_shared<CreatureCondition>(limiter->creatureID, limiter->includeUpgrades);
		}
		else if(type == "faction")
		{
			auto limiter = parseAsLimiter<FactionLimiter>(config, "FACTION_LIMITER");
			if(limiter && limiter->faction != FactionID::DEFAULT)
				return std::make_shared<FactionCondition>(limiter->faction);
		}
		else if(type == "spell")
		{
			auto rawId = identifiers.getIdentifier("spell", config["spell"], true);
			if(rawId)
				return std::make_shared<SpellEffectCondition>(SpellID(rawId.value()));
		}
		else if(type == "healthValueSpecial")
		{
			return std::make_shared<HealthValueCondition>();
		}

		logMod->error("Invalid spell target condition: %s", config.toCompactString());
		return Object();
	}

	Object createReceptiveFeature() const override
	{
		static const auto condition = std::make_shared<ReceptiveFeatureCondition>();
		return condition;
	}

	Object createImmunityNegation() const override
	{
		static const auto condition = std::make_shared<ImmunityNegationCondition>();
		return condition;
	}
};

std::unique_ptr<TargetConditionItemFactory> TargetConditionItemFactory::create(const CIdentifierStorage & identifiers)
{
	return std::make_unique<DefaultTargetConditionItemFactory>(identifiers);
}

bool TargetCondition::isReceptive(const Mechanics * m, const battle::Unit * target) const
{
	if(!check(absolute, m, target))
		return false;

	for(const auto & item : negation)
	{
		if(item->isReceptive(m, target))
			return true;
	}
	return check(normal, m, target);
}

void TargetCondition::serializeJson(JsonSerializeFormat & handler, const ItemFactory * itemFactory)
{
	if(handler.saving)
	{
		logGlobal->error("Spell target condition saving is not supported");
		return;
	}

	absolute.clear();
	normal.clear();
	negation.clear();

	absolute.push_back(itemFactory->createAbsoluteSpell());
	absolute.push_back(itemFactory->createAbsoluteLevel());
	normal.push_back(itemFactory->createElemental());
	normal.push_back(itemFactory->createResistance());
	normal.push_back(itemFactory->createNormalLevel());
	normal.push_back(itemFactory->createNormalSpell());
	negation.push_back(itemFactory->createReceptiveFeature());
	negation.push_back(itemFactory->createImmunityNegation());

	{
		auto anyOf = handler.enterStruct("anyOf");
		loadConditions(anyOf->getCurrent(), false, false, itemFactory);
	}

	{
		auto allOf = handler.enterStruct("allOf");
		loadConditions(allOf->getCurrent(), true, false, itemFactory);
	}

	{
		auto noneOf = handler.enterStruct("noneOf");
		loadConditions(noneOf->getCurrent(), true, true, itemFactory);
	}
}

bool TargetCondition::check(const ItemVector & condition, const Mechanics * m, const battle::Unit * target) const
{
	bool nonExclusiveCheck = false;
	bool nonExclusiveExits = false;

	for(const auto & item : condition)
	{
		if(item->isExclusive())
		{
			if(!item->isReceptive(m, target))
				return false;
		}
		else
		{
			if(item->isReceptive(m, target))
				nonExclusiveCheck = true;
			nonExclusiveExits = true;
		}
	}
	return !nonExclusiveExits || nonExclusiveCheck;
}

void TargetCondition::loadConditions(const JsonNode & source, bool exclusive, bool inverted, const ItemFactory * itemFactory)
{
	for(const auto & keyValue : source.Struct())
	{
		bool isAbsolute;

		const JsonNode & value = keyValue.second;

		if (!value.isString() && !value.isStruct())
			continue;

		const JsonNode & mode = value.isStruct() ? value["mode"] : value;

		if(mode.isString() && mode.String() == "absolute")
			isAbsolute = true;
		else if(mode.isString() && mode.String() == "normal")
			isAbsolute = false;
		else
		{
			if(value.isStruct())
				logMod->error("Spell target condition %s has invalid mode: %s", keyValue.first, value.toCompactString());
			continue;
		}

		std::shared_ptr<TargetConditionItem> item;

		if(value.isStruct())
		{
			item = itemFactory->createFromJson(value);
		}
		else
		{
			std::string scope;
			std::string type;
			std::string identifier;

			ModUtility::parseIdentifier(keyValue.first, scope, type, identifier);

			item = itemFactory->createConfigurable(value.getModScope(), type, identifier);
		}

		if(item)
		{
			item->setExclusive(exclusive);
			item->setInverted(inverted);

			if(isAbsolute)
				absolute.push_back(item);
			else
				normal.push_back(item);
		}
	}
}

}
