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

#include "AsyncRunner.h"
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
	void onStackMoved(const CStack * cstack);
private:
	struct SpecialHexes
	{
		BattleHex corner1;
		BattleHex corner2;
		BattleHex corner1Wide;
		BattleHex corner2Wide;
		std::vector<BattleHex> tempHexes;
	};

	const std::shared_ptr<CBattleCallback> cb = nullptr;
	const std::shared_ptr<CPlayerBattleCallback> battle = nullptr;
	const BattleID & bid;
	const Settings settings;
	std::unique_ptr<AsyncRunner> asyncTasks;

	const CStack * movingStack = nullptr;
	std::mutex mutex;
	std::condition_variable cond;

	bool canHandle() const;
	void tacticMove(const CStack * stack, const BattleHex & bh);
	bool guardVip(const CStack * guard, const CStack * vip);
	void end();
	void handle();

	std::vector<const CStack *> findVIPs() const;
	std::vector<const CStack *> findGuards(const std::vector<const CStack*> & vips) const;
	std::vector<BattleHex> GuardableHexes(const CStack * vip, const CStack * guard);

	SpecialHexes getSpecialHexes() const;
	void moveGuardsAwayFromCorners(const std::vector<const CStack*> & guards, const SpecialHexes & specialHexes);
	void moveVipsToCorners(const std::vector<const CStack *> & vips, const SpecialHexes & specialHexes);
	void moveGuardsAroundVips(const std::vector<const CStack*> & guards, const std::vector<const CStack*> & vips);
};
