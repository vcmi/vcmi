/*
 * BattleObstacleController.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../../lib/GameConstants.h"
#include "../../lib/battle/BattleHex.h"
#include "../../lib/filesystem/ResourcePath.h"

struct CatapultAttack;
class CCreature;
class CStack;
class CGTownInstance;
class Point;

class Canvas;
class BattleInterface;
class BattleRenderer;
class IImage;

namespace EWallVisual
{
	enum EWallVisual
	{
		BACKGROUND,
		BACKGROUND_WALL,

		KEEP,
		BOTTOM_TOWER,
		BOTTOM_WALL,
		WALL_BELLOW_GATE,
		WALL_OVER_GATE,
		UPPER_WALL,
		UPPER_TOWER,
		GATE,

		GATE_ARCH,
		BOTTOM_STATIC_WALL,
		UPPER_STATIC_WALL,
		MOAT,
		MOAT_BANK,
		KEEP_BATTLEMENT,
		BOTTOM_BATTLEMENT,
		UPPER_BATTLEMENT,

		CREATURE_KEEP,
		CREATURE_BOTTOM_TOWER,
		CREATURE_UPPER_TOWER,

		WALL_FIRST = BACKGROUND_WALL,
		WALL_LAST  = UPPER_BATTLEMENT,

		// these entries are mapped to EWallPart enum
		DESTRUCTIBLE_FIRST = KEEP,
		DESTRUCTIBLE_LAST = GATE,
	};
}

class BattleSiegeController
{
	BattleInterface & owner;

	/// besieged town
	const CGTownInstance *town;

	/// sections of castle walls, in their currently visible state
	std::array<std::shared_ptr<IImage>, EWallVisual::WALL_LAST + 1> wallPieceImages;

	/// drawbridge front overlay (chains), drawn over units standing on the lowered bridge
	std::shared_ptr<IImage> gateFrontImage;

	/// return URI for image for a wall piece
	ImagePath getWallPieceImageName(EWallVisual::EWallVisual what, EWallState state) const;

	/// returns BattleHex to which chosen wall piece is bound
	BattleHex getWallPiecePosition(EWallVisual::EWallVisual what) const;

	/// returns true if chosen wall piece should be present in current battle
	bool getWallPieceExistence(EWallVisual::EWallVisual what) const;

	void showWallPiece(Canvas & canvas, EWallVisual::EWallVisual what);

	BattleHex getTurretBattleHex(EWallVisual::EWallVisual wallPiece) const;
	const CStack * getTurretStack(EWallVisual::EWallVisual wallPiece) const;

	const std::string & getSiegePrefix() const;

public:
	BattleSiegeController(BattleInterface & owner, const CGTownInstance *siegeTown);

	/// call-ins from server
	void gateStateChanged(const EGateState state);

	/// plays the drawbridge sound and shows the partially-open frame; start of a lower/raise transition
	void showPartialGate();

	/// swaps the gate to its final sprite for the given state; called by the bridge transition animation
	void applyGateState(const EGateState state);
	void stackIsCatapulting(const CatapultAttack & ca);

	/// call-ins from other battle controllers
	void showAbsoluteObstacles(Canvas & canvas);
	void collectRenderableObjects(BattleRenderer & renderer);

	/// queries from other battle controllers
	bool isAttackableByCatapult(const BattleHex & hex) const;
	/// True if the given battle hex belongs to a siege tower (keep / upper / lower)
	bool isTowerHex(const BattleHex & hex) const;
	/// Right-click popup text describing the status (or attack/damage) of every siege tower present
	std::string getTowersInfoText() const;
	ImagePath getBattleBackgroundName() const;
	const CCreature *getTurretCreature(const BattleHex & turretPosition) const;
	Point getTurretCreaturePosition( BattleHex position ) const;

	const CGTownInstance *getSiegedTown() const;
};
