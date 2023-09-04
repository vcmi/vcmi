/*
 * BattleSiegeController.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "BattleSiegeController.h"

#include "BattleAnimationClasses.h"
#include "BattleInterface.h"
#include "BattleInterfaceClasses.h"
#include "BattleStacksController.h"
#include "BattleFieldController.h"
#include "BattleRenderer.h"

#include "../CMusicHandler.h"
#include "../CGameInfo.h"
#include "../CPlayerInterface.h"
#include "../gui/CGuiHandler.h"
#include "../render/Canvas.h"
#include "../render/IImage.h"
#include "../render/IRenderHandler.h"

#include "../../CCallback.h"
#include "../../lib/NetPacks.h"
#include "../../lib/CStack.h"
#include "../../lib/mapObjects/CGTownInstance.h"

ImagePath BattleSiegeController::getWallPieceImageName(EWallVisual::EWallVisual what, EWallState state) const
{
	auto getImageIndex = [&]() -> int
	{
		bool isTower = (what == EWallVisual::KEEP || what == EWallVisual::BOTTOM_TOWER || what == EWallVisual::UPPER_TOWER);

		switch (state)
		{
		case EWallState::REINFORCED :
			return 1;
		case EWallState::INTACT :
			if (town->hasBuilt(BuildingID::CASTLE))
				return 2; // reinforced walls were damaged
			else
				return 1;
		case EWallState::DAMAGED :
			// towers don't have separate image here - INTACT and DAMAGED is 1, DESTROYED is 2
			if (isTower)
				return 1;
			else
				return 2;
		case EWallState::DESTROYED :
			if (isTower)
				return 2;
			else
				return 3;
		}
		return 1;
	};

	const std::string & prefix = town->town->clientInfo.siegePrefix;
	std::string addit = std::to_string(getImageIndex());

	switch(what)
	{
	case EWallVisual::BACKGROUND_WALL:
		{
			auto faction = town->town->faction->getIndex();

			if (faction == ETownType::RAMPART || faction == ETownType::NECROPOLIS || faction == ETownType::DUNGEON || faction == ETownType::STRONGHOLD)
				return ImagePath::builtinTODO(prefix + "TPW1.BMP");
			else
				return ImagePath::builtinTODO(prefix + "TPWL.BMP");
		}
	case EWallVisual::KEEP:
		return ImagePath::builtinTODO(prefix + "MAN" + addit + ".BMP");
	case EWallVisual::BOTTOM_TOWER:
		return ImagePath::builtinTODO(prefix + "TW1" + addit + ".BMP");
	case EWallVisual::BOTTOM_WALL:
		return ImagePath::builtinTODO(prefix + "WA1" + addit + ".BMP");
	case EWallVisual::WALL_BELLOW_GATE:
		return ImagePath::builtinTODO(prefix + "WA3" + addit + ".BMP");
	case EWallVisual::WALL_OVER_GATE:
		return ImagePath::builtinTODO(prefix + "WA4" + addit + ".BMP");
	case EWallVisual::UPPER_WALL:
		return ImagePath::builtinTODO(prefix + "WA6" + addit + ".BMP");
	case EWallVisual::UPPER_TOWER:
		return ImagePath::builtinTODO(prefix + "TW2" + addit + ".BMP");
	case EWallVisual::GATE:
		return ImagePath::builtinTODO(prefix + "DRW" + addit + ".BMP");
	case EWallVisual::GATE_ARCH:
		return ImagePath::builtinTODO(prefix + "ARCH.BMP");
	case EWallVisual::BOTTOM_STATIC_WALL:
		return ImagePath::builtinTODO(prefix + "WA2.BMP");
	case EWallVisual::UPPER_STATIC_WALL:
		return ImagePath::builtinTODO(prefix + "WA5.BMP");
	case EWallVisual::MOAT:
		return ImagePath::builtinTODO(prefix + "MOAT.BMP");
	case EWallVisual::MOAT_BANK:
		return ImagePath::builtinTODO(prefix + "MLIP.BMP");
	case EWallVisual::KEEP_BATTLEMENT:
		return ImagePath::builtinTODO(prefix + "MANC.BMP");
	case EWallVisual::BOTTOM_BATTLEMENT:
		return ImagePath::builtinTODO(prefix + "TW1C.BMP");
	case EWallVisual::UPPER_BATTLEMENT:
		return ImagePath::builtinTODO(prefix + "TW2C.BMP");
	default:
		return ImagePath();
	}
}

void BattleSiegeController::showWallPiece(Canvas & canvas, EWallVisual::EWallVisual what)
{
	auto & ci = town->town->clientInfo;
	auto const & pos = ci.siegePositions[what];

	if ( wallPieceImages[what] && pos.isValid())
		canvas.draw(wallPieceImages[what], Point(pos.x, pos.y));
}

ImagePath BattleSiegeController::getBattleBackgroundName() const
{
	const std::string & prefix = town->town->clientInfo.siegePrefix;
	return ImagePath::builtinTODO(prefix + "BACK.BMP");
}

bool BattleSiegeController::getWallPieceExistance(EWallVisual::EWallVisual what) const
{
	//FIXME: use this instead of buildings test?
	//ui8 siegeLevel = owner.curInt->cb->battleGetSiegeLevel();

	switch (what)
	{
	case EWallVisual::MOAT:              return town->hasBuilt(BuildingID::CITADEL) && town->town->clientInfo.siegePositions.at(EWallVisual::MOAT).isValid();
	case EWallVisual::MOAT_BANK:         return town->hasBuilt(BuildingID::CITADEL) && town->town->clientInfo.siegePositions.at(EWallVisual::MOAT_BANK).isValid();
	case EWallVisual::KEEP_BATTLEMENT:   return town->hasBuilt(BuildingID::CITADEL) && owner.curInt->cb->battleGetWallState(EWallPart::KEEP) != EWallState::DESTROYED;
	case EWallVisual::UPPER_BATTLEMENT:  return town->hasBuilt(BuildingID::CASTLE) && owner.curInt->cb->battleGetWallState(EWallPart::UPPER_TOWER) != EWallState::DESTROYED;
	case EWallVisual::BOTTOM_BATTLEMENT: return town->hasBuilt(BuildingID::CASTLE) && owner.curInt->cb->battleGetWallState(EWallPart::BOTTOM_TOWER) != EWallState::DESTROYED;
	default:                             return true;
	}
}

BattleHex BattleSiegeController::getWallPiecePosition(EWallVisual::EWallVisual what) const
{
	static const std::array<BattleHex, 18> wallsPositions = {
		BattleHex::INVALID,        // BACKGROUND,         // handled separately
		BattleHex::HEX_BEFORE_ALL, // BACKGROUND_WALL,
		135,                       // KEEP,
		BattleHex::HEX_AFTER_ALL,  // BOTTOM_TOWER,
		182,                       // BOTTOM_WALL,
		130,                       // WALL_BELLOW_GATE,
		62,                        // WALL_OVER_GATE,
		12,                        // UPPER_WALL,
		BattleHex::HEX_BEFORE_ALL, // UPPER_TOWER,
		BattleHex::HEX_BEFORE_ALL, // GATE,               // 94
		112,                       // GATE_ARCH,
		165,                       // BOTTOM_STATIC_WALL,
		45,                        // UPPER_STATIC_WALL,
		BattleHex::INVALID,        // MOAT,               // printed as absolute obstacle
		BattleHex::INVALID,        // MOAT_BANK,          // printed as absolute obstacle
		135,                       // KEEP_BATTLEMENT,
		BattleHex::HEX_AFTER_ALL,  // BOTTOM_BATTLEMENT,
		BattleHex::HEX_BEFORE_ALL, // UPPER_BATTLEMENT,
	};

	return wallsPositions[what];
}

BattleSiegeController::BattleSiegeController(BattleInterface & owner, const CGTownInstance *siegeTown):
	owner(owner),
	town(siegeTown)
{
	assert(owner.fieldController.get() == nullptr); // must be created after this

	for (int g = 0; g < wallPieceImages.size(); ++g)
	{
		if ( g == EWallVisual::GATE ) // gate is initially closed and has no image to display in this state
			continue;

		if ( !getWallPieceExistance(EWallVisual::EWallVisual(g)) )
			continue;

		wallPieceImages[g] = GH.renderHandler().loadImage(getWallPieceImageName(EWallVisual::EWallVisual(g), EWallState::REINFORCED));
	}
}

const CCreature *BattleSiegeController::getTurretCreature() const
{
	return CGI->creh->objects[town->town->clientInfo.siegeShooter];
}

Point BattleSiegeController::getTurretCreaturePosition( BattleHex position ) const
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
		return {
			town->town->clientInfo.siegePositions[posID].x,
			town->town->clientInfo.siegePositions[posID].y
		};
	}

	assert(0);
	return Point(0,0);
}

void BattleSiegeController::gateStateChanged(const EGateState state)
{
	auto oldState = owner.curInt->cb->battleGetGateState();
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
		wallPieceImages[EWallVisual::GATE] = GH.renderHandler().loadImage(getWallPieceImageName(EWallVisual::GATE,  stateId));

	if (playSound)
		CCS->soundh->playSound(soundBase::DRAWBRG);
}

void BattleSiegeController::showAbsoluteObstacles(Canvas & canvas)
{
	if (getWallPieceExistance(EWallVisual::MOAT))
		showWallPiece(canvas, EWallVisual::MOAT);

	if (getWallPieceExistance(EWallVisual::MOAT_BANK))
		showWallPiece(canvas, EWallVisual::MOAT_BANK);
}

BattleHex BattleSiegeController::getTurretBattleHex(EWallVisual::EWallVisual wallPiece) const
{
	switch(wallPiece)
	{
	case EWallVisual::KEEP_BATTLEMENT:   return BattleHex::CASTLE_CENTRAL_TOWER;
	case EWallVisual::BOTTOM_BATTLEMENT: return BattleHex::CASTLE_BOTTOM_TOWER;
	case EWallVisual::UPPER_BATTLEMENT:  return BattleHex::CASTLE_UPPER_TOWER;
	}
	assert(0);
	return BattleHex::INVALID;
}

const CStack * BattleSiegeController::getTurretStack(EWallVisual::EWallVisual wallPiece) const
{
	for (auto & stack : owner.curInt->cb->battleGetAllStacks(true))
	{
		if ( stack->initialPosition == getTurretBattleHex(wallPiece))
			return stack;
	}
	assert(0);
	return nullptr;
}

void BattleSiegeController::collectRenderableObjects(BattleRenderer & renderer)
{
	for (int i = EWallVisual::WALL_FIRST; i <= EWallVisual::WALL_LAST; ++i)
	{
		auto wallPiece = EWallVisual::EWallVisual(i);

		if ( !getWallPieceExistance(wallPiece))
			continue;

		if ( getWallPiecePosition(wallPiece) == BattleHex::INVALID)
			continue;

		if (wallPiece == EWallVisual::KEEP_BATTLEMENT ||
			wallPiece == EWallVisual::BOTTOM_BATTLEMENT ||
			wallPiece == EWallVisual::UPPER_BATTLEMENT)
		{
			renderer.insert( EBattleFieldLayer::STACKS, getWallPiecePosition(wallPiece), [this, wallPiece](BattleRenderer::RendererRef canvas){
				owner.stacksController->showStack(canvas, getTurretStack(wallPiece));
			});
			renderer.insert( EBattleFieldLayer::OBSTACLES, getWallPiecePosition(wallPiece), [this, wallPiece](BattleRenderer::RendererRef canvas){
				showWallPiece(canvas, wallPiece);
			});
		}
		renderer.insert( EBattleFieldLayer::WALLS, getWallPiecePosition(wallPiece), [this, wallPiece](BattleRenderer::RendererRef canvas){
			showWallPiece(canvas, wallPiece);
		});
	}
}

bool BattleSiegeController::isAttackableByCatapult(BattleHex hex) const
{
	if (owner.tacticsMode)
		return false;

	auto wallPart = owner.curInt->cb->battleHexToWallPart(hex);
	return owner.curInt->cb->isWallPartAttackable(wallPart);
}

void BattleSiegeController::stackIsCatapulting(const CatapultAttack & ca)
{
	if (ca.attacker != -1)
	{
		const CStack *stack = owner.curInt->cb->battleGetStackByID(ca.attacker);
		for (auto attackInfo : ca.attackedParts)
		{
			owner.stacksController->addNewAnim(new CatapultAnimation(owner, stack, attackInfo.destinationTile, nullptr, attackInfo.damageDealt));
		}
	}
	else
	{
		std::vector<Point> positions;

		//no attacker stack, assume spell-related (earthquake) - only hit animation
		for (auto attackInfo : ca.attackedParts)
			positions.push_back(owner.stacksController->getStackPositionAtHex(attackInfo.destinationTile, nullptr) + Point(99, 120));

		CCS->soundh->playSound( AudioPath::builtin("WALLHIT") );
		owner.stacksController->addNewAnim(new EffectAnimation(owner, AnimationPath::builtin("SGEXPL.DEF"), positions));
	}

	owner.waitForAnimations();

	for (auto attackInfo : ca.attackedParts)
	{
		int wallId = static_cast<int>(attackInfo.attackedPart) + EWallVisual::DESTRUCTIBLE_FIRST;
		//gate state changing handled separately
		if (wallId == EWallVisual::GATE)
			continue;

		auto wallState = EWallState(owner.curInt->cb->battleGetWallState(attackInfo.attackedPart));

		wallPieceImages[wallId] = GH.renderHandler().loadImage(getWallPieceImageName(EWallVisual::EWallVisual(wallId), wallState));
	}
}

const CGTownInstance *BattleSiegeController::getSiegedTown() const
{
	return town;
}
