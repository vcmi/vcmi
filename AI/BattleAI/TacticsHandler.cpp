/*
 * Tactics.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h" // IWYU pragma: keep

#include "TacticsHandler.h" // IWYU pragma: keep

#include "CStack.h"
#include "battle/BattleAction.h"
#include "battle/BattleHex.h"

namespace
{
	void debug(const std::string & msg)
	{
		logAi->debug("[Tactics] " + msg);
	}

	void info(const std::string & msg)
	{
		logAi->info("[Tactics] " + msg);
	}

	int CalcStackValue(const CStack * stack)
	{
		return stack->getCount() * stack->unitType()->getAIValue();
	}

	BattleHex CloneHex(const BattleHex & bh, const std::initializer_list<BattleHex::EDir> & dirs)
	{
		auto res = bh;
		for(const auto dir : dirs)
			res = res.cloneInDirection(dir, false);
		return res;
	}

	struct VipInfo
	{
		const BattleHex vipPos;
		const BattleSide vipSide;
		const bool vipWide;
		const bool guardWide;

		bool operator==(const VipInfo &) const = default;
	};

	struct VipInfoHash
	{
		std::size_t operator()(const VipInfo & vi) const
		{
			std::size_t h = std::hash<si16>{}(vi.vipPos.toInt());
			h ^= std::hash<int>{}(static_cast<int>(vi.vipSide)) << 1;
			h ^= std::hash<bool>{}(vi.guardWide) << 2;
			h ^= std::hash<bool>{}(vi.vipWide) << 3;
			return h;
		}
	};
}

TacticsHandler::TacticsHandler(const std::shared_ptr<CBattleCallback> & cb, const BattleID & bid, Settings settings)
: cb(cb), battle(cb->getBattle(bid)), bid(bid), settings(settings)
{
}

bool TacticsHandler::canHandle() const
{
	auto myStacks = battle->battleGetStacks(CBattleInfoEssentials::ONLY_MINE);
	auto enemyStacks = battle->battleGetStacks(CBattleInfoEssentials::ONLY_ENEMY);

	auto myArmyValue = 0;
	for (const CStack * stack : myStacks)
		myArmyValue += CalcStackValue(stack);

	auto immuneToDeathCloud = std::ranges::all_of(
		myStacks,
		[](const CStack * stack)
		{
			return stack->hasBonusOfType(BonusType::UNDEAD)
				|| stack->hasBonusOfType(BonusType::NON_LIVING);
		}
	);

	auto haveEnemyShooterWithAoE = std::ranges::any_of(
		enemyStacks,
		[this, myArmyValue, immuneToDeathCloud](const CStack * stack)
		{
			// Ignore weak shooters
			if(static_cast<float>(CalcStackValue(stack)) / myArmyValue < settings.vipThreshold)
				return false;

			for(const auto & bonus : *stack->getBonusesOfType(BonusType::SPELL_LIKE_ATTACK))
			{
				const auto spellID = bonus->subtype.as<SpellID>();
				if(spellID == SpellID::FIREBALL || (spellID == SpellID::DEATH_CLOUD && !immuneToDeathCloud))
					return true;
			}

			return false;
		});

	if (haveEnemyShooterWithAoE)
	{
		info("Enemy AoE shooter found; stop");
		return false;
	}

	auto haveFastEnemyWithBreath = std::ranges::any_of(
		enemyStacks,
		[](const CStack * stack)
		{
			if(!stack->hasBonusOfType(BonusType::TWO_HEX_ATTACK_BREATH))
				return false;

			const auto y = stack->getPosition().getY();
			const auto speed = stack->getMovementRange() + static_cast<int>(stack->doubleWide());

			if (y == 0 || y == 10)
				return speed >= 11;
			else if (y <= 2 || y >= 8)
				return speed >= 12;
			else if (y <= 4 || y >= 6)
				return speed >= 13;
			else
				return speed >= 14;
		});

	if (haveFastEnemyWithBreath)
	{
		info("Fast enemy with breath found; stop");
		return false;
	}

	return true;
}

void TacticsHandler::end()
{
	info("Ending tactics");
	cb->battleMakeTacticAction(bid, BattleAction::makeEndOFTacticPhase(battle->battleGetMySide()));
};

std::vector<const CStack *> TacticsHandler::findVIPs() const
{
	struct VipData
	{
		const CStack * stack;
		int score;
	};

	auto vipdatas = std::vector<VipData>{};
	int armyValue = 0;

	const auto mystacks = battle->battleGetStacks(CBattleInfoEssentials::EStackOwnership::ONLY_MINE);

	for(const auto & stack : mystacks)
	{
		int value = CalcStackValue(stack);
		armyValue += value;
		// growth > 0 excludes ballistas, commanders, etc.
		if(stack->unitType()->getGrowth() > 0 && stack->isShooter())
		{
			bool noMelee = stack->hasBonusOfType(BonusType::NO_MELEE_PENALTY);
			auto mult = noMelee ? 0.7 : 1.0;
			auto score = static_cast<int>(value * mult);
			vipdatas.emplace_back(VipData{.stack = stack, .score = score});
		}
	}

	// Ignore weak vips
	std::erase_if(
		vipdatas,
		[this, armyValue](const VipData & vipdata)
		{
			return vipdata.score < settings.vipThreshold * armyValue;
		}
	);

	// Sort by score (descending)
	std::ranges::sort(vipdatas, std::greater{}, &VipData::score);

	// Map to cstack
	auto res = std::vector<const CStack *>{};
	res.reserve(vipdatas.size());
	std::ranges::transform(vipdatas, std::back_inserter(res), &VipData::stack);

	return res;
}

std::vector<const CStack *> TacticsHandler::findGuards() const
{
	auto stacks = battle->battleGetStacks(CBattleInfoEssentials::ONLY_MINE);
	assert(stacks.size() > 0);
	std::erase_if(
		stacks,
		[this](const CStack * guard)
		{
			return guard->getMovementRange() == 0 \
				|| std::ranges::find(vips, guard) != vips.end();
		}
	);
	std::ranges::sort(stacks, std::less{}, CalcStackValue);
	return stacks;
}

std::vector<BattleHex> TacticsHandler::guardableHexes(const CStack * vip, const CStack * guard) // NOSONAR
{
	auto res = std::vector<BattleHex>{};
	res.reserve(16);

	BattleHex vipHead = vip->getPosition();

	static auto cache = std::unordered_map<VipInfo, std::vector<BattleHex>, VipInfoHash>{};
	const auto vi = VipInfo{.vipPos = vip->getPosition(), .vipSide = vip->unitSide(), .vipWide = vip->doubleWide(), .guardWide = guard->doubleWide()};

	auto it = cache.find(vi);

	if(it != cache.end())
		return it->second;

	using EDir = BattleHex::EDir;
	auto L = EDir::LEFT;
	auto TL = EDir::TOP_LEFT;
	auto BL = EDir::BOTTOM_LEFT;
	auto R = EDir::RIGHT;
	auto TR = EDir::TOP_RIGHT;
	auto BR = EDir::BOTTOM_RIGHT;

	auto add = [&res, &vipHead](const std::initializer_list<BattleHex::EDir> dirs)
	{
		auto hex = CloneHex(vipHead, dirs);
		if(hex.isAvailable())
			res.push_back(hex);
	};

	auto convex = vip->unitSide() == BattleSide::LEFT_SIDE ? vipHead.getY() % 2 == 0 : vipHead.getY() % 2 == 1;

	std::ostringstream oss;
	oss << "vipHead=" << vipHead.toInt()
		<< " vip->unitSide()=" << static_cast<int>(vip->unitSide())
		<< " / guard->doubleWide()=" << static_cast<int>(guard->doubleWide())
		<< " / vip->doubleWide()=" << static_cast<int>(vip->doubleWide())
		<< " / vipHead.getY()=" << static_cast<int>(vipHead.getY());
	debug(oss.str());

	if(vip->unitSide() == BattleSide::RIGHT_SIDE)
	{
		if(guard->doubleWide())
		{
			if(vip->doubleWide())
			{
				/*
				 *  2-hex VIP, 2-hex guard
				 *  (R/convex)              (R/concave)
				 *  . . . . .                 . . . . .
				 * . . 6 1 3 .               . . 6 1 .
				 *  . 5 . x ~                 . 5 . x ~
				 * . . 7 2 4 .               . . 7 2 .
				 *  . . . . .                 . . . . .
				 */
				add({TL}); // 1
				add({BL}); // 2

				if(convex)
				{
					add({TR}); // 3
					add({BR}); // 4
				}

				add({L, L}); // 5
				add({L, TL}); // 6
				add({L, BL}); // 7
			}
			else
			{
				/*
				 *  1-hex VIP, 2-hex guard
				 *  (R/convex)              (R/concave)
				 *  . . . . .               . . . . .
				 * . . . 4 1 .             . . . 4 .
				 *  . . 3 . x               . . 3 . x
				 * . . . 5 2 .             . . . 5 .
				 *  . . . . .               . . . . .
				 */
				if(convex)
				{
					add({TL}); // 1
					add({BL}); // 2
				}

				add({L, L}); // 3
				add({L, TL}); // 4
				add({L, BL}); // 5
			}
		}
		else
		{
			if(vip->doubleWide())
			{
				/*
				 *  2-hex VIP, 1-hex guard
				 *  (R/convex)              (R/concave)
				 *  . . . . .                 . . . . . .
				 * . . . 2 4 6                 . . . 2 4
				 *  . . 1 x ~                 . . . 1 x ~
				 * . . . 3 5 7                 . . . 3 5
				 *  . . . . .                 . . . . . .
				 */
				add({L}); // 1
				add({TL}); // 2
				add({BL}); // 3
				add({TR}); // 4
				add({BR}); // 5

				if(convex)
				{
					add({R, TR}); // 6
					add({R, BR}); // 7
				}
			}
			else
			{
				/*
				 *  1-hex VIP, 1-hex guard
				 *  (R/convex)              (R/concave)
				 *  . . . . .                 . . . . .
				 * . . . . 2 4               . . . . 2
				 *  . . . 1 x                 . . . 1 x
				 * . . . . 3 5               . . . . 3
				 *  . . . . .                 . . . . .
				 */
				add({L}); // 1
				add({TL}); // 2
				add({BL}); // 3

				if(convex)
				{
					add({TR}); // 4
					add({BR}); // 5
				}
			}
		}
	}
	else
	{
		if(guard->doubleWide())
		{
			if(vip->doubleWide())
			{
				/*
				 *  2-hex VIP, 2-hex guard
				 *  (L/convex)              (L/concave)
				 *  . . . . .                 . . . . .
				 * . 3 1 6 . .                 . 1 6 . .
				 *  ~ x . 5 .                 ~ x . 5 .
				 * . 4 2 7 . .                 . 2 7 . .
				 *  . . . . .                 . . . . .
				 */
				add({TR}); // 1
				add({BR}); // 2

				if(convex)
				{
					add({TL}); // 3
					add({BL}); // 4
				}

				add({R, R}); // 5
				add({R, TR}); // 6
				add({R, BR}); // 7
			}
			else
			{
				/*
				 *  1-hex VIP, 2-hex guard
				 *  (L/convex)              (L/concave)
				 *  . . . . .               . . . . .
				 * . 1 4 . . .               . 4 . . .
				 *  x . 3 . .               x . 3 . .
				 * . 2 5 . . .               . 5 . . .
				 *  . . . . .               . . . . .
				 */
				if(convex)
				{
					add({TR}); // 1
					add({BR}); // 2
				}

				add({R, R}); // 3
				add({R, TR}); // 4
				add({R, BR}); // 5
			}
		}
		else
		{
			if(vip->doubleWide())
			{
				/*
				 *  2-hex VIP, 1-hex guard
				 *  (L/upper/convex)         (L/upper/concave)
				 *  . . . . .                 . . . . . .
				 * 6 4 2 . . .                 4 2 . . . .
				 *  ~ x 1 . .                 ~ x 1 . . .
				 * 7 5 3 . . .                 5 3 . . . .
				 *  . . . . .                 . . . . . .
				 */
				add({R}); // 1
				add({TR}); // 2
				add({BR}); // 3
				add({TL}); // 4
				add({BL}); // 5

				if(convex)
				{
					add({L, TL}); // 6
					add({L, BL}); // 7
				}
			}
			else
			{
				/*
				 *  1-hex VIP, 1-hex guard
				 *  (L/convex)              (L/concave)
				 *  . . . . .                 . . . . .
				 * 4 2 . . . .                 2 . . . .
				 *  x 1 . . .                 x 1 . . .
				 * 5 3 . . . .                 3 . . . .
				 *  . . . . .                 . . . . .
				 */
				add({R}); // 1
				add({TR}); // 2
				add({BR}); // 3

				if(convex)
				{
					add({TL}); // 4
					add({BL}); // 5
				}
			}
		}
	}

	cache.try_emplace(vi, res);
	return res;
}

