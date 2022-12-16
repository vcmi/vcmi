/*
 * Unit.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include <vcmi/spells/Caster.h>

#include "../HeroBonus.h"

#include "IUnitInfo.h"
#include "BattleHex.h"

VCMI_LIB_NAMESPACE_BEGIN

struct MetaString;
class JsonNode;
class JsonSerializeFormat;

namespace battle
{
class CUnitState;

class DLL_LINKAGE Unit : public IUnitInfo, public spells::Caster, public virtual IBonusBearer
{
public:
	virtual ~Unit();

	virtual bool doubleWide() const = 0;

	virtual int32_t creatureIndex() const = 0;
	virtual CreatureID creatureId() const = 0;
	virtual int32_t creatureLevel() const = 0;
	virtual int32_t creatureCost() const = 0;
	virtual int32_t creatureIconIndex() const = 0;

	virtual bool ableToRetaliate() const = 0;
	virtual bool alive() const = 0;
	virtual bool isGhost() const = 0;
	virtual bool isFrozen() const = 0;

	bool isDead() const;
	bool isTurret() const;
	virtual bool isValidTarget(bool allowDead = false) const = 0; //non-turret non-ghost stacks (can be attacked or be object of magic effect)

	virtual bool isClone() const = 0;
	virtual bool hasClone() const = 0;

	virtual bool canCast() const = 0;
	virtual bool isCaster() const = 0;
	virtual bool canShoot() const = 0;
	virtual bool isShooter() const = 0;

	virtual int32_t getCount() const = 0;
	virtual int32_t getFirstHPleft() const = 0;
	virtual int32_t getKilled() const = 0;
	virtual int64_t getAvailableHealth() const = 0;
	virtual int64_t getTotalHealth() const = 0;

	virtual int getTotalAttacks(bool ranged) const = 0;

	virtual BattleHex getPosition() const = 0;
	virtual void setPosition(BattleHex hex) = 0;

	virtual int32_t getInitiative(int turn = 0) const = 0;

	virtual bool canMove(int turn = 0) const = 0; //if stack can move
	virtual bool defended(int turn = 0) const = 0;
	virtual bool moved(int turn = 0) const = 0; //if stack was already moved this turn
	virtual bool willMove(int turn = 0) const = 0; //if stack has remaining move this turn
	virtual bool waited(int turn = 0) const = 0;

	virtual std::shared_ptr<Unit> acquire() const = 0;
	virtual std::shared_ptr<CUnitState> acquireState() const = 0;

	virtual int battleQueuePhase(int turn) const = 0;

	virtual std::string getDescription() const;

	std::vector<BattleHex> getSurroundingHexes(BattleHex assumedPosition = BattleHex::INVALID) const; // get six or 8 surrounding hexes depending on creature size
	std::vector<BattleHex> getAttackableHexes(const Unit * attacker) const;
	static std::vector<BattleHex> getSurroundingHexes(BattleHex position, bool twoHex, ui8 side);

	bool coversPos(BattleHex position) const; //checks also if unit is double-wide

	std::vector<BattleHex> getHexes() const; //up to two occupied hexes, starting from front
	std::vector<BattleHex> getHexes(BattleHex assumedPos) const; //up to two occupied hexes, starting from front
	static std::vector<BattleHex> getHexes(BattleHex assumedPos, bool twoHex, ui8 side);

	BattleHex occupiedHex() const; //returns number of occupied hex (not the position) if stack is double wide; otherwise -1
	BattleHex occupiedHex(BattleHex assumedPos) const; //returns number of occupied hex (not the position) if stack is double wide and would stand on assumedPos; otherwise -1
	static BattleHex occupiedHex(BattleHex assumedPos, bool twoHex, ui8 side);

	///MetaStrings
	void addText(MetaString & text, ui8 type, int32_t serial, const boost::logic::tribool & plural = boost::logic::indeterminate) const;
	void addNameReplacement(MetaString & text, const boost::logic::tribool & plural = boost::logic::indeterminate) const;
	std::string formatGeneralMessage(const int32_t baseTextId) const;

	int getRawSurrenderCost() const;

	//NOTE: save could possibly be const, but this requires heavy changes to Json serialization,
	//also this method should be called only after modifying object
	virtual void save(JsonNode & data) = 0;
	virtual void load(const JsonNode & data) = 0;

	virtual void damage(int64_t & amount) = 0;
	virtual void heal(int64_t & amount, EHealLevel level, EHealPower power) = 0;
};

class DLL_LINKAGE UnitInfo
{
public:
    uint32_t id;
	TQuantity count;
	CreatureID type;
	ui8 side;
	BattleHex position;
	bool summoned;

	UnitInfo();

	void serializeJson(JsonSerializeFormat & handler);

	void save(JsonNode & data);
	void load(uint32_t id_, const JsonNode & data);
};

}

VCMI_LIB_NAMESPACE_END
