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

	static ISpellMechanics * createMechanics(CSpell * s);
protected:
	CSpell * owner;
};
