/*
 * ObstacleCasterProxy.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "ProxyCaster.h"
#include "../battle/BattleHex.h"
#include "../battle/CObstacleInstance.h"

VCMI_LIB_NAMESPACE_BEGIN
namespace spells
{

class DLL_LINKAGE SilentCaster : public ProxyCaster
{
protected:
	const PlayerColor owner;
public:
	SilentCaster(PlayerColor owner_, const Caster * caster);

	void getCasterName(MetaString & text) const override;
	void getCastDescription(const Spell * spell, const battle::Units & attacked, MetaString & text) const override;
	void spendMana(ServerCallback * server, const int spellCost) const override;
	PlayerColor getCasterOwner() const override;
	int32_t manaLimit() const override;
};

class DLL_LINKAGE ObstacleCasterProxy : public SilentCaster
{
public:
	ObstacleCasterProxy(PlayerColor owner_, const Caster * hero_, const SpellCreatedObstacle & obs_);

	int32_t getSpellSchoolLevel(const Spell * spell, SpellSchool * outSelectedSchool = nullptr) const override;
	int32_t getEffectLevel(const Spell * spell) const override;
	int64_t getSpellBonus(const Spell * spell, int64_t base, const battle::Unit * affectedStack) const override;
	int32_t getEffectPower(const Spell * spell) const override;
	int32_t getEnchantPower(const Spell * spell) const override;
	int64_t getEffectValue(const Spell * spell) const override;
	int64_t getEffectRange(const Spell * spell) const override;

private:
	const SpellCreatedObstacle & obs;
};

}//
VCMI_LIB_NAMESPACE_END
