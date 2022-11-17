/*
 * CBattleSiegeController.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "CBattleSiegeController.h"

#include "../CMusicHandler.h"
#include "../../lib/NetPacks.h"
#include "../CGameInfo.h"
#include "../../CCallback.h"
#include "../CBitmapHandler.h"
#include "../CPlayerInterface.h"
#include "../../lib/CStack.h"
#include "../../lib/mapObjects/CGTownInstance.h"
#include "CBattleInterface.h"
#include "CBattleAnimations.h"
#include "CBattleInterfaceClasses.h"

CBattleSiegeController::~CBattleSiegeController()
{
	auto gateState = owner->curInt->cb->battleGetGateState();
	for (int g = 0; g < ARRAY_COUNT(walls); ++g)
	{
		if (g != EWallVisual::GATE || (gateState != EGateState::NONE && gateState != EGateState::CLOSED && gateState != EGateState::BLOCKED))
			SDL_FreeSurface(walls[g]);
	}
}

std::string CBattleSiegeController::getSiegeName(ui16 what) const
{
	return getSiegeName(what, EWallState::INTACT);
}

std::string CBattleSiegeController::getSiegeName(ui16 what, int state) const
{
	auto getImageIndex = [&]() -> int
	{
		switch (state)
		{
		case EWallState::INTACT : return 1;
		case EWallState::DAMAGED :
			if(what == 2 || what == 3 || what == 8) // towers don't have separate image here - INTACT and DAMAGED is 1, DESTROYED is 2
				return 1;
			else
				return 2;
		case EWallState::DESTROYED :
			if (what == 2 || what == 3 || what == 8)
				return 2;
			else
				return 3;
		}
		return 1;
	};

	const std::string & prefix = town->town->clientInfo.siegePrefix;
	std::string addit = boost::lexical_cast<std::string>(getImageIndex());

	switch(what)
	{
	case EWallVisual::BACKGROUND:
		return prefix + "BACK.BMP";
	case EWallVisual::BACKGROUND_WALL:
		{
			switch(town->town->faction->index)
			{
			case ETownType::RAMPART:
			case ETownType::NECROPOLIS:
			case ETownType::DUNGEON:
			case ETownType::STRONGHOLD:
				return prefix + "TPW1.BMP";
			default:
				return prefix + "TPWL.BMP";
			}
		}
	case EWallVisual::KEEP:
		return prefix + "MAN" + addit + ".BMP";
	case EWallVisual::BOTTOM_TOWER:
		return prefix + "TW1" + addit + ".BMP";
	case EWallVisual::BOTTOM_WALL:
		return prefix + "WA1" + addit + ".BMP";
	case EWallVisual::WALL_BELLOW_GATE:
		return prefix + "WA3" + addit + ".BMP";
	case EWallVisual::WALL_OVER_GATE:
		return prefix + "WA4" + addit + ".BMP";
	case EWallVisual::UPPER_WALL:
		return prefix + "WA6" + addit + ".BMP";
	case EWallVisual::UPPER_TOWER:
		return prefix + "TW2" + addit + ".BMP";
	case EWallVisual::GATE:
		return prefix + "DRW" + addit + ".BMP";
	case EWallVisual::GATE_ARCH:
		return prefix + "ARCH.BMP";
	case EWallVisual::BOTTOM_STATIC_WALL:
		return prefix + "WA2.BMP";
	case EWallVisual::UPPER_STATIC_WALL:
		return prefix + "WA5.BMP";
	case EWallVisual::MOAT:
		return prefix + "MOAT.BMP";
	case EWallVisual::BACKGROUND_MOAT:
		return prefix + "MLIP.BMP";
	case EWallVisual::KEEP_BATTLEMENT:
		return prefix + "MANC.BMP";
	case EWallVisual::BOTTOM_BATTLEMENT:
		return prefix + "TW1C.BMP";
	case EWallVisual::UPPER_BATTLEMENT:
		return prefix + "TW2C.BMP";
	default:
		return "";
	}
}

void CBattleSiegeController::printPartOfWall(SDL_Surface *to, int what)
{
	Point pos = Point(-1, -1);
	auto & ci = town->town->clientInfo;

	if (vstd::iswithin(what, 1, 17))
	{
		pos.x = ci.siegePositions[what].x + owner->pos.x;
		pos.y = ci.siegePositions[what].y + owner->pos.y;
	}

	if (town->town->faction->index == ETownType::TOWER
		&& (what == EWallVisual::MOAT || what == EWallVisual::BACKGROUND_MOAT))
		return; // no moat in Tower. TODO: remove hardcode somehow?

	if (pos.x != -1)
	{
		//gate have no displayed bitmap when drawbridge is raised
		if (what == EWallVisual::GATE)
		{
			auto gateState = owner->curInt->cb->battleGetGateState();
			if (gateState != EGateState::OPENED && gateState != EGateState::DESTROYED)
				return;
		}

		blitAt(walls[what], pos.x, pos.y, to);
	}
}


CBattleSiegeController::CBattleSiegeController(CBattleInterface * owner, const CGTownInstance *siegeTown)
{
	owner->background = BitmapHandler::loadBitmap( getSiegeName(0), false );
	ui8 siegeLevel = owner->curInt->cb->battleGetSiegeLevel();
	if (siegeLevel >= 2) //citadel or castle
	{
		//print moat/mlip
		SDL_Surface *moat = BitmapHandler::loadBitmap( getSiegeName(13) ),
			* mlip = BitmapHandler::loadBitmap( getSiegeName(14) );

		auto & info = town->town->clientInfo;
		Point moatPos(info.siegePositions[13].x, info.siegePositions[13].y);
		Point mlipPos(info.siegePositions[14].x, info.siegePositions[14].y);

		if (moat) //eg. tower has no moat
			blitAt(moat, moatPos.x,moatPos.y, owner->background);
		if (mlip) //eg. tower has no mlip
			blitAt(mlip, mlipPos.x, mlipPos.y, owner->background);

		SDL_FreeSurface(moat);
		SDL_FreeSurface(mlip);
	}

	for (int g = 0; g < ARRAY_COUNT(walls); ++g)
	{
		if (g != EWallVisual::GATE)
			walls[g] = BitmapHandler::loadBitmap(getSiegeName(g));
	}
}

const CCreature *CBattleSiegeController::turretCreature()
{
	return CGI->creh->objects[town->town->clientInfo.siegeShooter];
}

Point CBattleSiegeController::turretCreaturePosition( BattleHex position )
{
	// Turret positions are read out of the config/wall_pos.txt
	int posID = 0;
	switch (position)
	{
	case BattleHex::CASTLE_CENTRAL_TOWER: // keep creature
		posID = 18;
		break;
	case BattleHex::CASTLE_BOTTOM_TOWER: // bottom creature
		posID = 19;
		break;
	case BattleHex::CASTLE_UPPER_TOWER: // upper creature
		posID = 20;
		break;
	}

	if (posID != 0)
	{
		Point result = owner->pos.topLeft();
		result.x += town->town->clientInfo.siegePositions[posID].x;
		result.y += town->town->clientInfo.siegePositions[posID].y;
		return result;
	}

	assert(0);
	return Point(0,0);
}


void CBattleSiegeController::gateStateChanged(const EGateState state)
{
	auto oldState = owner->curInt->cb->battleGetGateState();
	bool playSound = false;
	int stateId = EWallState::NONE;
	switch(state)
	{
	case EGateState::CLOSED:
		if (oldState != EGateState::BLOCKED)
			playSound = true;
		break;
	case EGateState::BLOCKED:
		if (oldState != EGateState::CLOSED)
			playSound = true;
		break;
	case EGateState::OPENED:
		playSound = true;
		stateId = EWallState::DAMAGED;
		break;
	case EGateState::DESTROYED:
		stateId = EWallState::DESTROYED;
		break;
	}

	if (oldState != EGateState::NONE && oldState != EGateState::CLOSED && oldState != EGateState::BLOCKED)
		SDL_FreeSurface(walls[EWallVisual::GATE]);

	if (stateId != EWallState::NONE)
		walls[EWallVisual::GATE] = BitmapHandler::loadBitmap(getSiegeName(EWallVisual::GATE, stateId));
	if (playSound)
		CCS->soundh->playSound(soundBase::DRAWBRG);
}

void CBattleSiegeController::showAbsoluteObstacles(SDL_Surface * to)
{
	if (town->hasBuilt(BuildingID::CITADEL))
		printPartOfWall(to, EWallVisual::BACKGROUND_MOAT);
}

void CBattleSiegeController::sortObjectsByHex(BattleObjectsByHex & sorted)
{

	sorted.beforeAll.walls.push_back(EWallVisual::BACKGROUND_WALL);
	sorted.hex[135].walls.push_back(EWallVisual::KEEP);
	sorted.afterAll.walls.push_back(EWallVisual::BOTTOM_TOWER);
	sorted.hex[182].walls.push_back(EWallVisual::BOTTOM_WALL);
	sorted.hex[130].walls.push_back(EWallVisual::WALL_BELLOW_GATE);
	sorted.hex[78].walls.push_back(EWallVisual::WALL_OVER_GATE);
	sorted.hex[12].walls.push_back(EWallVisual::UPPER_WALL);
	sorted.beforeAll.walls.push_back(EWallVisual::UPPER_TOWER);
	sorted.hex[94].walls.push_back(EWallVisual::GATE);
	sorted.hex[112].walls.push_back(EWallVisual::GATE_ARCH);
	sorted.hex[165].walls.push_back(EWallVisual::BOTTOM_STATIC_WALL);
	sorted.hex[45].walls.push_back(EWallVisual::UPPER_STATIC_WALL);

	if (town->hasBuilt(BuildingID::CITADEL))
	{
		sorted.beforeAll.walls.push_back(EWallVisual::MOAT);
		//sorted.beforeAll.walls.push_back(EWallVisual::BACKGROUND_MOAT); // blit as absolute obstacle
		sorted.hex[135].walls.push_back(EWallVisual::KEEP_BATTLEMENT);
	}
	if (town->hasBuilt(BuildingID::CASTLE))
	{
		sorted.afterAll.walls.push_back(EWallVisual::BOTTOM_BATTLEMENT);
		sorted.beforeAll.walls.push_back(EWallVisual::UPPER_BATTLEMENT);
	}
}


void CBattleSiegeController::showPiecesOfWall(SDL_Surface *to, std::vector<int> pieces)
{
	for (auto piece : pieces)
	{
		if (piece < 15) // not a tower - just print
			printPartOfWall(to, piece);
		else // tower. find if tower is built and not destroyed - stack is present
		{
			// PieceID    StackID
			// 15 = keep,  -2
			// 16 = lower, -3
			// 17 = upper, -4

			// tower. check if tower is alive - stack is found
			int stackPos = 13 - piece;

			const CStack *turret = nullptr;

			for (auto & stack : owner->curInt->cb->battleGetAllStacks(true))
			{
				if(stack->initialPosition == stackPos)
				{
					turret = stack;
					break;
				}
			}

			if (turret)
			{
				std::vector<const CStack *> stackList(1, turret);
				owner->showStacks(to, stackList);
				printPartOfWall(to, piece);
			}
		}
	}
}


bool CBattleSiegeController::isCatapultAttackable(BattleHex hex) const
{
	if (owner->tacticsMode)
		return false;

	auto wallPart = owner->curInt->cb->battleHexToWallPart(hex);
	if (!owner->curInt->cb->isWallPartPotentiallyAttackable(wallPart))
		return false;

	auto state = owner->curInt->cb->battleGetWallState(static_cast<int>(wallPart));
	return state != EWallState::DESTROYED && state != EWallState::NONE;
}

void CBattleSiegeController::stackIsCatapulting(const CatapultAttack & ca)
{
	if (ca.attacker != -1)
	{
		const CStack *stack = owner->curInt->cb->battleGetStackByID(ca.attacker);
		for (auto attackInfo : ca.attackedParts)
		{
			owner->addNewAnim(new CShootingAnimation(owner, stack, attackInfo.destinationTile, nullptr, true, attackInfo.damageDealt));
		}
	}
	else
	{
		//no attacker stack, assume spell-related (earthquake) - only hit animation
		for (auto attackInfo : ca.attackedParts)
		{
			Point destPos = CClickableHex::getXYUnitAnim(attackInfo.destinationTile, nullptr, owner) + Point(99, 120);

			owner->addNewAnim(new CEffectAnimation(owner, "SGEXPL.DEF", destPos.x, destPos.y));
		}
	}

	owner->waitForAnims();

	for (auto attackInfo : ca.attackedParts)
	{
		int wallId = attackInfo.attackedPart + 2;
		//gate state changing handled separately
		if (wallId == EWallVisual::GATE)
			continue;

		SDL_FreeSurface(walls[wallId]);
		walls[wallId] = BitmapHandler::loadBitmap(
			getSiegeName(wallId, owner->curInt->cb->battleGetWallState(attackInfo.attackedPart)));
	}
}
