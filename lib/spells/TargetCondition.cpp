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

#include "../serializer/JsonSerializeFormat.h"
#include "../VCMI_Lib.h"
#include "../CModHandler.h"

VCMI_LIB_NAMESPACE_BEGIN


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

class SelectorCondition : public TargetConditionItemBase
{
public:
	SelectorCondition(const CSelector & csel):
		sel(csel)
	{
	}
	SelectorCondition(const CSelector & csel, si32 minVal, si32 maxVal):
		sel(csel),
		minVal(minVal),
		maxVal(maxVal)
	{
	}

protected:
	bool check(const Mechanics * m, const battle::Unit * target) const override
	{
		if(target->hasBonus(sel)) {
			auto b = target->valOfBonuses(sel,"");
			return b >= minVal && b <= maxVal;
		}
		return false;
	}

private:
	CSelector sel;
	si32 minVal = std::numeric_limits<si32>::min();
	si32 maxVal = std::numeric_limits<si32>::max();
};

class CreatureCondition : public TargetConditionItemBase
{
public:
	CreatureCondition(const CreatureID & type_): type(type_) {}

protected:
	bool check(const Mechanics * m, const battle::Unit * target) const override
	{
		return target->creatureId() == type;
	}

private:
	CreatureID type;
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
		std::stringstream cachingStr;
		cachingStr << "type_" << Bonus::LEVEL_SPELL_IMMUNITY << "addInfo_1";

		TConstBonusListPtr levelImmunities = target->getBonuses(Selector::type()(Bonus::LEVEL_SPELL_IMMUNITY).And(Selector::info()(1)), cachingStr.str());
		return (levelImmunities->size() == 0 || levelImmunities->totalValue() < m->getSpellLevel() || m->getSpellLevel() <= 0);
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
		std::stringstream cachingStr;
		cachingStr << "type_" << Bonus::SPELL_IMMUNITY << "subtype_" << m->getSpellIndex() << "addInfo_1";
		return !target->hasBonus(Selector::typeSubtypeInfo(Bonus::SPELL_IMMUNITY, m->getSpellIndex(), 1), cachingStr.str());
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

		auto filter = m->getElementalImmunity();

		for(auto element : filter)
		{
			if(target->hasBonusOfType(element, 0)) //always resist if immune to all spells altogether
			{
				elementalImmune = true;
				break;
			}
			else if(!m->isPositiveSpell()) //negative or indifferent
			{
				if(target->hasBonusOfType(element, 1))
				{
					elementalImmune = true;
					break;
				}
			}
		}
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
		TConstBonusListPtr levelImmunities = target->getBonuses(Selector::type()(Bonus::LEVEL_SPELL_IMMUNITY));
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
		return !target->hasBonusOfType(Bonus::SPELL_IMMUNITY, m->getSpellIndex());
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
		builder << "source_" << Bonus::SPELL_EFFECT << "id_" << spellID.num;
		cachingString = builder.str();

		selector = Selector::source(Bonus::SPELL_EFFECT, spellID.num);
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
	CSelector selector = Selector::type()(Bonus::RECEPTIVE);
	std::string cachingString = "type_RECEPTIVE";
};