TacticsHandler::SpecialHexes TacticsHandler::getSpecialHexes() const
{
	auto side = battle->battleGetMySide();
	auto isLeft = side == BattleSide::LEFT_SIDE;
	const auto & [cornerHex1, cornerHex2] = isLeft ? std::pair<BattleHex, BattleHex>{1, 171} : std::pair<BattleHex, BattleHex>{15, 185};

	auto tempHexes = isLeft ? std::vector<BattleHex>{69, 103, 86, 52, 120} : std::vector<BattleHex>{83, 117, 100, 66, 134};
	auto offset = isLeft ? 1 : -1;
	auto tempSize = tempHexes.size();

	for(int m = 0; m < 2; ++m)
		for(int i = 0; i < tempSize; ++i)
			tempHexes.emplace_back(tempHexes[i].toInt() + (m * offset));

	return {.corner1=cornerHex1,
			.corner2=cornerHex2,
			.corner1Wide=BattleHex(cornerHex1.toInt() + offset),
			.corner2Wide=BattleHex(cornerHex2.toInt() + offset),
			.tempHexes=tempHexes};
}


void TacticsHandler::onTacticsStarted()
{
	if (battle->battleTacticDist() == 0)
	{
		phase = Phase::INACTIVE;
		return;
	}

	if (!settings.enabled || !canHandle())
	{
		phase = Phase::INACTIVE;
		end();
		return;
	}

	handle();
}

