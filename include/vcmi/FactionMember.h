/*
 * FactionMember.h, part of VCMI engine
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

class BonusList;

namespace PrimarySkill
{
    enum PrimarySkill : int8_t;
}

class DLL_LINKAGE AFactionMember: public IConstBonusProvider, public INativeTerrainProvider
{
public:
	/**
	 Returns native terrain considering some terrain bonuses.
	*/
	virtual Identifier<ETerrainId> getNativeTerrain() const;
	/**
	 Returns magic resistance considering some bonuses.
	*/
	virtual int32_t magicResistance() const;
	/**
	 Returns minimal damage of creature or (when implemented) hero.
	*/
	virtual int getMinDamage(bool ranged) const;
	/**
	 Returns maximal damage of creature or (when implemented) hero.
	*/
	virtual int getMaxDamage(bool ranged) const;
	/**
	 Returns attack of creature or hero.
	*/
	virtual int getAttack(bool ranged) const;
	/**
	 Returns defence of creature or hero.
	*/
	virtual int getDefense(bool ranged) const;
	/**
	 Returns primskill of creature or hero.
	*/
	int getPrimSkillLevel(PrimarySkill::PrimarySkill id) const;
	/**
	 Returns morale of creature or hero. Taking absolute bonuses into account.
	 For now, uses range from EGameSettings
	*/
	int moraleVal() const;
	/**
	 Returns luck of creature or hero. Taking absolute bonuses into account.
	 For now, uses range from EGameSettings
	*/
	int luckVal() const;
	/**
	 Returns total value of all morale bonuses and sets bonusList as a pointer to the list of selected bonuses.
	 @param bonusList is the out param it's list of all selected bonuses
	 @return total value of all morale in the range from EGameSettings and 0 otherwise
	*/
	int moraleValAndBonusList(std::shared_ptr<const BonusList> & bonusList) const;
	int luckValAndBonusList(std::shared_ptr<const BonusList> & bonusList) const;
};

VCMI_LIB_NAMESPACE_END