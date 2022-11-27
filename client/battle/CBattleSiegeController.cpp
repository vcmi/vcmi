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

#include "CBattleAnimations.h"
#include "CBattleInterface.h"
#include "CBattleInterfaceClasses.h"
#include "CBattleStacksController.h"

#include "../CMusicHandler.h"
#include "../CGameInfo.h"
#include "../CPlayerInterface.h"
#include "../gui/CAnimation.h"
#include "../gui/CCanvas.h"

#include "../../CCallback.h"
#include "../../lib/NetPacks.h"
#include "../../lib/CStack.h"
#include "../../lib/mapObjects/CGTownInstance.h"

std::string CBattleSiegeController::getWallPieceImageName(EWallVisual::EWallVisual what, EWallState::EWallState state) const
{
	auto getImageIndex = [&]() -> int
	{
		switch (state)
		{
		case EWallState::INTACT :
			return 1;
		case EWallState::DAMAGED :
			// towers don't have separate image here - INTACT and DAMAGED is 1, DESTROYED is 2
			if(what == EWallVisual::KEEP || what == EWallVisual::BOTTOM_TOWER || what == EWallVisual::UPPER_TOWER)
				return 1;
			else
				return 2;
		case EWallState::DESTROYED :
			if (what == EWallVisual::KEEP || what == EWallVisual::BOTTOM_TOWER || what == EWallVisual::UPPER_TOWER)
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

void CBattleSiegeController::showWallPiece(std::shared_ptr<CCanvas> canvas, EWallVisual::EWallVisual what)
{
	auto & ci = town->town->clientInfo;
	auto const & pos = ci.siegePositions[what];

	canvas->draw(wallPieceImages[what], owner->pos.topLeft() + Point(pos.x, pos.y));
}

std::string CBattleSiegeController::getBattleBackgroundName() const
{
	const std::string & prefix = town->town->clientInfo.siegePrefix;
	return prefix + "BACK.BMP";
}

bool CBattleSiegeController::getWallPieceExistance(EWallVisual::EWallVisual what) const
{
	//FIXME: use this instead of buildings test?
	//ui8 siegeLevel = owner->curInt->cb->battleGetSiegeLevel();

	bool isMoat = (what == EWallVisual::BACKGROUND_MOAT || what == EWallVisual::MOAT);
	bool isKeep = what == EWallVisual::KEEP_BATTLEMENT;
	bool isTower = (what == EWallVisual::UPPER_BATTLEMENT || what == EWallVisual::BOTTOM_BATTLEMENT);

	bool hasMoat = town->hasBuilt(BuildingID::CITADEL) && town->town->faction->index != ETownType::TOWER;
	bool hasKeep = town->hasBuilt(BuildingID::CITADEL);
	bool hasTower = town->hasBuilt(BuildingID::CASTLE);

	if ( isMoat ) return hasMoat;
	if ( isKeep ) return hasKeep;
	if ( isTower ) return hasTower;

	return true;
}

BattleHex CBattleSiegeController::getWallPiecePosition(EWallVisual::EWallVisual what) const
{
	static const std::array<BattleHex, 18> wallsPositions = {
		BattleHex::INVALID, // background, handled separately
		BattleHex::HEX_BEFORE_ALL,
		135,
		BattleHex::HEX_AFTER_ALL,
		182,
		130,
		78,
		12,
		BattleHex::HEX_BEFORE_ALL,
		94,
		112,
		165,
		45,
		BattleHex::INVALID, //moat, printed as obstacle // BattleHex::HEX_BEFORE_ALL,
		BattleHex::INVALID, //moat, printed as obstacle
		135,
		BattleHex::HEX_AFTER_ALL,
		BattleHex::HEX_BEFORE_ALL
	};

	return wallsPositions[what];
}

CBattleSiegeController::CBattleSiegeController(CBattleInterface * owner, const CGTownInstance *siegeTown):
	owner(owner),
	town(siegeTown)
{
	assert(owner->fieldController.get() == nullptr); // must be created after this

	for (int g = 0; g < wallPieceImages.size(); ++g)
	{
		if ( g == EWallVisual::GATE ) // gate is initially closed and has no image to display in this state
			continue;

		if ( !getWallPieceExistance(EWallVisual::EWallVisual(g)) )
			continue;

		wallPieceImages[g] = IImage::createFromFile(getWallPieceImageName(EWallVisual::EWallVisual(g), EWallState::INTACT));
	}
}

const CCreature *CBattleSiegeController::getTurretCreature() const
{
	return CGI->creh->objects[town->town->clientInfo.siegeShooter];
}

Point CBattleSiegeController::getTurretCreaturePosition( BattleHex position ) const
{
	// Turret positions are read out of the config/wall_pos.txt
	int posID = 0;
	switch (position)
	{
	case BattleHex::CASTLE_CENTRAL_TOWER: // keep creature
		posID = EWallVisual::CREATURE_KEEP;
		break;
	case BattleHex::CASTLE_BOTTOM_TOWER: // bottom creature
		posID = EWallVisual::CREATURE_BOTTOM_TOWER;
		break;
	case BattleHex::CASTLE_UPPER_TOWER: // upper creature
		posID = EWallVisual::CREATURE_UPPER_TOWER;
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
	auto stateId = EWallState::NONE;
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
		wallPieceImages[EWallVisual::GATE] = nullptr;

	if (stateId != EWallState::NONE)
		wallPieceImages[EWallVisual::GATE] = IImage::createFromFile(getWallPieceImageName(EWallVisual::GATE,  stateId));

	if (playSound)
		CCS->soundh->playSound(soundBase::DRAWBRG);
}

void CBattleSiegeController::showAbsoluteObstacles(std::shared_ptr<CCanvas> canvas)
{
	if (getWallPieceExistance(EWallVisual::MOAT))
		showWallPiece(canvas, EWallVisual::MOAT);

	if (getWallPieceExistance(EWallVisual::BACKGROUND_MOAT))
		showWallPiece(canvas, EWallVisual::BACKGROUND_MOAT);
}

void CBattleSiegeController::showBattlefieldObjects(std::shared_ptr<CCanvas> canvas, const BattleHex & location )
{
	for (int i = EWallVisual::WALL_FIRST; i <= EWallVisual::WALL_LAST; ++i)
	{
		auto wallPiece = EWallVisual::EWallVisual(i);

		if ( !getWallPieceExistance(wallPiece))
			continue;

		if ( getWallPiecePosition(wallPiece) != location)
			continue;

		if (wallPiece != EWallVisual::KEEP_BATTLEMENT &&
			wallPiece != EWallVisual::BOTTOM_BATTLEMENT &&
			wallPiece != EWallVisual::UPPER_BATTLEMENT)
		{
			showWallPiece(canvas, wallPiece);
			continue;
		}

		// tower. check if tower is alive - stack is found
		BattleHex stackPos;
		switch(wallPiece)
		{
		case EWallVisual::KEEP_BATTLEMENT:
			stackPos = BattleHex::CASTLE_CENTRAL_TOWER;
			break;
		case EWallVisual::BOTTOM_BATTLEMENT:
			stackPos = BattleHex::CASTLE_BOTTOM_TOWER;
			break;
		case EWallVisual::UPPER_BATTLEMENT:
			stackPos = BattleHex::CASTLE_UPPER_TOWER;
			break;
		}

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
			owner->stacksController->showStack(canvas, turret);
			showWallPiece(canvas, wallPiece);
		}
	}
}

bool CBattleSiegeController::isAttackableByCatapult(BattleHex hex) const
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
			owner->stacksController->addNewAnim(new CShootingAnimation(owner, stack, attackInfo.destinationTile, nullptr, true, attackInfo.damageDealt));
		}
	}
	else
	{
		//no attacker stack, assume spell-related (earthquake) - only hit animation
		for (auto attackInfo : ca.attackedParts)
		{
			Point destPos = owner->stacksController->getStackPositionAtHex(attackInfo.destinationTile, nullptr) + Point(99, 120);

			owner->stacksController->addNewAnim(new CEffectAnimation(owner, "SGEXPL.DEF", destPos.x, destPos.y));
		}
	}

	owner->waitForAnims();

	for (auto attackInfo : ca.attackedParts)
	{
		int wallId = attackInfo.attackedPart + EWallVisual::DESTRUCTIBLE_FIRST;
		//gate state changing handled separately
		if (wallId == EWallVisual::GATE)
			continue;

		auto wallState = EWallState::EWallState(owner->curInt->cb->battleGetWallState(attackInfo.attackedPart));

		wallPieceImages[wallId] = IImage::createFromFile(getWallPieceImageName(EWallVisual::EWallVisual(wallId), wallState));
	}
}