void TacticsHandler::tacticMove(const CStack * cstack, const BattleHex & bh)
{
	logAi->debug("[Tactics] Moving to hex %d", bh.toInt());
	movingStack = cstack;
	cb->battleMakeUnitAction(bid, BattleAction::makeMove(cstack, bh));
}

void TacticsHandler::handle()
{
	vips = findVIPs();
	guards = findGuards();
	specialHexes = getSpecialHexes();
	guardIndex = 0;
	vipIndex = 0;
	guardPass = 0;
	phase = Phase::MOVE_GUARDS_AWAY_FROM_CORNERS;

	for(const CStack * vip : vips)
		logAi->debug("VIP: " + vip->getDescription());

	advance();
}

void TacticsHandler::advance()
{
	while(phase != Phase::INACTIVE)
	{
		bool moveStarted = false;
		switch(phase)
		{
			case Phase::MOVE_GUARDS_AWAY_FROM_CORNERS:
				moveStarted = moveNextGuardAwayFromCorners();
				break;
			case Phase::MOVE_VIPS_TO_CORNERS:
				moveStarted = moveNextVipToCorner();
				break;
			case Phase::MOVE_GUARDS_AROUND_VIPS:
				moveStarted = moveNextGuardAroundVip();
				break;
			case Phase::INACTIVE:
				break;
		}

		if(moveStarted)
			return;
	}

	end();
}

