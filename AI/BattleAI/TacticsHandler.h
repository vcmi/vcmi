/*
 * Tactics.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "battle/CPlayerBattleCallback.h"
#include "callback/CBattleCallback.h"

class TacticsHandler {
public:
	struct Settings
	{
		bool enabled = true;
		double enemyAoeThreshold = 0.08;
		double vipThreshold = 0.05;
	};

	TacticsHandler(const std::shared_ptr<CBattleCallback> & cb, const BattleID & bid, Settings settings);
	void onTacticsStarted();
	void onActionFinished(const BattleAction & action);
private:
	struct SpecialHexes
	{
		BattleHex corner1;
		BattleHex corner2;
		BattleHex corner1Wide;
		BattleHex corner2Wide;
		std::vector<BattleHex> tempHexes;
	};

	enum class Phase : ui8
	{
		INACTIVE,
		MOVE_GUARDS_AWAY_FROM_CORNERS,
		MOVE_VIPS_TO_CORNERS,
		MOVE_GUARDS_AROUND_VIPS
	};

	const std::shared_ptr<CBattleCallback> cb = nullptr;
	const std::shared_ptr<CPlayerBattleCallback> battle = nullptr;
	const BattleID bid;
	const Settings settings;

	Phase phase = Phase::INACTIVE;
	const CStack * movingStack = nullptr;
	std::vector<const CStack *> vips;
	std::vector<const CStack *> guards;
	std::vector<const CStack *> vipsToMove;
	SpecialHexes specialHexes;
	std::size_t guardIndex = 0;
	std::size_t vipIndex = 0;
	int guardPass = 0;

	bool canHandle() const;
	void tacticMove(const CStack * stack, const BattleHex & bh);
	std::optional<BattleHex> findGuardDestination(const CStack * guard, const CStack * vip);
	void end();
	void handle();
	void advance();
	bool moveNextGuardAwayFromCorners();
	bool moveNextVipToCorner();
	bool moveNextGuardAroundVip();

	std::vector<const CStack *> findVIPs() const;
	std::vector<const CStack *> findGuards() const;
	std::vector<BattleHex> guardableHexes(const CStack * vip, const CStack * guard);

	SpecialHexes getSpecialHexes() const;
};
