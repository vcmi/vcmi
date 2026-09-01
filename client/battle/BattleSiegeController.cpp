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
#include "BattleFieldController.h"
#include "BattleInterface.h"
#include "BattleRenderer.h"
#include "BattleStacksController.h"

#include "../CPlayerInterface.h"
#include "../GameEngine.h"
#include "../media/ISoundPlayer.h"
#include "../render/Canvas.h"
#include "../render/IImage.h"
#include "../render/IRenderHandler.h"

#include "../../lib/CStack.h"
#include "../../lib/GameLibrary.h"
#include "../../lib/battle/CPlayerBattleCallback.h"
#include "../../lib/battle/IBattleInfoCallback.h"
#include "../../lib/entities/building/TownFortifications.h"
#include "../../lib/filesystem/Filesystem.h"
#include "../../lib/texts/CGeneralTextHandler.h"
#include "../../lib/mapping/CMapHeader.h"
#include "../../lib/mapObjects/CGTownInstance.h"
#include "../../lib/networkPacks/PacksForClientBattle.h"
#include "../../lib/callback/CCallback.h"
#include "../GameInstance.h"

const std::string & BattleSiegeController::getSiegePrefix() const
{
	const auto & siegePrefixes = town->getTown()->clientInfo.siegePrefix;
	const auto & currentLayer = owner.curInt->cb->getMapHeader()->mapLayers.at(town->pos.z);
	if(siegePrefixes.count(currentLayer))
		return siegePrefixes.at(currentLayer);
	else
	{
		logGlobal->warn("No siege prefix for town %s for layer %s found, fallback", town->getObjectName().toString(&GAME->translator()), MapLayerId::encode(currentLayer));
		if(siegePrefixes.count(MapLayerId::UNKNOWN))
			return siegePrefixes.at(MapLayerId::UNKNOWN);
		if(siegePrefixes.count(MapLayerId::SURFACE))
			return siegePrefixes.at(MapLayerId::SURFACE);
		return siegePrefixes.begin()->second;
	}
}

ImagePath BattleSiegeController::getWallPieceImageName(EWallVisual::EWallVisual what, EWallState state) const
{
	auto getImageIndex = [&]() -> int
	{
		int health = static_cast<int>(state);

		switch (what)
		{
			case EWallVisual::KEEP:
			case EWallVisual::BOTTOM_TOWER:
			case EWallVisual::UPPER_TOWER:
				if (health > 0)
					return 1;
				else
					return 2;
			default:
			{
				int healthTotal = town->fortificationsLevel().wallsHealth;
				if (healthTotal == health)
					return 1;
				if (health > 0)
					return 2;
				return 3;
			}
		};
	};

	const std::string & prefix = getSiegePrefix();
	std::string addit = std::to_string(getImageIndex());

	switch(what)
	{
	case EWallVisual::BACKGROUND_WALL:
		{
			auto faction = town->getFactionID();

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
	auto & ci = town->getTown()->clientInfo;
	auto const & pos = ci.siegePositions[what];

	if ( wallPieceImages[what] && pos.isValid())
		canvas.draw(wallPieceImages[what], Point(pos.x, pos.y));
}

ImagePath BattleSiegeController::getBattleBackgroundName() const
{
	const std::string & prefix = getSiegePrefix();
	return ImagePath::builtinTODO(prefix + "BACK.BMP");
}

bool BattleSiegeController::getWallPieceExistence(EWallVisual::EWallVisual what) const
{
	const auto & fortifications = town->fortificationsLevel();

	switch (what)
	{
	case EWallVisual::MOAT:              return fortifications.hasMoat && town->getTown()->clientInfo.siegePositions.at(EWallVisual::MOAT).isValid();
	case EWallVisual::MOAT_BANK:         return fortifications.hasMoat && town->getTown()->clientInfo.siegePositions.at(EWallVisual::MOAT_BANK).isValid();
	case EWallVisual::KEEP_BATTLEMENT:   return fortifications.citadelHealth > 0 && owner.getBattle()->battleGetWallState(EWallPart::KEEP) != EWallState::DESTROYED;
	case EWallVisual::UPPER_BATTLEMENT:  return fortifications.upperTowerHealth > 0 && owner.getBattle()->battleGetWallState(EWallPart::UPPER_TOWER) != EWallState::DESTROYED;
	case EWallVisual::BOTTOM_BATTLEMENT: return fortifications.lowerTowerHealth > 0 && owner.getBattle()->battleGetWallState(EWallPart::BOTTOM_TOWER) != EWallState::DESTROYED;
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

		if ( !getWallPieceExistence(EWallVisual::EWallVisual(g)) )
			continue;

		auto fullState = static_cast<EWallState>(town->fortificationsLevel().wallsHealth);
		wallPieceImages[g] = ENGINE->renderHandler().loadImage(getWallPieceImageName(EWallVisual::EWallVisual(g), fullState), EImageBlitMode::COLORKEY);
	}

	// optional: not every town ships the drawbridge front overlay
	auto gateFrontPath = ImagePath::builtinTODO(getSiegePrefix() + "DRWC.BMP");
	if (town->fortificationsLevel().wallsHealth > 0 && CResourceHandler::get()->existsResource(gateFrontPath))
		gateFrontImage = ENGINE->renderHandler().loadImage(gateFrontPath, EImageBlitMode::COLORKEY);
}

const CCreature *BattleSiegeController::getTurretCreature(const BattleHex & position) const
{
	switch (position.toInt())
	{
		case BattleHex::CASTLE_CENTRAL_TOWER:
			return town->fortificationsLevel().citadelShooter.toCreature();
		case BattleHex::CASTLE_UPPER_TOWER:
			return town->fortificationsLevel().upperTowerShooter.toCreature();
		case BattleHex::CASTLE_BOTTOM_TOWER:
			return town->fortificationsLevel().lowerTowerShooter.toCreature();
	}

	throw std::runtime_error("Unable to select shooter for tower at " + std::to_string(position.toInt()));
}

Point BattleSiegeController::getTurretCreaturePosition( BattleHex position ) const
{
	// Turret positions are read out of the config/wall_pos.txt
	int posID = 0;
	switch (position.toInt())
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
			town->getTown()->clientInfo.siegePositions[posID].x,
			town->getTown()->clientInfo.siegePositions[posID].y
		};
	}

	assert(0);
	return Point(0,0);
}