bool TacticsHandler::moveNextGuardAwayFromCorners()
{
	while(guardIndex < guards.size())
	{
		const auto * guard = guards[guardIndex++];
		if(!guard->coversPos(specialHexes.corner1) && !guard->coversPos(specialHexes.corner2))
			continue;

		const auto reachability = battle->getReachability(guard);
		for(const auto & hex : specialHexes.tempHexes)
		{
			if(reachability.isReachable(hex))
			{
				tacticMove(guard, hex);
				return true;
			}
		}
	}

	vipsToMove.clear();
	for(const auto * vip : vips)
	{
		if(vipsToMove.size() < 2 && !vip->coversPos(specialHexes.corner1) && !vip->coversPos(specialHexes.corner2))
			vipsToMove.push_back(vip);
	}
	vipIndex = 0;
	phase = Phase::MOVE_VIPS_TO_CORNERS;
	return false;
}

bool TacticsHandler::moveNextVipToCorner()
{
	while(vipIndex < vipsToMove.size())
	{
		const auto * vip = vipsToMove[vipIndex++];
		const auto reachability = battle->getReachability(vip);
		const auto destinations = vip->doubleWide()
			? std::vector<BattleHex>{specialHexes.corner1Wide, specialHexes.corner2Wide}
			: std::vector<BattleHex>{specialHexes.corner1, specialHexes.corner2};

		for(const auto & hex : destinations)
		{
			if(reachability.isReachable(hex))
			{
				tacticMove(vip, hex);
				return true;
			}
		}
	}

	guardIndex = 0;
	vipIndex = 0;
	phase = Phase::MOVE_GUARDS_AROUND_VIPS;
	return false;
}

