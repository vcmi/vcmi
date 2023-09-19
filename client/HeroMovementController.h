/*
 * HeroMovementController.h, part of VCMI engine
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
#include "../lib/filesystem/ResourcePath.h"

VCMI_LIB_NAMESPACE_BEGIN
using TTeleportExitsList = std::vector<std::pair<ObjectInstanceID, int3>>;

class CGHeroInstance;
class CArmedInstance;
struct CGPathNode;

struct CGPath;
struct TryMoveHero;
enum class EPathNodeAction : ui8;
VCMI_LIB_NAMESPACE_END

class HeroMovementController
{
	/// there is an ongoing movement loop, in one or another stage
	bool duringMovement = false;
	/// movement was requested to be terminated, e.g. by player or due to inability to move
	bool stoppingMovement = false;

	bool waitingForQueryApplyReply = false;

	const CGHeroInstance * currentlyMovingHero = nullptr;
	AudioPath currentMovementSoundName;
	int currentMovementSoundChannel = -1;

	bool canHeroStopAtNode(const CGPathNode & node) const;

	void updatePath(const CGHeroInstance * hero, const TryMoveHero & details);

	/// Moves hero 1 tile / path node
	void moveOnce(const CGHeroInstance * h, const CGPath & path);

	void endMove(const CGHeroInstance * h);

	AudioPath getMovementSoundFor(const CGHeroInstance * hero, int3 posPrev, int3 posNext, EPathNodeAction moveType);
	void updateMovementSound(const CGHeroInstance * hero, int3 posPrev, int3 posNext, EPathNodeAction moveType);
	void stopMovementSound();

public:
	// const queries

	/// Returns true if hero should move through garrison without displaying garrison dialog
	bool isHeroMovingThroughGarrison(const CGHeroInstance * hero, const CArmedInstance * garrison) const;

	/// Returns true if there is an ongoing hero movement process
	bool isHeroMoving() const;

	// netpack handlers
	void onMoveHeroApplied();
	void onQueryReplyApplied();
	void onPlayerTurnStarted();
	void onBattleStarted();
	void showTeleportDialog(const CGHeroInstance * hero, TeleportChannelID channel, TTeleportExitsList exits, bool impassable, QueryID askID);
	void onTryMoveHero(const CGHeroInstance * hero, const TryMoveHero & details);

	// UI handlers
	void requestMovementStart(const CGHeroInstance * h, const CGPath & path);
	void requestMovementAbort();
};