void BattleSiegeController::gateStateChanged(const EGateState state)
{
	auto oldState = owner.getBattle()->battleGetGateState();

	bool wasClosed = oldState == EGateState::CLOSED || oldState == EGateState::BLOCKED;
	bool isClosed  = state == EGateState::CLOSED || state == EGateState::BLOCKED;

	// only lowering and raising animate through the partial frame; destruction and blocked<->closed apply at once
	bool animate = (state == EGateState::OPENED && wasClosed) || (isClosed && oldState == EGateState::OPENED);

	if (animate)
	{
		// block until the bridge finishes lowering/raising, so a unit crossing does not move through it mid-transition
		owner.stacksController->addNewAnim(new GateAnimation(owner, state));
		owner.waitForAnimations();
	}
	else
		applyGateState(state);
}

void BattleSiegeController::showPartialGate()
{
	ENGINE->sound().playSound(soundBase::DRAWBRG);
	auto partialState = static_cast<EWallState>(town->fortificationsLevel().wallsHealth);
	wallPieceImages[EWallVisual::GATE] = ENGINE->renderHandler().loadImage(getWallPieceImageName(EWallVisual::GATE, partialState), EImageBlitMode::COLORKEY);
}

void BattleSiegeController::applyGateState(const EGateState state)
{
	auto stateId = EWallState::NONE;
	if (state == EGateState::OPENED)
		stateId = EWallState::DAMAGED;
	else if (state == EGateState::DESTROYED)
		stateId = EWallState::DESTROYED;

	if (stateId == EWallState::NONE)
		wallPieceImages[EWallVisual::GATE] = nullptr; // closed / blocked -> gate hidden, part of the static wall
	else
		wallPieceImages[EWallVisual::GATE] = ENGINE->renderHandler().loadImage(getWallPieceImageName(EWallVisual::GATE, stateId), EImageBlitMode::COLORKEY);
}

void BattleSiegeController::showAbsoluteObstacles(Canvas & canvas)
{
	if (getWallPieceExistence(EWallVisual::MOAT))
		showWallPiece(canvas, EWallVisual::MOAT);

	if (getWallPieceExistence(EWallVisual::MOAT_BANK))
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
	for (auto & stack : owner.getBattle()->battleGetAllStacks(true))
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

		if ( !getWallPieceExistence(wallPiece))
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

	if (gateFrontImage && owner.getBattle()->battleGetGateState() == EGateState::OPENED &&
		owner.getBattle()->battleGetWallState(EWallPart::GATE) != EWallState::DESTROYED)
	{
		renderer.insert( EBattleFieldLayer::OBSTACLES, BattleHex(BattleHex::GATE_BRIDGE), [this](BattleRenderer::RendererRef canvas){
			const auto & pos = town->getTown()->clientInfo.siegePositions[EWallVisual::GATE];
			if (pos.isValid())
				canvas.draw(gateFrontImage, Point(pos.x, pos.y));
		});
	}
}

bool BattleSiegeController::isAttackableByCatapult(const BattleHex & hex) const
{
	if (owner.isInTacticsMode())
		return false;

	auto wallPart = owner.getBattle()->battleHexToWallPart(hex);
	return owner.getBattle()->isWallPartAttackable(wallPart);
}

