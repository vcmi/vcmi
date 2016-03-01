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
#include "../BattleHex.h"


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

	virtual bool moveHero(ObjectInstanceID hid, int3 dst, ui8 teleporting, PlayerColor asker = PlayerColor::NEUTRAL) const =0;	//TODO: remove
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

	BattleSpellCastParameters(const BattleInfo * cb, const ISpellCaster * caster, const CSpell * spell);
	void aimToHex(const BattleHex & destination);
	void aimToStack(const CStack * destination);
	BattleHex getFirstDestinationHex() const;

	const BattleInfo * cb;
	const ISpellCaster * caster;
	const PlayerColor casterColor;
	const ui8 casterSide;

	std::vector<Destination> destinations;

	const CGHeroInstance * casterHero; //deprecated
	ECastingMode::ECastingMode mode;
	const CStack * casterStack; //deprecated
	const CStack * selectedStack;//deprecated

	///spell school level
	int spellLvl;
	///spell school level to use for effects
	int effectLevel;
	///actual spell-power affecting effect values
	int effectPower;
	///actual spell-power affecting effect duration
	int enchantPower;
	///for Archangel-like casting
	int effectValue;
private:
	void prepare(const CSpell * spell);
};

struct DLL_LINKAGE AdventureSpellCastParameters
{
	const CGHeroInstance * caster;
	int3 pos;
};

class DLL_LINKAGE ISpellMechanics
{
public:
	struct DLL_LINKAGE SpellTargetingContext
	{
		const CBattleInfoCallback * cb;
		CSpell::TargetInfo ti;
		ECastingMode::ECastingMode mode;
		BattleHex destination;
		const ISpellCaster * caster;
		int schoolLvl;

		SpellTargetingContext(const CSpell * s, const CBattleInfoCallback * c, ECastingMode::ECastingMode m, const ISpellCaster * caster_, int lvl, BattleHex dest)
			: cb(c), ti(s,lvl, m), mode(m), destination(dest), caster(caster_), schoolLvl(lvl)
		{};

	};
public:
	ISpellMechanics(CSpell * s);
	virtual ~ISpellMechanics(){};

	virtual std::vector<BattleHex> rangeInHexes(BattleHex centralHex, ui8 schoolLvl, ui8 side, bool * outDroppedHexes = nullptr) const = 0;
	virtual std::set<const CStack *> getAffectedStacks(SpellTargetingContext & ctx) const = 0;

	virtual ESpellCastProblem::ESpellCastProblem canBeCast(const CBattleInfoCallback * cb, const ISpellCaster * caster) const = 0;

	virtual ESpellCastProblem::ESpellCastProblem canBeCast(const SpellTargetingContext & ctx) const = 0;

	virtual ESpellCastProblem::ESpellCastProblem isImmuneByStack(const ISpellCaster * caster, const CStack * obj) const = 0;

	virtual void applyBattle(BattleInfo * battle, const BattleSpellCast * packet) const = 0;
	virtual bool adventureCast(const SpellCastEnvironment * env, AdventureSpellCastParameters & parameters) const = 0;
	virtual void battleCast(const SpellCastEnvironment * env, BattleSpellCastParameters & parameters) const = 0;

	virtual void battleLogSingleTarget(std::vector<std::string> & logLines, const BattleSpellCast * packet,
		const std::string & casterName, const CStack * attackedStack, bool & displayDamage) const = 0;

	static ISpellMechanics * createMechanics(CSpell * s);
protected:
	CSpell * owner;
};