bool TacticsHandler::moveNextGuardAroundVip()
{
	// Initial pass might fail if some units were blocked by obstacles.
	// Loop again in case they have become unblocked.
	while(guardPass < 2)
	{
		while(guardIndex < guards.size())
		{
			const auto * guard = guards[guardIndex];
			while(vipIndex < vips.size())
			{
				const auto destination = findGuardDestination(guard, vips[vipIndex++]);
				if(destination)
				{
					++guardIndex;
					vipIndex = 0;
					tacticMove(guard, *destination);
					return true;
				}
			}
			++guardIndex;
			vipIndex = 0;
		}
		++guardPass;
		guardIndex = 0;
	}

	phase = Phase::INACTIVE;
	return false;
}

std::optional<BattleHex> TacticsHandler::findGuardDestination(const CStack * guard, const CStack * vip)
{
	assert(vip);
	logAi->debug("Handling GUARD stack %s (vip=%s)", guard->getDescription(), vip->getDescription());

	const auto hexes = guardableHexes(vip, guard);
	const auto reachability = cb->getBattle(bid)->getReachability(guard);

	for(const auto hex : hexes)
	{
		if(guard->getPosition() == hex)
			break;

		if(reachability.isReachable(hex) && cb->getBattle(bid)->isInTacticRange(hex))
			return hex;
	}

	logAi->debug("No viable guard move (VIP unreachable or already surrounded)");
	return std::nullopt;
}

void TacticsHandler::onActionFinished(const BattleAction & action)
{
	if (battle->battleTacticDist() == 0)
	{
		phase = Phase::INACTIVE;
		return;
	}

	if(!movingStack || action.actionType != EActionType::WALK || action.stackNumber != movingStack->unitId())
		return;

	logAi->debug("[Tactics] Move finished");
	movingStack = nullptr;
	advance();
}
