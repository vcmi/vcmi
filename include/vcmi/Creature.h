/*
 * Creature.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "FactionMember.h"

VCMI_LIB_NAMESPACE_BEGIN

class CreatureID;
class ResourceSet;
enum class EGameResID : int8_t;

/// Base class for creatures and battle stacks
class DLL_LINKAGE ACreature: public AFactionMember
{
public:
	bool isLiving() const; //non-undead, non-non living or alive
	ui32 speed(int turn = 0, bool useBind = false) const; //get speed (in moving tiles) of creature with all modificators
	virtual ui32 getMaxHealth() const; //get max HP of stack with all modifiers
};

template <typename IdType>
class DLL_LINKAGE CreatureEntity : public EntityT<IdType>, public ACreature
{
};

class DLL_LINKAGE Creature : public CreatureEntity<CreatureID>
{
protected:
	// use getNamePlural/Singular instead
	std::string getNameTranslated() const override = 0;
	std::string getNameTextID() const override = 0;

public:
	virtual std::string getNamePluralTranslated() const = 0;
	virtual std::string getNameSingularTranslated() const = 0;

	virtual std::string getNamePluralTextID() const = 0;
	virtual std::string getNameSingularTextID() const = 0;

	virtual int32_t getAdvMapAmountMin() const = 0;
	virtual int32_t getAdvMapAmountMax() const = 0;
	virtual int32_t getAIValue() const = 0;
	virtual int32_t getFightValue() const = 0;
	virtual int32_t getLevel() const = 0;
	virtual int32_t getGrowth() const = 0;
	virtual int32_t getHorde() const = 0;

	virtual int32_t getBaseAttack() const = 0;
	virtual int32_t getBaseDefense() const = 0;
	virtual int32_t getBaseDamageMin() const = 0;
	virtual int32_t getBaseDamageMax() const = 0;
	virtual int32_t getBaseHitPoints() const = 0;
	virtual int32_t getBaseSpellPoints() const = 0;
	virtual int32_t getBaseSpeed() const = 0;
	virtual int32_t getBaseShots() const = 0;

	virtual int32_t getRecruitCost(Identifier<EGameResID> resIndex) const = 0;
	virtual ResourceSet getFullRecruitCost() const = 0;
	
	virtual bool hasUpgrades() const = 0;

	virtual bool isDoubleWide() const = 0;
};

VCMI_LIB_NAMESPACE_END
