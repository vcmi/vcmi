/*
 * hex.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "battle/AccessibilityInfo.h"
#include "battle/BattleHex.h"
#include "battle/CObstacleInstance.h"
#include "battle/ReachabilityInfo.h"
#include "constants/Enumerations.h"

#include "BAI/v13/stack.h"
#include "schema/v13/types.h"

#include <memory>

namespace MMAI::BAI::V13
{
using HexAction = Schema::V13::HexAction;
using HexAttribute = Schema::V13::HexAttribute;
using HexAttrs = Schema::V13::HexAttrs;
using HexState = Schema::V13::HexState;

using HexActionMask = std::bitset<EI(HexAction::_count)>;
using HexStateMask = std::bitset<EI(HexState::_count)>;
using HexActionHex = std::array<BattleHex, 12>;

struct ActiveStackInfo
{
	const Stack * stack;
	const bool canshoot;
	const std::shared_ptr<ReachabilityInfo> rinfo;

	ActiveStackInfo(const Stack * stack_, const bool canshoot_, const std::shared_ptr<ReachabilityInfo> & rinfo_)
		: stack(stack_), canshoot(canshoot_), rinfo(rinfo_) {};
};

/*
 * A wrapper around BattleHex. Differences:
 *
 * x is 0..14     (instead of 0..16),
 * id is 0..164  (instead of 0..177)
 */
class Hex : public Schema::V13::IHex
{
public:
	static int CalcId(const BattleHex & bh);
	static std::pair<int, int> CalcXY(const BattleHex & bh);
	static HexActionHex NearbyBattleHexes(const BattleHex & bh);

	Hex(const BattleHex & bh,
		EAccessibility accessibility,
		EGateState gatestate,
		const std::vector<std::shared_ptr<const CObstacleInstance>> & obstacles,
		const std::map<BattleHex, std::shared_ptr<Stack>> & hexstacks,
		const std::shared_ptr<ActiveStackInfo> & astackinfo);

	// IHex impl
	const HexAttrs & getAttrs() const override;
	int getID() const override;
	int getAttr(HexAttribute a) const override;
	const Stack * getStack() const override;

	const BattleHex bhex;
	const int id;
	std::shared_ptr<const Stack> stack = nullptr;
	HexAttrs attrs = {};
	HexActionMask actmask = 0; // for active stack only
	HexStateMask statemask = 0; //

	std::string name() const;
	int attr(HexAttribute a) const;

private:
	void setattr(HexAttribute a, int value);
	void finalize();

	void setStateMask(EAccessibility accessibility, const std::vector<std::shared_ptr<const CObstacleInstance>> & obstacles, BattleSide side);

	void setActionMask(const std::shared_ptr<ActiveStackInfo> & astackinfo, const std::map<BattleHex, std::shared_ptr<Stack>> & hexstacks);
};
}