class ImmunityNegationCondition : public TargetConditionItemBase
{
protected:
	bool check(const Mechanics * m, const battle::Unit * target) const override
	{
		const bool battleWideNegation = target->hasBonusOfType(Bonus::NEGATE_ALL_NATURAL_IMMUNITIES, 0);
		const bool heroNegation = target->hasBonusOfType(Bonus::NEGATE_ALL_NATURAL_IMMUNITIES, 1);
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

class DefaultTargetConditionItemFactory : public TargetConditionItemFactory
{
public:
	Object createAbsoluteLevel() const override
	{
		static std::shared_ptr<TargetConditionItem> antimagicCondition = std::make_shared<AbsoluteLevelCondition>();
        return antimagicCondition;
	}

	Object createAbsoluteSpell() const override
	{
		static std::shared_ptr<TargetConditionItem> alCondition = std::make_shared<AbsoluteSpellCondition>();
		return alCondition;
	}

	Object createElemental() const override
	{
		static std::shared_ptr<TargetConditionItem> elementalCondition = std::make_shared<ElementalCondition>();
		return elementalCondition;
	}

	Object createNormalLevel() const override
	{
		static std::shared_ptr<TargetConditionItem> nlCondition = std::make_shared<NormalLevelCondition>();
		return nlCondition;
	}

	Object createNormalSpell() const override
	{
		static std::shared_ptr<TargetConditionItem> nsCondition = std::make_shared<NormalSpellCondition>();
		return nsCondition;
	}

	Object createConfigurable(std::string scope, std::string type, std::string identifier) const override
	{
		if(type == "bonus")
		{
			//TODO: support custom bonus types

			auto it = bonusNameMap.find(identifier);
			if(it != bonusNameMap.end())
				return std::make_shared<SelectorCondition>(Selector::type()(it->second));

			auto params = BonusParams(identifier, "", -1);
			if(params.isConverted)
			{
				if(params.valRelevant)
					return std::make_shared<SelectorCondition>(params.toSelector(), params.val, params.val);
				return std::make_shared<SelectorCondition>(params.toSelector());
			}

				logMod->error("Invalid bonus type %s in spell target condition.", identifier);
		}
		else if(type == "creature")
		{
			auto rawId = VLC->modh->identifiers.getIdentifier(scope, type, identifier, true);

			if(rawId)
				return std::make_shared<CreatureCondition>(CreatureID(rawId.get()));
			else
				logMod->error("Invalid creature %s type in spell target condition.", identifier);
		}
		else if(type == "spell")
		{
			auto rawId = VLC->modh->identifiers.getIdentifier(scope, type, identifier, true);

			if(rawId)
				return std::make_shared<SpellEffectCondition>(SpellID(rawId.get()));
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

	Object createFromJsonStruct(const JsonNode & jsonStruct) const override
	{
		auto type = jsonStruct["type"].String();
		auto parameters = jsonStruct["parameters"];
		if(type == "selector")
		{
			auto minVal = std::numeric_limits<si32>::min();
			auto maxVal = std::numeric_limits<si32>::max();
			if(parameters["minVal"].isNumber())
				minVal = parameters["minVal"].Integer();
			if(parameters["maxVal"].isNumber())
				maxVal = parameters["maxVal"].Integer();
			auto sel = JsonUtils::parseSelector(parameters);
			return std::make_shared<SelectorCondition>(sel, minVal, maxVal);
		}

		logMod->error("Invalid type %s in spell target condition.", type);
		return Object();
	}

	Object createReceptiveFeature() const override
	{
		static std::shared_ptr<TargetConditionItem> condition = std::make_shared<ReceptiveFeatureCondition>();
		return condition;
	}

	Object createImmunityNegation() const override
	{
		static std::shared_ptr<TargetConditionItem> condition = std::make_shared<ImmunityNegationCondition>();
		return condition;
	}
};

const TargetConditionItemFactory * TargetConditionItemFactory::getDefault()
{
	static std::unique_ptr<TargetConditionItemFactory> singleton;

	if(!singleton)
		singleton = std::make_unique<DefaultTargetConditionItemFactory>();
	return singleton.get();
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
	bool isNonMagical = false;
	if(handler.saving)
	{
		logGlobal->error("Spell target condition saving is not supported");
		return;
	}

	absolute.clear();
	normal.clear();
	negation.clear();

	absolute.push_back(itemFactory->createAbsoluteSpell());

	handler.serializeBool("nonMagical", isNonMagical);
	if(!isNonMagical)
	{
		absolute.push_back(itemFactory->createAbsoluteLevel());
		normal.push_back(itemFactory->createElemental());
		normal.push_back(itemFactory->createNormalLevel());
		normal.push_back(itemFactory->createNormalSpell());
		negation.push_back(itemFactory->createReceptiveFeature());
		negation.push_back(itemFactory->createImmunityNegation());
	}

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

		if(value.String() == "absolute")
			isAbsolute = true;
		else if(value.String() == "normal")
			isAbsolute = false;
		else if(value.isStruct()) //assume conditions have a new struct format
			isAbsolute = value["absolute"].Bool();
		else
			continue;

		std::shared_ptr<TargetConditionItem> item;
		if(value.isStruct())
			item = itemFactory->createFromJsonStruct(value);
		else
		{
			std::string scope;
			std::string type;
			std::string identifier;

			CModHandler::parseIdentifier(keyValue.first, scope, type, identifier);

			item = itemFactory->createConfigurable(scope, type, identifier);
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

VCMI_LIB_NAMESPACE_END
