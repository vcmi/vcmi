/*
 * AdventureSpellEffect.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "../ISpellMechanics.h"

VCMI_LIB_NAMESPACE_BEGIN

enum class ESpellCastResult : int8_t
{
	OK, // cast successful
	CANCEL, // cast failed but it is not an error, no mana has been spent
	PENDING,
	ERROR // error occurred, for example invalid request from player
};

class AdventureSpellMechanics;

class IAdventureSpellEffect
{
public:
	virtual ~IAdventureSpellEffect() = default;

	virtual ESpellCastResult applyAdventureEffects(SpellCastEnvironment * env, const AdventureSpellCastParameters & parameters) const {return ESpellCastResult::OK;};
	virtual ESpellCastResult beginCast(SpellCastEnvironment * env, const AdventureSpellCastParameters & parameters, const AdventureSpellMechanics & mechanics) const {return ESpellCastResult::OK;};
	virtual void endCast(SpellCastEnvironment * env, const AdventureSpellCastParameters & parameters) const {};
	virtual bool canBeCastImpl(spells::Problem & problem, const IGameInfoCallback * cb, const spells::Caster * caster) const {return true;};
	virtual bool canBeCastAtImpl(spells::Problem & problem, const IGameInfoCallback * cb, const spells::Caster * caster, const int3 & pos) const {return true;};
	virtual std::string getCursorForTarget(const IGameInfoCallback * cb, const spells::Caster * caster, const int3 & pos) const {return {};};
};

class AdventureSpellEffect final : public IAdventureSpellEffect
{
public:
	AdventureSpellEffect() = default;
};

class DLL_LINKAGE AdventureSpellRangedEffect : public IAdventureSpellEffect
{
	int rangeX;
	int rangeY;
	bool ignoreFow;

public:
	AdventureSpellRangedEffect(const JsonNode & config);

	bool isTargetInRange(const IGameInfoCallback * cb, const spells::Caster * caster, const int3 & pos) const;
	std::string getCursorForTarget(const IGameInfoCallback * cb, const spells::Caster * caster, const int3 & pos) const override = 0; //must be implemented in derived classes
	bool canBeCastAtImpl(spells::Problem & problem, const IGameInfoCallback * cb, const spells::Caster * caster, const int3 & pos) const override = 0; //must be implemented in derived classes
};

VCMI_LIB_NAMESPACE_END
