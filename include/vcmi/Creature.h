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

#include "Entity.h"

VCMI_LIB_NAMESPACE_BEGIN

class CreatureID;

class DLL_LINKAGE Creature : public EntityWithBonuses<CreatureID>
{
protected:
	using EntityWithBonuses<CreatureID>::getName;

	virtual std::string getNameTranslated() const = 0;
	virtual std::string getNameTextID() const = 0;
public:
	virtual std::string getNamePluralTranslated() const = 0;
	virtual std::string getNameSingularTranslated() const = 0;

	virtual std::string getNamePluralTextID() const = 0;
	virtual std::string getNameSingularTextID() const = 0;

	virtual uint32_t getMaxHealth() const = 0;

	virtual int32_t getAdvMapAmountMin() const = 0;
	virtual int32_t getAdvMapAmountMax() const = 0;
	virtual int32_t getAIValue() const = 0;
	virtual int32_t getFightValue() const = 0;
	virtual int32_t getLevel() const = 0;
	virtual int32_t getGrowth() const = 0;
	virtual int32_t getHorde() const = 0;
	virtual int32_t getFactionIndex() const = 0;

	virtual int32_t getBaseAttack() const = 0;
	virtual int32_t getBaseDefense() const = 0;
	virtual int32_t getBaseDamageMin() const = 0;
	virtual int32_t getBaseDamageMax() const = 0;
	virtual int32_t getBaseHitPoints() const = 0;
	virtual int32_t getBaseSpellPoints() const = 0;
	virtual int32_t getBaseSpeed() const = 0;
	virtual int32_t getBaseShots() const = 0;

	virtual int32_t getCost(int32_t resIndex) const = 0;

	virtual bool isDoubleWide() const = 0;
};

VCMI_LIB_NAMESPACE_END
