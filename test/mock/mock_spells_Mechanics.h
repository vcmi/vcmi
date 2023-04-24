/*
 * mock_spells_Mechanics.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "../../lib/spells/ISpellMechanics.h"

namespace spells
{

class MechanicsMock : public Mechanics
{
public:
	MOCK_CONST_METHOD2(adaptProblem, bool(ESpellCastProblem::ESpellCastProblem, Problem &));
	MOCK_CONST_METHOD1(adaptGenericProblem, bool(Problem &));

	MOCK_CONST_METHOD1(rangeInHexes, std::vector<BattleHex>(BattleHex));
	MOCK_CONST_METHOD1(getAffectedStacks, std::vector<const CStack *>(const Target &));

	MOCK_CONST_METHOD1(canBeCast, bool(Problem &));
	MOCK_CONST_METHOD2(canBeCastAt, bool(const Target &, Problem &));

	MOCK_CONST_METHOD4(applyEffects, void(ServerCallback *, const Target &, bool, bool));

	MOCK_METHOD2(cast, void(ServerCallback *, const Target &));
	MOCK_METHOD2(castEval, void(ServerCallback *, const Target &));

	MOCK_CONST_METHOD1(isReceptive, bool(const battle::Unit * ));
	MOCK_CONST_METHOD0(getTargetTypes, std::vector<AimType>());
	MOCK_CONST_METHOD3(getPossibleDestinations, std::vector<Destination>(size_t, AimType, const Target &));

	MOCK_CONST_METHOD0(getSpell, const Spell *());

	MOCK_CONST_METHOD0(getEffectLevel, IBattleCast::Value());
	MOCK_CONST_METHOD0(getRangeLevel, IBattleCast::Value());
	MOCK_CONST_METHOD0(getEffectPower, IBattleCast::Value());
	MOCK_CONST_METHOD0(getEffectDuration, IBattleCast::Value());
	MOCK_CONST_METHOD0(getEffectValue, IBattleCast::Value64());

	MOCK_CONST_METHOD0(getCasterColor, PlayerColor());

	MOCK_CONST_METHOD0(getSpellIndex, int32_t());
	MOCK_CONST_METHOD0(getSpellId, SpellID());
	MOCK_CONST_METHOD0(getSpellName, std::string());
	MOCK_CONST_METHOD0(getSpellLevel, int32_t());

	MOCK_CONST_METHOD0(isSmart, bool());
	MOCK_CONST_METHOD0(isMassive, bool());
	MOCK_CONST_METHOD0(alwaysHitFirstTarget, bool());
	MOCK_CONST_METHOD0(requiresClearTiles, bool());
	MOCK_CONST_METHOD0(isNegativeSpell, bool());
	MOCK_CONST_METHOD0(isPositiveSpell, bool());

	MOCK_CONST_METHOD1(adjustEffectValue,int64_t(const battle::Unit *));
	MOCK_CONST_METHOD2(applySpellBonus,int64_t(int64_t, const battle::Unit *));
	MOCK_CONST_METHOD1(applySpecificSpellBonus,int64_t(int64_t));
	MOCK_CONST_METHOD2(calculateRawEffectValue, int64_t(int32_t, int32_t));

	MOCK_CONST_METHOD0(getElementalImmunity, std::vector<Bonus::BonusType>());

	MOCK_CONST_METHOD1(ownerMatches, bool(const battle::Unit *));
	MOCK_CONST_METHOD2(ownerMatches, bool(const battle::Unit *, const boost::logic::tribool));

	MOCK_CONST_METHOD0(creatures, const CreatureService *());
	MOCK_CONST_METHOD0(spells, const Service *());

	MOCK_CONST_METHOD0(game, const IGameInfoCallback * ());
	MOCK_CONST_METHOD0(battle, const CBattleInfoCallback * ());
	
	MOCK_CONST_METHOD0(isMagicalEffect, bool());
};

}

