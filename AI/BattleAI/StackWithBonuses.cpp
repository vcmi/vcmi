/*
 * StackWithBonuses.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "StackWithBonuses.h"
#include "../../lib/NetPacksBase.h"
#include "../../lib/CStack.h"

void actualizeEffect(TBonusListPtr target, const Bonus & ef)
{
	for(auto bonus : *target) //TODO: optimize
	{
		if(bonus->source == Bonus::SPELL_EFFECT && bonus->type == ef.type && bonus->subtype == ef.subtype)
		{
			bonus->turnsRemain = std::max(bonus->turnsRemain, ef.turnsRemain);
		}
	}
}

StackWithBonuses::StackWithBonuses(const HypotheticBattle * Owner, const CStack * Stack)
	: battle::CUnitState(),
	origBearer(Stack),
	owner(Owner),
	type(Stack->unitType()),
	baseAmount(Stack->unitBaseAmount()),
	id(Stack->unitId()),
	side(Stack->unitSide()),
	player(Stack->unitOwner()),
	slot(Stack->unitSlot())
{
	localInit(Owner);

	battle::CUnitState::operator=(*Stack);
}

StackWithBonuses::StackWithBonuses(const HypotheticBattle * Owner, const battle::UnitInfo & info)
	: battle::CUnitState(),
	origBearer(nullptr),
	owner(Owner),
	baseAmount(info.count),
	id(info.id),
	side(info.side),
	slot(SlotID::SUMMONED_SLOT_PLACEHOLDER)
{
	type = info.type.toCreature();
	origBearer = type;

	player = Owner->getSidePlayer(side);

	localInit(Owner);

	position = info.position;
	summoned = info.summoned;
}

StackWithBonuses::~StackWithBonuses() = default;

StackWithBonuses & StackWithBonuses::operator=(const battle::CUnitState & other)
{
	battle::CUnitState::operator=(other);
	return *this;
}

const CCreature * StackWithBonuses::unitType() const
{
	return type;
}

int32_t StackWithBonuses::unitBaseAmount() const
{
	return baseAmount;
}

uint32_t StackWithBonuses::unitId() const
{
	return id;
}

ui8 StackWithBonuses::unitSide() const
{
	return side;
}

PlayerColor StackWithBonuses::unitOwner() const
{
	return player;
}

SlotID StackWithBonuses::unitSlot() const
{
	return slot;
}

const TBonusListPtr StackWithBonuses::getAllBonuses(const CSelector & selector, const CSelector & limit,
	const CBonusSystemNode * root, const std::string & cachingStr) const
{
	TBonusListPtr ret = std::make_shared<BonusList>();
	const TBonusListPtr originalList = origBearer->getAllBonuses(selector, limit, root, cachingStr);

	vstd::copy_if(*originalList, std::back_inserter(*ret), [this](const std::shared_ptr<Bonus> & b)
	{
		return !vstd::contains(bonusesToRemove, b);
	});


	for(const Bonus & bonus : bonusesToUpdate)
	{
		if(selector(&bonus) && (!limit || !limit(&bonus)))
		{
			if(ret->getFirst(Selector::source(Bonus::SPELL_EFFECT, bonus.sid).And(Selector::typeSubtype(bonus.type, bonus.subtype))))
			{
				actualizeEffect(ret, bonus);
			}
			else
			{
				auto b = std::make_shared<Bonus>(bonus);
				ret->push_back(b);
			}
		}
	}

	for(auto & bonus : bonusesToAdd)
	{
		auto b = std::make_shared<Bonus>(bonus);
		if(selector(b.get()) && (!limit || !limit(b.get())))
			ret->push_back(b);
	}
	//TODO limiters?
	return ret;
}

int64_t StackWithBonuses::getTreeVersion() const
{
	return owner->getTreeVersion();
}

void StackWithBonuses::addUnitBonus(const std::vector<Bonus> & bonus)
{
	vstd::concatenate(bonusesToAdd, bonus);
}

void StackWithBonuses::updateUnitBonus(const std::vector<Bonus> & bonus)
{
	//TODO: optimize, actualize to last value

	vstd::concatenate(bonusesToUpdate, bonus);
}

void StackWithBonuses::removeUnitBonus(const std::vector<Bonus> & bonus)
{
	for(auto & one : bonus)
	{
		CSelector selector([&one](const Bonus * b) -> bool
		{
			//compare everything but turnsRemain, limiter and propagator
			return one.duration == b->duration
				&& one.type == b->type
				&& one.subtype == b->subtype
				&& one.source == b->source
				&& one.val == b->val
				&& one.sid == b->sid
				&& one.valType == b->valType
				&& one.additionalInfo == b->additionalInfo
				&& one.effectRange == b->effectRange
				&& one.description == b->description;
		});

		removeUnitBonus(selector);
	}
}

void StackWithBonuses::removeUnitBonus(const CSelector & selector)
{
	TBonusListPtr toRemove = origBearer->getBonuses(selector);

	for(auto b : *toRemove)
		bonusesToRemove.insert(b);

	vstd::erase_if(bonusesToAdd, [&](const Bonus & b){return selector(&b);});
	vstd::erase_if(bonusesToUpdate, [&](const Bonus & b){return selector(&b);});
}

void StackWithBonuses::spendMana(const spells::PacketSender * server, const int spellCost) const
{
	//TODO: evaluate cast use
}

HypotheticBattle::HypotheticBattle(Subject realBattle)
	: BattleProxy(realBattle),
	bonusTreeVersion(1)
{
	auto activeUnit = realBattle->battleActiveUnit();
	activeUnitId = activeUnit ? activeUnit->unitId() : -1;

	nextId = 0xF0000000;
}

bool HypotheticBattle::unitHasAmmoCart(const battle::Unit * unit) const
{
	//FIXME: check ammocart alive state here
	return false;
}

PlayerColor HypotheticBattle::unitEffectiveOwner(const battle::Unit * unit) const
{
	return battleGetOwner(unit);
}

std::shared_ptr<StackWithBonuses> HypotheticBattle::getForUpdate(uint32_t id)
{
	auto iter = stackStates.find(id);

	if(iter == stackStates.end())
	{
		const CStack * s = subject->battleGetStackByID(id, false);

		auto ret = std::make_shared<StackWithBonuses>(this, s);
		stackStates[id] = ret;
		return ret;
	}
	else
	{
		return iter->second;
	}
}

battle::Units HypotheticBattle::getUnitsIf(battle::UnitFilter predicate) const
{
	battle::Units proxyed = BattleProxy::getUnitsIf(predicate);

	battle::Units ret;
	ret.reserve(proxyed.size());

	for(auto unit : proxyed)
	{
		//unit was not changed, trust proxyed data
		if(stackStates.find(unit->unitId()) == stackStates.end())
			ret.push_back(unit);
	}

	for(auto id_unit : stackStates)
	{
		if(predicate(id_unit.second.get()))
			ret.push_back(id_unit.second.get());
	}

	return ret;
}

int32_t HypotheticBattle::getActiveStackID() const
{
	return activeUnitId;
}

void HypotheticBattle::nextRound(int32_t roundNr)
{
	//TODO:HypotheticBattle::nextRound

	for(auto unit : battleAliveUnits())
	{
		auto forUpdate = getForUpdate(unit->unitId());
		//TODO: update Bonus::NTurns effects
		forUpdate->afterNewRound();
	}
}

void HypotheticBattle::nextTurn(uint32_t unitId)
{
	activeUnitId = unitId;
	auto unit = getForUpdate(unitId);

	unit->removeUnitBonus(Bonus::UntilGetsTurn);

	unit->afterGetsTurn();
}

void HypotheticBattle::addUnit(uint32_t id, const JsonNode & data)
{
	battle::UnitInfo info;
	info.load(id, data);
	std::shared_ptr<StackWithBonuses> newUnit = std::make_shared<StackWithBonuses>(this, info);
	stackStates[newUnit->unitId()] = newUnit;
}

void HypotheticBattle::moveUnit(uint32_t id, BattleHex destination)
{
	std::shared_ptr<StackWithBonuses> changed = getForUpdate(id);
	changed->position = destination;
}

void HypotheticBattle::setUnitState(uint32_t id, const JsonNode & data, int64_t healthDelta)
{
	std::shared_ptr<StackWithBonuses> changed = getForUpdate(id);

	changed->load(data);

	if(healthDelta < 0)
	{
		changed->removeUnitBonus(Bonus::UntilBeingAttacked);
	}
}

void HypotheticBattle::removeUnit(uint32_t id)
{
	std::set<uint32_t> ids;
	ids.insert(id);

	while(!ids.empty())
	{
		auto toRemoveId = *ids.begin();

		auto toRemove = getForUpdate(toRemoveId);

		if(!toRemove->ghost)
		{
			toRemove->onRemoved();

			//TODO: emulate detachFromAll() somehow

			//stack may be removed instantly (not being killed first)
			//handle clone remove also here
			if(toRemove->cloneID >= 0)
			{
				ids.insert(toRemove->cloneID);
				toRemove->cloneID = -1;
			}

			//TODO: cleanup remaining clone links if any
//			for(auto s : stacks)
//			{
//				if(s->cloneID == toRemoveId)
//					s->cloneID = -1;
//			}
		}

		ids.erase(toRemoveId);
	}
}

void HypotheticBattle::addUnitBonus(uint32_t id, const std::vector<Bonus> & bonus)
{
	getForUpdate(id)->addUnitBonus(bonus);
	bonusTreeVersion++;
}

void HypotheticBattle::updateUnitBonus(uint32_t id, const std::vector<Bonus> & bonus)
{
	getForUpdate(id)->updateUnitBonus(bonus);
	bonusTreeVersion++;
}

void HypotheticBattle::removeUnitBonus(uint32_t id, const std::vector<Bonus> & bonus)
{
	getForUpdate(id)->removeUnitBonus(bonus);
	bonusTreeVersion++;
}

void HypotheticBattle::setWallState(int partOfWall, si8 state)
{
	//TODO:HypotheticBattle::setWallState
}

void HypotheticBattle::addObstacle(const ObstacleChanges & changes)
{
	//TODO:HypotheticBattle::addObstacle
}

void HypotheticBattle::updateObstacle(const ObstacleChanges& changes)
{
	//TODO:HypotheticBattle::updateObstacle
}

void HypotheticBattle::removeObstacle(uint32_t id)
{
	//TODO:HypotheticBattle::removeObstacle
}

uint32_t HypotheticBattle::nextUnitId() const
{
	return nextId++;
}

int64_t HypotheticBattle::getActualDamage(const TDmgRange & damage, int32_t attackerCount, vstd::RNG & rng) const
{
	return (damage.first + damage.second) / 2;
}

int64_t HypotheticBattle::getTreeVersion() const
{
	return getBattleNode()->getTreeVersion() + bonusTreeVersion;
}
