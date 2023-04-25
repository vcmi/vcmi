/*
 * mock_battle_Unit.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "../../lib/battle/Unit.h"

class UnitMock : public battle::Unit
{
public:
	MOCK_CONST_METHOD4(getAllBonuses, TConstBonusListPtr(const CSelector &, const CSelector &, const CBonusSystemNode *, const std::string &));
	MOCK_CONST_METHOD0(getTreeVersion, int64_t());

	MOCK_CONST_METHOD0(getCasterUnitId, int32_t());
	MOCK_CONST_METHOD2(getSpellSchoolLevel, int32_t(const spells::Spell *, int32_t *));
	MOCK_CONST_METHOD1(getEffectLevel, int32_t(const spells::Spell *));
	MOCK_CONST_METHOD3(getSpellBonus, int64_t(const spells::Spell *, int64_t, const battle::Unit *));
	MOCK_CONST_METHOD2(getSpecificSpellBonus, int64_t(const spells::Spell *, int64_t));
	MOCK_CONST_METHOD1(getEffectPower, int32_t(const spells::Spell *));
	MOCK_CONST_METHOD1(getEnchantPower, int32_t(const spells::Spell *));
	MOCK_CONST_METHOD1(getEffectValue, int64_t(const spells::Spell *));
	MOCK_CONST_METHOD0(getCasterOwner, PlayerColor());
	MOCK_CONST_METHOD1(getCasterName, void(MetaString &));
	MOCK_CONST_METHOD3(getCastDescription, void(const spells::Spell *, const std::vector<const battle::Unit *> &, MetaString &));
	MOCK_CONST_METHOD2(spendMana, void(ServerCallback *, const int32_t));
	MOCK_CONST_METHOD0(manaLimit, int32_t());
	MOCK_CONST_METHOD0(getHeroCaster, CGHeroInstance*());

	MOCK_CONST_METHOD0(unitBaseAmount, int32_t());
	MOCK_CONST_METHOD0(unitId, uint32_t());
	MOCK_CONST_METHOD0(unitSide, ui8());
	MOCK_CONST_METHOD0(unitOwner, PlayerColor());
	MOCK_CONST_METHOD0(unitSlot, SlotID());
	MOCK_CONST_METHOD0(unitType, const CCreature * ());

	MOCK_CONST_METHOD0(doubleWide, bool());

	MOCK_CONST_METHOD0(creatureIndex, int32_t());
	MOCK_CONST_METHOD0(creatureId, CreatureID());
	MOCK_CONST_METHOD0(creatureLevel, int32_t());
	MOCK_CONST_METHOD0(creatureCost, int32_t());
	MOCK_CONST_METHOD0(creatureIconIndex, int32_t());

	MOCK_CONST_METHOD0(ableToRetaliate, bool());
	MOCK_CONST_METHOD0(alive, bool());
	MOCK_CONST_METHOD0(isGhost, bool());
	MOCK_CONST_METHOD0(isFrozen, bool());
	MOCK_CONST_METHOD1(isValidTarget, bool(bool));

	MOCK_CONST_METHOD0(isClone, bool());
	MOCK_CONST_METHOD0(hasClone, bool());
	MOCK_CONST_METHOD0(canCast, bool());
	MOCK_CONST_METHOD0(isCaster, bool());
	MOCK_CONST_METHOD0(canShoot, bool());
	MOCK_CONST_METHOD0(isShooter, bool());

	MOCK_CONST_METHOD0(getCount, int32_t());
	MOCK_CONST_METHOD0(getFirstHPleft, int32_t());
	MOCK_CONST_METHOD0(getKilled, int32_t());
	MOCK_CONST_METHOD0(getAvailableHealth, int64_t());
	MOCK_CONST_METHOD0(getTotalHealth, int64_t());

	MOCK_CONST_METHOD1(getTotalAttacks, int(bool));

	MOCK_CONST_METHOD0(getPosition, BattleHex());
	MOCK_METHOD1(setPosition, void(BattleHex));
	MOCK_CONST_METHOD1(getInitiative, int32_t(int));

	MOCK_CONST_METHOD1(canMove, bool(int));
	MOCK_CONST_METHOD1(defended, bool(int));
	MOCK_CONST_METHOD1(moved, bool(int));
	MOCK_CONST_METHOD1(willMove, bool(int));
	MOCK_CONST_METHOD1(waited, bool(int));

	MOCK_CONST_METHOD0(getFaction, FactionID());

	MOCK_CONST_METHOD1(battleQueuePhase, battle::BattlePhases::Type(int));

	MOCK_CONST_METHOD0(acquire, std::shared_ptr<battle::Unit>());
	MOCK_CONST_METHOD0(acquireState, std::shared_ptr<battle::CUnitState>());

	MOCK_METHOD1(save, void(JsonNode &));
	MOCK_METHOD1(load, void(const JsonNode &));

	MOCK_METHOD1(damage, void(int64_t &));
	MOCK_METHOD3(heal, void(int64_t &, EHealLevel, EHealPower));
};


