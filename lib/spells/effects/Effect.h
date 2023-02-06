/*
 * Effect.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include <vcmi/spells/Magic.h>

VCMI_LIB_NAMESPACE_BEGIN

struct BattleHex;
class CBattleInfoCallback;
class JsonSerializeFormat;
class ServerCallback;

namespace vstd
{
	class RNG;
}

namespace spells
{
using EffectTarget = Target;

namespace effects
{
using RNG = vstd::RNG;
class Effects;
class Effect;
class Registry;

template<typename F>
class RegisterEffect;

using TargetType = spells::AimType;

class DLL_LINKAGE Effect
{
public:
	bool indirect = false;
	bool optional = false;

	std::string name;

	virtual ~Effect() = default; //Required for child classes

	// TODO: document me
	virtual void adjustTargetTypes(std::vector<TargetType> & types) const = 0;

	/// Generates list of hexes affected by spell, if spell were to cast at specified target
	virtual void adjustAffectedHexes(std::set<BattleHex> & hexes, const Mechanics * m, const Target & spellTarget) const = 0;

	/// Returns whether effect has any valid targets on the battlefield
	virtual bool applicable(Problem & problem, const Mechanics * m) const;

	/// Returns whether effect is valid and can be applied onto selected target
	virtual bool applicable(Problem & problem, const Mechanics * m, const EffectTarget & target) const;

	virtual void apply(ServerCallback * server, const Mechanics * m, const EffectTarget & target) const = 0;

	/// Processes input target and generates subset-result that contains only valid targets
	virtual EffectTarget filterTarget(const Mechanics * m, const EffectTarget & target) const = 0;

	// TODO: document me
	virtual EffectTarget transformTarget(const Mechanics * m, const Target & aimPoint, const Target & spellTarget) const = 0;

	/// Serializes (or deserializes) parameters of Effect
	void serializeJson(JsonSerializeFormat & handler);

	static std::shared_ptr<Effect> create(const Registry * registry, const std::string & type);

protected:
	virtual void serializeJsonEffect(JsonSerializeFormat & handler) = 0;
};

}
}

VCMI_LIB_NAMESPACE_END