bool BattleSiegeController::isTowerHex(const BattleHex & hex) const
{
	const auto fortifications = town->fortificationsLevel();
	switch(owner.getBattle()->battleHexToWallPart(hex))
	{
	case EWallPart::KEEP:         return fortifications.citadelHealth > 0;
	case EWallPart::UPPER_TOWER:  return fortifications.upperTowerHealth > 0;
	case EWallPart::BOTTOM_TOWER: return fortifications.lowerTowerHealth > 0;
	default:                      return false;
	}
}

std::string BattleSiegeController::getTowersInfoText() const
{
	const auto fortifications = town->fortificationsLevel();
	std::string result;

	auto appendTower = [&](EWallVisual::EWallVisual creaturePiece, EWallPart wallPart, int towerHealth, const std::string & nameTextID)
	{
		if(towerHealth <= 0)
			return; // tower not built

		if(owner.getBattle()->battleGetWallState(wallPart) == EWallState::DESTROYED)
		{
			MetaString text = MetaString::createFromTextID("core.genrltxt.154"); // The %s is destroyed.
			text.replaceTextID(nameTextID);
			result += text.toString(&GAME->translator());
		}
		else
		{
			const CStack * turret = getTurretStack(creaturePiece);
			MetaString text = MetaString::createFromTextID("core.genrltxt.155"); // The %s has an attack skill of %d and does %d-%d damage.
			text.replaceTextID(nameTextID);
			text.replaceNumber(turret->getAttack(true)); // NOTE: H3 bug - tower attack always shows 10, but has no effect. VCMI shows 0
			text.replaceNumber(turret->getMinDamage(true));
			text.replaceNumber(turret->getMaxDamage(true));
			result += text.toString(&GAME->translator());
		}
	};

	appendTower(EWallVisual::KEEP_BATTLEMENT,   EWallPart::KEEP,         fortifications.citadelHealth,    "vcmi.battleWindow.siegeTower.keep");
	appendTower(EWallVisual::UPPER_BATTLEMENT,  EWallPart::UPPER_TOWER,  fortifications.upperTowerHealth, "vcmi.battleWindow.siegeTower.upper");
	appendTower(EWallVisual::BOTTOM_BATTLEMENT, EWallPart::BOTTOM_TOWER, fortifications.lowerTowerHealth, "vcmi.battleWindow.siegeTower.lower");

	return result;
}

void BattleSiegeController::stackIsCatapulting(const CatapultAttack & ca)
{
	// swaps a wall piece to its current (damaged) sprite
	auto updateWallPiece = [this](EWallPart attackedPart)
	{
		int wallId = static_cast<int>(attackedPart) + EWallVisual::DESTRUCTIBLE_FIRST;
		auto wallState = EWallState(owner.getBattle()->battleGetWallState(attackedPart));
		// the gate's open/close transitions are driven by BattleUpdateGateState; sync only its destroyed sprite
		// here so a broken gate updates on the breaking shot instead of after the whole catapult sequence
		if (wallId == EWallVisual::GATE && wallState != EWallState::DESTROYED)
			return;
		wallPieceImages[wallId] = ENGINE->renderHandler().loadImage(getWallPieceImageName(EWallVisual::EWallVisual(wallId), wallState), EImageBlitMode::COLORKEY);
	};

	if (ca.attacker != -1)
	{
		const CStack *stack = owner.getBattle()->battleGetStackByID(ca.attacker);
		auto catapult = new CatapultAnimation(owner, stack, ca.destinationTile, nullptr, ca.damageDealt);
		catapult->onExplosion = [this, updateWallPiece, ca]()
		{
			updateWallPiece(ca.attackedPart);
			if (ca.killedTowerShooter != -1)
				owner.stackRemoved(static_cast<uint32_t>(ca.killedTowerShooter));
		};
		owner.stacksController->addNewAnim(catapult);
		owner.waitForAnimations();
	}
	else
	{
		//no attacker stack, assume spell-related (earthquake) - only hit animation
		std::vector<Point> positions;
		positions.push_back(owner.stacksController->getStackPositionAtHex(ca.destinationTile, nullptr) + Point(99, 120));

		ENGINE->sound().playSound( AudioPath::builtin("WALLHIT") );

		// swap the wall sprite at the explosion midpoint instead of blocking, so a mid-cast earthquake
		// deferred into the caster's HIT stage does not wait for animations on the main thread
		auto attackedPart = ca.attackedPart;
		auto effect = new EffectAnimation(owner, AnimationPath::builtin("SGEXPL.DEF"), positions);
		effect->onMidpoint = [updateWallPiece, attackedPart](){ updateWallPiece(attackedPart); };
		owner.stacksController->addNewAnim(effect);
		owner.fieldController->startShakeAnimation();
	}
}

const CGTownInstance *BattleSiegeController::getSiegedTown() const
{
	return town;
}
