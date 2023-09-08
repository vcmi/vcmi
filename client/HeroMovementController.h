/*
 * CPlayerInterface.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../lib/constants/EntityIdentifiers.h"
#include "../lib/int3.h"

VCMI_LIB_NAMESPACE_BEGIN
using TTeleportExitsList = std::vector<std::pair<ObjectInstanceID, int3>>;

class CGHeroInstance;
class CArmedInstance;

struct CGPath;
struct TryMoveHero;
VCMI_LIB_NAMESPACE_END

class HeroMovementController
{
	ObjectInstanceID destinationTeleport; //contain -1 or object id if teleportation
	int3 destinationTeleportPos;

	bool duringMovement;


	enum class EMoveState
	{
		STOP_MOVE,
		WAITING_MOVE,
		CONTINUE_MOVE,
		DURING_MOVE
	};

	EMoveState movementState;

	void setMovementStatus(bool value);
public:
	HeroMovementController();

	// const queries
	bool isHeroMovingThroughGarrison(const CGHeroInstance * hero, const CArmedInstance * garrison) const;
	bool isHeroMoving() const;

	// netpack handlers
	void onMoveHeroApplied();
	void onQueryReplyApplied();
	void onPlayerTurnStarted();
	void onBattleStarted();
	void showTeleportDialog(TeleportChannelID channel, TTeleportExitsList exits, bool impassable, QueryID askID);
	void heroMoved(const CGHeroInstance * hero, const TryMoveHero & details);

	// UI handlers
	void movementStartRequested(const CGHeroInstance * h, const CGPath & path);
	void movementStopRequested();
};
