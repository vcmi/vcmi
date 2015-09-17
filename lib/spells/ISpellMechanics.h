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
#include "../BattleState.h"
#include "../NetPacks.h"

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

///helper struct
struct DLL_LINKAGE BattleSpellCastParameters
{
public:
	BattleSpellCastParameters(const BattleInfo * cb);
	///spell school level , 0-use default
	int spellLvl;
	BattleHex destination;
	ui8 casterSide;
	PlayerColor casterColor;
	const CGHeroInstance * casterHero; //deprecated
	ECastingMode::ECastingMode mode;
	const CStack * casterStack;
	const CStack * selectedStack;
	const BattleInfo * cb;
	
	///spell school level to use for effects, 0-use spellLvl
	int effectLevel;
	///actual spell-power affecting effect values, 0-use default
	int effectPower;
	///actual spell-power affecting effect duration, 0-use default
	int enchantPower;
	///for Archangel-like casting, 0-use default
	int effectValue;	
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
		PlayerColor casterColor;
		int schoolLvl;

		SpellTargetingContext(const CSpell * s, const CBattleInfoCallback * c, ECastingMode::ECastingMode m, PlayerColor cc, int lvl, BattleHex dest)
			: cb(c), ti(s,lvl, m), mode(m), destination(dest), casterColor(cc), schoolLvl(lvl)
		{};

	};
public:
	ISpellMechanics(CSpell * s);
	virtual ~ISpellMechanics(){};

	virtual std::vector<BattleHex> rangeInHexes(BattleHex centralHex, ui8 schoolLvl, ui8 side, bool * outDroppedHexes = nullptr) const = 0;
	virtual std::set<const CStack *> getAffectedStacks(SpellTargetingContext & ctx) const = 0;
	
	virtual ESpellCastProblem::ESpellCastProblem canBeCasted(const CBattleInfoCallback * cb, PlayerColor player) const = 0;
	
	virtual ESpellCastProblem::ESpellCastProblem isImmuneByStack(const CGHeroInstance * caster, const CStack * obj) const = 0;
	
	virtual void applyBattle(BattleInfo * battle, const BattleSpellCast * packet) const = 0;
	virtual bool adventureCast(const SpellCastEnvironment * env, AdventureSpellCastParameters & parameters) const = 0;
	virtual void battleCast(const SpellCastEnvironment * env, BattleSpellCastParameters & parameters) const = 0;

	virtual void battleLogSingleTarget(std::vector<std::string> & logLines, const BattleSpellCast * packet, 
		const std::string & casterName, const CStack * attackedStack, bool & displayDamage) const = 0;
	
	static ISpellMechanics * createMechanics(CSpell * s);
protected:
	CSpell * owner;
};
