/*
 * ISpellMechanics.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "CSpellHandler.h"
#include "../battle/BattleHex.h"

struct Query;

///callback to be provided by server
class DLL_LINKAGE SpellCastEnvironment
{
public:
	virtual ~SpellCastEnvironment(){};
	virtual void sendAndApply(CPackForClient * info) const = 0;

	virtual CRandomGenerator & getRandomGenerator() const = 0;
	virtual void complain(const std::string & problem) const = 0;

	virtual const CMap * getMap() const = 0;
	virtual const CGameInfoCallback * getCb() const = 0;

	virtual bool moveHero(ObjectInstanceID hid, int3 dst, bool teleporting) const = 0;	//TODO: remove

	virtual void genericQuery(Query * request, PlayerColor color, std::function<void(const JsonNode &)> callback) const = 0;//TODO: type safety on query, use generic query packet when implemented
};

///all parameters of particular cast event
struct DLL_LINKAGE BattleSpellCastParameters
{
public:
	///Single spell destination.
	/// (assumes that anything but battle stack can share same hex)
	struct DLL_LINKAGE Destination
	{
		explicit Destination(const CStack * destination);
		explicit Destination(const BattleHex & destination);

		const CStack * stackValue;
		const BattleHex hexValue;
	};

	//normal constructor
	BattleSpellCastParameters(const BattleInfo * cb, const ISpellCaster * caster, const CSpell * spell_);

	//magic mirror constructor
	BattleSpellCastParameters(const BattleSpellCastParameters & orig, const ISpellCaster * caster);

	void aimToHex(const BattleHex & destination);
	void aimToStack(const CStack * destination);

	void cast(const SpellCastEnvironment * env);

	///cast with silent check for permitted cast
	///returns true if cast was permitted
	bool castIfPossible(const SpellCastEnvironment * env);

	BattleHex getFirstDestinationHex() const;

	int getEffectValue() const;

	const CSpell * spell;
	const BattleInfo * cb;
	const ISpellCaster * caster;
	const PlayerColor casterColor;
	const ui8 casterSide;

	std::vector<Destination> destinations;

	const CGHeroInstance * casterHero; //deprecated
	ECastingMode::ECastingMode mode;
	const CStack * casterStack; //deprecated

	///spell school level
	int spellLvl;
	///spell school level to use for effects
	int effectLevel;
	///actual spell-power affecting effect values
	int effectPower;
	///actual spell-power affecting effect duration
	int enchantPower;

private:
	///for Archangel-like casting
	int effectValue;
};

class DLL_LINKAGE ISpellMechanics
{
public:
	ISpellMechanics(const CSpell * s);
	virtual ~ISpellMechanics(){};

	virtual std::vector<BattleHex> rangeInHexes(BattleHex centralHex, ui8 schoolLvl, ui8 side, bool * outDroppedHexes = nullptr) const = 0;
	virtual std::vector<const CStack *> getAffectedStacks(const CBattleInfoCallback * cb, const ECastingMode::ECastingMode mode, const ISpellCaster * caster, int spellLvl, BattleHex destination) const = 0;

	virtual ESpellCastProblem::ESpellCastProblem canBeCast(const CBattleInfoCallback * cb, const ECastingMode::ECastingMode mode, const ISpellCaster * caster) const = 0;

	virtual ESpellCastProblem::ESpellCastProblem canBeCastAt(const CBattleInfoCallback * cb, const ECastingMode::ECastingMode mode, const ISpellCaster * caster, BattleHex destination) const = 0;

	virtual ESpellCastProblem::ESpellCastProblem isImmuneByStack(const ISpellCaster * caster, const CStack * obj) const = 0;

	virtual void applyBattle(BattleInfo * battle, const BattleSpellCast * packet) const = 0;
	virtual void battleCast(const SpellCastEnvironment * env, const BattleSpellCastParameters & parameters) const = 0;

	//if true use generic algorithm for target existence check, see CSpell::canBeCast
	virtual bool requiresCreatureTarget() const = 0;

	static std::unique_ptr<ISpellMechanics> createMechanics(const CSpell * s);
protected:
	const CSpell * owner;
};

struct DLL_LINKAGE AdventureSpellCastParameters
{
	const CGHeroInstance * caster;
	int3 pos;
};

class DLL_LINKAGE IAdventureSpellMechanics
{
public:
	IAdventureSpellMechanics(const CSpell * s);
	virtual ~IAdventureSpellMechanics() = default;

	virtual bool adventureCast(const SpellCastEnvironment * env, const AdventureSpellCastParameters & parameters) const = 0;

	static std::unique_ptr<IAdventureSpellMechanics> createMechanics(const CSpell * s);
protected:
	const CSpell * owner;
};
