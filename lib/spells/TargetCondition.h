/*
 * TargetCondition.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "ISpellMechanics.h"

VCMI_LIB_NAMESPACE_BEGIN

class JsonNode;
class JsonSerializeFormat;

namespace battle
{
	class Unit;
}

namespace spells
{
class Mechanics;


class DLL_LINKAGE TargetConditionItem : public IReceptiveCheck
{
public:
	virtual void setInverted(bool value) = 0;
	virtual void setExclusive(bool value) = 0;

	virtual bool isExclusive() const = 0;
};

class DLL_LINKAGE TargetConditionItemFactory
{
public:
	using Object = std::shared_ptr<TargetConditionItem>;

	static const TargetConditionItemFactory * getDefault();
	virtual ~TargetConditionItemFactory() = default;

	virtual Object createAbsoluteLevel() const = 0;
	virtual Object createAbsoluteSpell() const = 0;
	virtual Object createElemental() const = 0;
	virtual Object createNormalLevel() const = 0;
	virtual Object createNormalSpell() const = 0;

	virtual Object createConfigurable(std::string scope, std::string type, std::string identifier) const = 0;

	virtual Object createReceptiveFeature() const = 0;
	virtual Object createImmunityNegation() const = 0;
};

class DLL_LINKAGE TargetCondition : public IReceptiveCheck
{
public:
	using Item = TargetConditionItem;
	using ItemVector = std::vector<std::shared_ptr<Item>>;
	using ItemFactory = TargetConditionItemFactory;

	ItemVector normal;
	ItemVector absolute;
	ItemVector negation;

	bool isReceptive(const Mechanics * m, const battle::Unit * target) const override;

	void serializeJson(JsonSerializeFormat & handler, const ItemFactory * itemFactory);
protected:

private:
	bool check(const ItemVector & condition, const Mechanics * m, const battle::Unit * target) const;

	void loadConditions(const JsonNode & source, bool exclusive, bool inverted, const ItemFactory * itemFactory);
};

}

VCMI_LIB_NAMESPACE_END
