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

#include "../../constants/EntityIdentifiers.h"

VCMI_LIB_NAMESPACE_BEGIN

class BattleHex;
class BattleHexArray;
class CBattleInfoCallback;
class JsonSerializeFormat;
class ServerCallback;

namespace vstd
{
	class RNG;
}

namespace spells
{

namespace effects
{
using RNG = vstd::RNG;
class Effects;
class Effect;
class SpellEffectService;

using TargetType = spells::AimType;

struct SpellEffectValue : public scripting::ApiSerializable<SpellEffectValue>
{
	int64_t hpDelta = 0; // positive -> healed health points, negative -> damage
	int64_t unitsDelta = 0; // positive -> resurrected / summoned (demons) / animated (undeads), negative -> kills
	CreatureID unitType = CreatureID::NONE; // type of creatures summoned / resurrected / animated / etc.

	SpellEffectValue & operator+=(const SpellEffectValue & rhs) noexcept
	{
		hpDelta += rhs.hpDelta;
		unitsDelta += rhs.unitsDelta;
		if(unitType == CreatureID::NONE)
			unitType = rhs.unitType;

		return *this;
	}

	template<typename Serializer>
	void serializeScript(Serializer & s)
	{
		s("hpDelta", hpDelta);
		s("unitsDelta", unitsDelta);
		s("unitType", unitType);
	}
};

class DLL_LINKAGE Effect
{
public:
	bool indirect = false;
	bool optional = false;

	std::string name;
	std::string spellScope;
	std::string spellIdentifier;

	virtual ~Effect() = default; //Required for child classes

	// TODO: document me
	virtual void adjustTargetTypes(std::vector<TargetType> & types, const Mechanics * m) const {};

	/// Generates list of hexes affected by spell, if spell were to cast at specified target
	virtual void adjustAffectedHexes(BattleHexArray & hexes, const Mechanics * m, const Target & spellTarget) const = 0;

	/// Returns whether effect has any valid targets on the battlefield
	virtual bool applicableGeneral(Problem & problem, const Mechanics * m) const;

	/// Returns whether effect is valid and can be applied onto selected target
	virtual bool applicableTarget(Problem & problem, const Mechanics * m, const Target & target) const;

	virtual void apply(ServerCallback * server, const Mechanics * m, const Target & target) const = 0;

	/// Processes input target and generates subset-result that contains only valid targets
	virtual Target filterTarget(const Mechanics * m, const Target & target) const = 0;

	// TODO: document me
	virtual Target transformTarget(const Mechanics * m, const Target & aimPoint, const Target & spellTarget) const = 0;

	// Returns total damage or heal amount that this spell will result in when cast on unit
	virtual SpellEffectValue getHealthChange(const Mechanics * m, const Target & spellTarget) const { return {};}

	/// Serializes (or deserializes) parameters of Effect
	void serializeJson(JsonSerializeFormat & handler);

protected:
	virtual void serializeJsonEffect(JsonSerializeFormat & handler) = 0;
};

}
}

VCMI_LIB_NAMESPACE_END
