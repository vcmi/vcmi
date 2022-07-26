/*
 * Effects.h, part of VCMI engine
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

class DLL_LINKAGE Effects
{
public:
	using EffectsToApply = std::vector<std::pair<const Effect *, EffectTarget>>;

	using EffectsMap = std::map<std::string, std::shared_ptr<Effect>>;
	using EffectData = std::array<EffectsMap, GameConstants::SPELL_SCHOOL_LEVELS>;

	EffectData data;

	Effects();
	virtual ~Effects();

	void add(const std::string & name, std::shared_ptr<Effect> effect, const int level);

	bool applicable(Problem & problem, const Mechanics * m) const;
	bool applicable(Problem & problem, const Mechanics * m, const Target & aimPoint, const Target & spellTarget) const;

	void forEachEffect(const int level, const std::function<void(const Effect *, bool &)> & callback) const;

	EffectsToApply prepare(const Mechanics * m, const Target & aimPoint, const Target & spellTarget) const;

	void serializeJson(const Registry * registry, JsonSerializeFormat & handler, const int level);
};


}
}

VCMI_LIB_NAMESPACE_END
