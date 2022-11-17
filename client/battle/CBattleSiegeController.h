/*
 * CBattleObstacleController.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../../lib/GameConstants.h"

struct BattleObjectsByHex;
struct CatapultAttack;
struct SDL_Surface;
struct BattleHex;
struct Point;
class CGTownInstance;
class CBattleInterface;
class CCreature;

namespace EWallVisual
{
	enum EWallVisual
	{
		BACKGROUND = 0,
		BACKGROUND_WALL = 1,
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
		BACKGROUND_MOAT,
		KEEP_BATTLEMENT,
		BOTTOM_BATTLEMENT,
		UPPER_BATTLEMENT
	};
}

class CBattleSiegeController
{
	CBattleInterface * owner;

	SDL_Surface* walls[18];
	const CGTownInstance *town; //besieged town

	std::string getSiegeName(ui16 what) const;
	std::string getSiegeName(ui16 what, int state) const; // state uses EWallState enum

	void printPartOfWall(SDL_Surface *to, int what);

public:
	CBattleSiegeController(CBattleInterface * owner, const CGTownInstance *siegeTown);
	~CBattleSiegeController();

	void showPiecesOfWall(SDL_Surface *to, std::vector<int> pieces);

	const CCreature *turretCreature();
	Point turretCreaturePosition( BattleHex position );

	void gateStateChanged(const EGateState state);

	void showAbsoluteObstacles(SDL_Surface * to);
	bool isCatapultAttackable(BattleHex hex) const; //returns true if given tile can be attacked by catapult
	void stackIsCatapulting(const CatapultAttack & ca); //called when a stack is attacking walls

	void sortObjectsByHex(BattleObjectsByHex & sorted);
};
