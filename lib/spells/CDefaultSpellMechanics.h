/*
 * CDefaultSpellMechanics.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "ISpellMechanics.h"
#include "../NetPacks.h"
#include "../battle/CBattleInfoEssentials.h"

namespace spells
{

///all combat spells
class DLL_LINKAGE DefaultSpellMechanics : public BaseMechanics
{
public:
	DefaultSpellMechanics(const IBattleCast * event);

	std::vector<BattleHex> rangeInHexes(BattleHex centralHex, bool * outDroppedHexes = nullptr) const override;

	void cast(const SpellCastEnvironment * env, const Target & target, std::vector <const CStack*> & reflected) override final;

	bool counteringSelector(const Bonus * bonus) const;

protected:
	std::vector<const battle::Unit *> affectedUnits;

	static void doRemoveEffects(const SpellCastEnvironment * env, const std::vector<const battle::Unit *> & targets, const CSelector & selector);

	virtual void beforeCast(vstd::RNG & rng, const Target & target, std::vector <const CStack*> & reflected) = 0;

	virtual void applyCastEffects(const SpellCastEnvironment * env, const Target & target) const = 0;

	virtual void afterCast() const;

	void addBattleLog(MetaString && line);
	void addCustomEffect(const battle::Unit * target, ui32 effect);

	std::set<BattleHex> spellRangeInHexes(BattleHex centralHex) const;

private:
	BattleSpellCast sc;
	int spellCost;
};

} //namespace spells

