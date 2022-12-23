/*
 * Summon.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "Effect.h"
#include "../../GameConstants.h"

VCMI_LIB_NAMESPACE_BEGIN

namespace spells
{
namespace effects
{

class Summon : public Effect
{
public:
	Summon();
	virtual ~Summon();

	void adjustAffectedHexes(std::set<BattleHex> & hexes, const Mechanics * m, const Target & spellTarget) const override;
	void adjustTargetTypes(std::vector<TargetType> & types) const override;

	bool applicable(Problem & problem, const Mechanics * m) const override;

	void apply(ServerCallback * server, const Mechanics * m, const EffectTarget & target) const override;

	EffectTarget filterTarget(const Mechanics * m, const EffectTarget & target) const override;

	EffectTarget transformTarget(const Mechanics * m, const Target & aimPoint, const Target & spellTarget) const override;

protected:
	void serializeJsonEffect(JsonSerializeFormat & handler) override final;

private:
	CreatureID creature;

	bool permanent;
	bool exclusive;
	bool summonByHealth;
	bool summonSameUnit;
};

}
}

VCMI_LIB_NAMESPACE_END
