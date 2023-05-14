/*
 * BattleFieldController.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "BattleFieldController.h"

#include "BattleInterface.h"
#include "BattleActionsController.h"
#include "BattleInterfaceClasses.h"
#include "BattleEffectsController.h"
#include "BattleSiegeController.h"
#include "BattleStacksController.h"
#include "BattleObstacleController.h"
#include "BattleProjectileController.h"
#include "BattleRenderer.h"

#include "../CGameInfo.h"
#include "../CPlayerInterface.h"
#include "../render/Canvas.h"
#include "../render/IImage.h"
#include "../renderSDL/SDL_Extensions.h"
#include "../gui/CGuiHandler.h"
#include "../gui/CursorHandler.h"
#include "../adventureMap/CInGameConsole.h"

#include "../../CCallback.h"
#include "../../lib/BattleFieldHandler.h"
#include "../../lib/CConfigHandler.h"
#include "../../lib/CStack.h"
#include "../../lib/spells/ISpellMechanics.h"

BattleFieldController::BattleFieldController(BattleInterface & owner):
	owner(owner)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL_NO_DISPOSE;
	strongInterest = true;

	//preparing cells and hexes
	cellBorder = IImage::createFromFile("CCELLGRD.BMP", EImageBlitMode::COLORKEY);
	cellShade = IImage::createFromFile("CCELLSHD.BMP");
	cellUnitMovementHighlight = IImage::createFromFile("UnitMovementHighlight.PNG", EImageBlitMode::COLORKEY);
	cellUnitMaxMovementHighlight = IImage::createFromFile("UnitMaxMovementHighlight.PNG", EImageBlitMode::COLORKEY);

	// load single edges
	fullDamageRangeLimitImages[0b000001] = IImage::createFromFile("rangeHighlights/green/topLeft.PNG", EImageBlitMode::COLORKEY);
	fullDamageRangeLimitImages[0b000010] = IImage::createFromFile("rangeHighlights/green/topRight.PNG", EImageBlitMode::COLORKEY);
	fullDamageRangeLimitImages[0b000100] = IImage::createFromFile("rangeHighlights/green/right.PNG", EImageBlitMode::COLORKEY);
	fullDamageRangeLimitImages[0b001000] = IImage::createFromFile("rangeHighlights/green/bottomRight.PNG", EImageBlitMode::COLORKEY);
	fullDamageRangeLimitImages[0b010000] = IImage::createFromFile("rangeHighlights/green/bottomLeft.PNG", EImageBlitMode::COLORKEY);
	fullDamageRangeLimitImages[0b100000] = IImage::createFromFile("rangeHighlights/green/left.PNG", EImageBlitMode::COLORKEY);

	// load double edges
	fullDamageRangeLimitImages[0b000011] = IImage::createFromFile("rangeHighlights/green/top.PNG", EImageBlitMode::COLORKEY);
	fullDamageRangeLimitImages[0b011000] = IImage::createFromFile("rangeHighlights/green/bottom.PNG", EImageBlitMode::COLORKEY);

	fullDamageRangeLimitImages[0b000110] = IImage::createFromFile("rangeHighlights/green/topRightHalfCorner.PNG", EImageBlitMode::COLORKEY);
	fullDamageRangeLimitImages[0b001100] = IImage::createFromFile("rangeHighlights/green/bottomRightHalfCorner.PNG", EImageBlitMode::COLORKEY);
	fullDamageRangeLimitImages[0b110000] = IImage::createFromFile("rangeHighlights/green/bottomLeftHalfCorner.PNG", EImageBlitMode::COLORKEY);
	fullDamageRangeLimitImages[0b100001] = IImage::createFromFile("rangeHighlights/green/topLeftHalfCorner.PNG", EImageBlitMode::COLORKEY);

	// load halves
	fullDamageRangeLimitImages[0b001110] = IImage::createFromFile("rangeHighlights/green/rightHalf.PNG", EImageBlitMode::COLORKEY);
	fullDamageRangeLimitImages[0b110001] = IImage::createFromFile("rangeHighlights/green/leftHalf.PNG", EImageBlitMode::COLORKEY);

	// load corners
	fullDamageRangeLimitImages[0b000111] = IImage::createFromFile("rangeHighlights/green/topRightCorner.PNG", EImageBlitMode::COLORKEY);
	fullDamageRangeLimitImages[0b011100] = IImage::createFromFile("rangeHighlights/green/bottomRightCorner.PNG", EImageBlitMode::COLORKEY);
	fullDamageRangeLimitImages[0b111000] = IImage::createFromFile("rangeHighlights/green/bottomLeftCorner.PNG", EImageBlitMode::COLORKEY);
	fullDamageRangeLimitImages[0b100011] = IImage::createFromFile("rangeHighlights/green/topLeftCorner.PNG", EImageBlitMode::COLORKEY);

	if(!owner.siegeController)
	{
		auto bfieldType = owner.curInt->cb->battleGetBattlefieldType();

		if(bfieldType == BattleField::NONE)
			logGlobal->error("Invalid battlefield returned for current battle");
		else
			background = IImage::createFromFile(bfieldType.getInfo()->graphics, EImageBlitMode::OPAQUE);
	}
	else
	{
		std::string backgroundName = owner.siegeController->getBattleBackgroundName();
		background = IImage::createFromFile(backgroundName, EImageBlitMode::OPAQUE);
	}

	pos.w = background->width();
	pos.h = background->height();

	backgroundWithHexes = std::make_unique<Canvas>(Point(background->width(), background->height()));

	updateAccessibleHexes();
	addUsedEvents(LCLICK | RCLICK | MOVE);
}

void BattleFieldController::activate()
{
	LOCPLINT->cingconsole->pos = this->pos;
	CIntObject::activate();
}

void BattleFieldController::createHeroes()
{
	OBJ_CONSTRUCTION_CAPTURING_ALL_NO_DISPOSE;

	// create heroes as part of our constructor for correct positioning inside battlefield
	if(owner.attackingHeroInstance)
		owner.attackingHero = std::make_shared<BattleHero>(owner, owner.attackingHeroInstance, false);

	if(owner.defendingHeroInstance)
		owner.defendingHero = std::make_shared<BattleHero>(owner, owner.defendingHeroInstance, true);
}

void BattleFieldController::mouseMoved(const Point & cursorPosition)
{
	if (!pos.isInside(cursorPosition))
	{
		owner.actionsController->onHoverEnded();
		return;
	}

	BattleHex selectedHex = getHoveredHex();
	owner.actionsController->onHexHovered(selectedHex);
}

void BattleFieldController::clickLeft(tribool down, bool previousState)
{
	if(!down)
	{
		BattleHex selectedHex = getHoveredHex();

		if (selectedHex != BattleHex::INVALID)
			owner.actionsController->onHexLeftClicked(selectedHex);
	}
}

void BattleFieldController::clickRight(tribool down, bool previousState)
{
	if(down)
	{
		BattleHex selectedHex = getHoveredHex();

		if (selectedHex != BattleHex::INVALID)
			owner.actionsController->onHexRightClicked(selectedHex);

	}
}

void BattleFieldController::renderBattlefield(Canvas & canvas)
{
	Canvas clippedCanvas(canvas, pos);

	showBackground(clippedCanvas);

	BattleRenderer renderer(owner);

	renderer.execute(clippedCanvas);

	owner.projectilesController->showProjectiles(clippedCanvas);
}

void BattleFieldController::showBackground(Canvas & canvas)
{
	if (owner.stacksController->getActiveStack() != nullptr ) //&& creAnims[stacksController->getActiveStack()->unitId()]->isIdle() //show everything with range
		showBackgroundImageWithHexes(canvas);
	else
		showBackgroundImage(canvas);

	showHighlightedHexes(canvas);
}

void BattleFieldController::showBackgroundImage(Canvas & canvas)
{
	canvas.draw(background, Point(0, 0));

	owner.obstacleController->showAbsoluteObstacles(canvas);
	if ( owner.siegeController )
		owner.siegeController->showAbsoluteObstacles(canvas);

	if (settings["battle"]["cellBorders"].Bool())
	{
		for (int i=0; i<GameConstants::BFIELD_SIZE; ++i)
		{
			if ( i % GameConstants::BFIELD_WIDTH == 0)
				continue;
			if ( i % GameConstants::BFIELD_WIDTH == GameConstants::BFIELD_WIDTH - 1)
				continue;

			canvas.draw(cellBorder, hexPositionLocal(i).topLeft());
		}
	}
}

void BattleFieldController::showBackgroundImageWithHexes(Canvas & canvas)
{
	canvas.draw(*backgroundWithHexes.get(), Point(0, 0));
}

void BattleFieldController::redrawBackgroundWithHexes()
{
	const CStack *activeStack = owner.stacksController->getActiveStack();
	std::vector<BattleHex> attackableHexes;
	if(activeStack)
		occupiableHexes = owner.curInt->cb->battleGetAvailableHexes(activeStack, false, true, &attackableHexes);

	// prepare background graphic with hexes and shaded hexes
	backgroundWithHexes->draw(background, Point(0,0));
	owner.obstacleController->showAbsoluteObstacles(*backgroundWithHexes);
	if(owner.siegeController)
		owner.siegeController->showAbsoluteObstacles(*backgroundWithHexes);

	// show shaded hexes for active's stack valid movement and the hexes that it can attack
	if(settings["battle"]["stackRange"].Bool())
	{
		std::vector<BattleHex> hexesToShade = occupiableHexes;
		hexesToShade.insert(hexesToShade.end(), attackableHexes.begin(), attackableHexes.end());
		for(BattleHex hex : hexesToShade)
		{
			showHighlightedHex(*backgroundWithHexes, cellShade, hex, false);
		}
	}

	// draw cell borders
	if(settings["battle"]["cellBorders"].Bool())
	{
		for(int i=0; i<GameConstants::BFIELD_SIZE; ++i)
		{
			if(i % GameConstants::BFIELD_WIDTH == 0)
				continue;
			if(i % GameConstants::BFIELD_WIDTH == GameConstants::BFIELD_WIDTH - 1)
				continue;

			backgroundWithHexes->draw(cellBorder, hexPositionLocal(i).topLeft());
		}
	}
}

void BattleFieldController::showHighlightedHex(Canvas & canvas, std::shared_ptr<IImage> highlight, BattleHex hex, bool darkBorder)
{
	Point hexPos = hexPositionLocal(hex).topLeft();

	canvas.draw(highlight, hexPos);
	if(!darkBorder && settings["battle"]["cellBorders"].Bool())
		canvas.draw(cellBorder, hexPos);
}

std::set<BattleHex> BattleFieldController::getHighlightedHexesForActiveStack()
{
	std::set<BattleHex> result;

	if(!owner.stacksController->getActiveStack())
		return result;

	if(!settings["battle"]["stackRange"].Bool())
		return result;

	auto hoveredHex = getHoveredHex();

	std::set<BattleHex> set = owner.curInt->cb->battleGetAttackedHexes(owner.stacksController->getActiveStack(), hoveredHex, attackingHex);
	for(BattleHex hex : set)
		result.insert(hex);

	return result;
}

std::set<BattleHex> BattleFieldController::getMovementRangeForHoveredStack()
{
	std::set<BattleHex> result;

	if (!owner.stacksController->getActiveStack())
		return result;

	if (!settings["battle"]["movementHighlightOnHover"].Bool() && !GH.isKeyboardShiftDown())
		return result;

	auto hoveredHex = getHoveredHex();

	// add possible movement hexes for stack under mouse
	const CStack * const hoveredStack = owner.curInt->cb->battleGetStackByPos(hoveredHex, true);
	if(hoveredStack)
	{
		std::vector<BattleHex> v = owner.curInt->cb->battleGetAvailableHexes(hoveredStack, true, true, nullptr);
		for(BattleHex hex : v)
			result.insert(hex);
	}
	return result;
}

std::set<BattleHex> BattleFieldController::getHighlightedHexesForSpellRange()
{
	std::set<BattleHex> result;
	auto hoveredHex = getHoveredHex();

	if(!settings["battle"]["mouseShadow"].Bool())
		return result;

	const spells::Caster *caster = nullptr;
	const CSpell *spell = nullptr;

	spells::Mode mode = owner.actionsController->getCurrentCastMode();
	spell = owner.actionsController->getCurrentSpell(hoveredHex);
	caster = owner.actionsController->getCurrentSpellcaster();

	if(caster && spell) //when casting spell
	{
		// printing shaded hex(es)
		spells::BattleCast event(owner.curInt->cb.get(), caster, mode, spell);
		auto shadedHexes = spell->battleMechanics(&event)->rangeInHexes(hoveredHex);

		for(BattleHex shadedHex : shadedHexes)
		{
			if((shadedHex.getX() != 0) && (shadedHex.getX() != GameConstants::BFIELD_WIDTH - 1))
				result.insert(shadedHex);
		}
	}
	return result;
}

std::set<BattleHex> BattleFieldController::getHighlightedHexesForMovementTarget()
{
	const CStack * stack = owner.stacksController->getActiveStack();
	auto hoveredHex = getHoveredHex();

	if(!stack)
		return {};

	std::vector<BattleHex> availableHexes = owner.curInt->cb->battleGetAvailableHexes(stack, false, false, nullptr);

	auto hoveredStack = owner.curInt->cb->battleGetStackByPos(hoveredHex, true);
	if(owner.curInt->cb->battleCanAttack(stack, hoveredStack, hoveredHex))
	{
		if(isTileAttackable(hoveredHex))
		{
			BattleHex attackFromHex = fromWhichHexAttack(hoveredHex);

			if(stack->doubleWide())
				return {attackFromHex, stack->occupiedHex(attackFromHex)};
			else
				return {attackFromHex};
		}
	}

	if(vstd::contains(availableHexes, hoveredHex))
	{
		if(stack->doubleWide())
			return {hoveredHex, stack->occupiedHex(hoveredHex)};
		else
			return {hoveredHex};
	}

	if(stack->doubleWide())
	{
		for(auto const & hex : availableHexes)
		{
			if(stack->occupiedHex(hex) == hoveredHex)
				return {hoveredHex, hex};
		}
	}

	return {};
}

std::vector<BattleHex> BattleFieldController::getRangedFullDamageHexes()
{
	std::vector<BattleHex> rangedFullDamageHexes; // used for return
	std::set<BattleHex> battleFieldWithoutSideColumns;

	// if not a hovered arcer unit -> return
	auto hoveredHex = getHoveredHex();
	const CStack * hoveredStack = owner.curInt->cb->battleGetStackByPos(hoveredHex, true);

	if (!settings["battle"]["rangedFullDamageLimitHighlightOnHover"].Bool() && !GH.isKeyboardShiftDown())
		return rangedFullDamageHexes;

	if(!(hoveredStack && hoveredStack->isShooter()))
		return rangedFullDamageHexes;
	
	// construct battlefield grid without the 2 unusable sidecolumns
	for(auto x = 1; x < GameConstants::BFIELD_WIDTH - 1; ++x)
	{
		for(auto y = 0; y < GameConstants::BFIELD_HEIGHT; ++y)
			battleFieldWithoutSideColumns.insert(BattleHex(x, y));
	}

	auto rangedFullDamageDistance = GameConstants::BATTLE_PENALTY_DISTANCE;

	// overwrite full ranged damage distance from Additional info field of LIMITED_SHOOTING_RANGE bonus
	auto bonus = hoveredStack->getBonus(Selector::type()(BonusType::LIMITED_SHOOTING_RANGE));
	if(bonus != nullptr && bonus->additionalInfo != CAddInfo::NONE)
		rangedFullDamageDistance = bonus->additionalInfo[0];

	// get only battlefield hexes that are in full range damage distance
	std::set<BattleHex> fullRangeLimit; 
	for(auto & hex : battleFieldWithoutSideColumns)
	{
		if(BattleHex::getDistance(hoveredHex, hex) <= rangedFullDamageDistance)
			rangedFullDamageHexes.push_back(hex);
	}

	return rangedFullDamageHexes;
}

std::vector<BattleHex> BattleFieldController::getRangedFullDamageLimitHexes(std::vector<BattleHex> rangedFullDamageHexes)
{
	std::vector<BattleHex> rangedFullDamageLimitHexes; // used for return

	// if not a hovered arcer unit -> return
	auto hoveredHex = getHoveredHex();
	const CStack * hoveredStack = owner.curInt->cb->battleGetStackByPos(hoveredHex, true);
	if(!(hoveredStack && hoveredStack->isShooter()))
		return rangedFullDamageLimitHexes;

	auto rangedFullDamageDistance = GameConstants::BATTLE_PENALTY_DISTANCE;

	// overwrite full ranged damage distance from Additional info field of LIMITED_SHOOTING_RANGE bonus
	auto bonus = hoveredStack->getBonus(Selector::type()(BonusType::LIMITED_SHOOTING_RANGE));
	if(bonus != nullptr && bonus->additionalInfo != CAddInfo::NONE)
		rangedFullDamageDistance = bonus->additionalInfo[0];

	// from ranged full damage hexes get only the ones at the limit
	for(auto & hex : rangedFullDamageHexes)
	{
		if(BattleHex::getDistance(hoveredHex, hex) == rangedFullDamageDistance)
			rangedFullDamageLimitHexes.push_back(hex);
	}

	return rangedFullDamageLimitHexes;
}

std::vector<std::vector<BattleHex::EDir>> BattleFieldController::getOutsideNeighbourDirectionsForLimitHexes(std::vector<BattleHex> rangedFullDamageHexes, std::vector<BattleHex> rangedFullDamageLimitHexes)
{
	std::vector<std::vector<BattleHex::EDir>> output;

	if(rangedFullDamageHexes.empty())
		return output;

	for(auto & hex : rangedFullDamageLimitHexes)
	{
		// get all neighbours and their directions
		auto neighbours = hex.neighbouringTilesWithDirection();

		std::vector<BattleHex::EDir> outsideNeighbourDirections;

		// for each neighbour add to output only the ones not found in rangedFullDamageHexes
		for(auto & neighbour : neighbours)
		{
			auto it = std::find(rangedFullDamageHexes.begin(), rangedFullDamageHexes.end(), neighbour.second);

			if(it == rangedFullDamageHexes.end())
				outsideNeighbourDirections.push_back(neighbour.first); // push direction
		}

		output.push_back(outsideNeighbourDirections);
	}

	return output;
}

std::vector<std::shared_ptr<IImage>> BattleFieldController::calculateFullRangedDamageHighlightImages(std::vector<std::vector<BattleHex::EDir>> fullRangedDamageLimitHexesNeighbourDirections)
{
	std::vector<std::shared_ptr<IImage>> output; // if no image is to be shown an empty image is still added to help with traverssing the range

	if(fullRangedDamageLimitHexesNeighbourDirections.empty())
		return output;

	for(auto & directions : fullRangedDamageLimitHexesNeighbourDirections)
	{
		std::bitset<6> mask;
		
		// convert directions to mask
		for(auto direction : directions)
			mask.set(direction);

		uint8_t imageKey = static_cast<uint8_t>(mask.to_ulong());
		output.push_back(fullDamageRangeLimitImages[imageKey]);
	}

	return output;
}

void BattleFieldController::showHighlightedHexes(Canvas & canvas)
{
	std::set<BattleHex> hoveredStackMovementRangeHexes = getMovementRangeForHoveredStack();
	std::set<BattleHex> hoveredSpellHexes = getHighlightedHexesForSpellRange();
	std::set<BattleHex> hoveredMoveHexes  = getHighlightedHexesForMovementTarget();

	if(getHoveredHex() == BattleHex::INVALID)
		return;

	// calculate array with highlight images for ranged full damage limit
	std::vector<BattleHex> rangedFullDamageHexes = getRangedFullDamageHexes();
	std::vector<BattleHex> rangedFullDamageLimitHexes = getRangedFullDamageLimitHexes(rangedFullDamageHexes);
	std::vector<std::vector<BattleHex::EDir>> rangedFullDamageLimitHexesNeighbourDirections = getOutsideNeighbourDirectionsForLimitHexes(rangedFullDamageHexes, rangedFullDamageLimitHexes);
	std::vector<std::shared_ptr<IImage>> rangedFullDamageLimitHexesHighligts = calculateFullRangedDamageHighlightImages(rangedFullDamageLimitHexesNeighbourDirections);

	auto const & hoveredMouseHexes = owner.actionsController->currentActionSpellcasting(getHoveredHex()) ? hoveredSpellHexes : hoveredMoveHexes;

	for(int hex = 0; hex < GameConstants::BFIELD_SIZE; ++hex)
	{
		bool stackMovement = hoveredStackMovementRangeHexes.count(hex);
		bool mouse = hoveredMouseHexes.count(hex);

		// calculate if hex is Ranged Full Damage Limit and its position in highlight array
		bool isRangedFullDamageLimit = false;
		int hexIndexInRangedFullDamageLimit = 0;
		if(!rangedFullDamageLimitHexes.empty())
		{
			auto pos = std::find(rangedFullDamageLimitHexes.begin(), rangedFullDamageLimitHexes.end(), hex);
			hexIndexInRangedFullDamageLimit = std::distance(rangedFullDamageLimitHexes.begin(), pos);
			isRangedFullDamageLimit = pos != rangedFullDamageLimitHexes.end();
		}

		if(stackMovement && mouse) // area where hovered stackMovement can move shown with highlight. Because also affected by mouse cursor, shade as well
		{
			showHighlightedHex(canvas, cellUnitMovementHighlight, hex, false);
			showHighlightedHex(canvas, cellShade, hex, true);
		}
		if(!stackMovement && mouse) // hexes affected only at mouse cursor shown as shaded
		{
			showHighlightedHex(canvas, cellShade, hex, true);
		}
		if(stackMovement && !mouse) // hexes where hovered stackMovement can move shown with highlight
		{
			showHighlightedHex(canvas, cellUnitMovementHighlight, hex, false);
		}
		if(isRangedFullDamageLimit)
		{
			showHighlightedHex(canvas, rangedFullDamageLimitHexesHighligts[hexIndexInRangedFullDamageLimit], hex, false);
		}
	}
}

Rect BattleFieldController::hexPositionLocal(BattleHex hex) const
{
	int x = 14 + ((hex.getY())%2==0 ? 22 : 0) + 44*hex.getX();
	int y = 86 + 42 *hex.getY();
	int w = cellShade->width();
	int h = cellShade->height();
	return Rect(x, y, w, h);
}

Rect BattleFieldController::hexPositionAbsolute(BattleHex hex) const
{
	return hexPositionLocal(hex) + pos.topLeft();
}

bool BattleFieldController::isPixelInHex(Point const & position)
{
	return !cellShade->isTransparent(position);
}

BattleHex BattleFieldController::getHoveredHex()
{
	Point hoverPos = GH.getCursorPosition();

	if (owner.attackingHero)
	{
		if (owner.attackingHero->pos.isInside(hoverPos))
			return BattleHex::HERO_ATTACKER;
	}

	if (owner.defendingHero)
	{
		if (owner.attackingHero->pos.isInside(hoverPos))
			return BattleHex::HERO_DEFENDER;
	}


	for (int h = 0; h < GameConstants::BFIELD_SIZE; ++h)
	{
		Rect hexPosition = hexPositionAbsolute(h);

		if (!hexPosition.isInside(hoverPos))
			continue;

		if (isPixelInHex(hoverPos - hexPosition.topLeft()))
			return h;
	}

	return BattleHex::INVALID;
}

void BattleFieldController::setBattleCursor(BattleHex myNumber)
{
	Point cursorPos = CCS->curh->position();

	std::vector<Cursor::Combat> sectorCursor = {
		Cursor::Combat::HIT_SOUTHEAST,
		Cursor::Combat::HIT_SOUTHWEST,
		Cursor::Combat::HIT_WEST,
		Cursor::Combat::HIT_NORTHWEST,
		Cursor::Combat::HIT_NORTHEAST,
		Cursor::Combat::HIT_EAST,
		Cursor::Combat::HIT_SOUTH,
		Cursor::Combat::HIT_NORTH,
	};

	auto direction = static_cast<size_t>(selectAttackDirection(myNumber, cursorPos));

	assert(direction != -1);
	if (direction != -1)
		CCS->curh->set(sectorCursor[direction]);
}

BattleHex::EDir BattleFieldController::selectAttackDirection(BattleHex myNumber, const Point & cursorPos)
{
	const bool doubleWide = owner.stacksController->getActiveStack()->doubleWide();
	auto neighbours = myNumber.allNeighbouringTiles();
	//   0 1
	//  5 x 2
	//   4 3

	// if true - our current stack can move into this hex (and attack)
	std::array<bool, 8> attackAvailability;

	if (doubleWide)
	{
		// For double-hexes we need to ensure that both hexes needed for this direction are occupyable:
		// |    -0-   |   -1-    |    -2-   |   -3-    |    -4-   |   -5-    |    -6-   |   -7-
		// |  o o -   |   - o o  |    - -   |   - -    |    - -   |   - -    |    o o   |   - -
		// |   - x -  |  - x -   |   - x o o|  - x -   |   - x -  |o o x -   |   - x -  |  - x -
		// |    - -   |   - -    |    - -   |   - o o  |  o o -   |   - -    |    - -   |   o o

		for (size_t i : { 1, 2, 3})
			attackAvailability[i] = vstd::contains(occupiableHexes, neighbours[i]) && vstd::contains(occupiableHexes, neighbours[i].cloneInDirection(BattleHex::RIGHT, false));

		for (size_t i : { 4, 5, 0})
			attackAvailability[i] = vstd::contains(occupiableHexes, neighbours[i]) && vstd::contains(occupiableHexes, neighbours[i].cloneInDirection(BattleHex::LEFT, false));

		attackAvailability[6] = vstd::contains(occupiableHexes, neighbours[0]) && vstd::contains(occupiableHexes, neighbours[1]);
		attackAvailability[7] = vstd::contains(occupiableHexes, neighbours[3]) && vstd::contains(occupiableHexes, neighbours[4]);
	}
	else
	{
		for (size_t i = 0; i < 6; ++i)
			attackAvailability[i] = vstd::contains(occupiableHexes, neighbours[i]);

		attackAvailability[6] = false;
		attackAvailability[7] = false;
	}

	// Zero available tiles to attack from
	if ( vstd::find(attackAvailability, true) == attackAvailability.end())
	{
		logGlobal->error("Error: cannot find a hex to attack hex %d from!", myNumber);
		return BattleHex::NONE;
	}

	// For each valid direction, select position to test against
	std::array<Point, 8> testPoint;

	for (size_t i = 0; i < 6; ++i)
		if (attackAvailability[i])
			testPoint[i] = hexPositionAbsolute(neighbours[i]).center();

	// For bottom/top directions select central point, but move it a bit away from true center to reduce zones allocated to them
	if (attackAvailability[6])
		testPoint[6] = (hexPositionAbsolute(neighbours[0]).center() + hexPositionAbsolute(neighbours[1]).center()) / 2 + Point(0, -5);

	if (attackAvailability[7])
		testPoint[7] = (hexPositionAbsolute(neighbours[3]).center() + hexPositionAbsolute(neighbours[4]).center()) / 2 + Point(0,  5);

	// Compute distance between tested position & cursor position and pick nearest
	std::array<int, 8> distance2;

	for (size_t i = 0; i < 8; ++i)
		if (attackAvailability[i])
			distance2[i] = (testPoint[i].y - cursorPos.y)*(testPoint[i].y - cursorPos.y) + (testPoint[i].x - cursorPos.x)*(testPoint[i].x - cursorPos.x);

	size_t nearest = -1;
	for (size_t i = 0; i < 8; ++i)
		if (attackAvailability[i] && (nearest == -1 || distance2[i] < distance2[nearest]) )
			nearest = i;

	assert(nearest != -1);
	return BattleHex::EDir(nearest);
}

BattleHex BattleFieldController::fromWhichHexAttack(BattleHex attackTarget)
{
	BattleHex::EDir direction = selectAttackDirection(attackTarget, CCS->curh->position());

	const CStack * attacker = owner.stacksController->getActiveStack();

	assert(direction != BattleHex::NONE);
	assert(attacker);

	if (!attacker->doubleWide())
	{
		assert(direction != BattleHex::BOTTOM);
		assert(direction != BattleHex::TOP);
		return attackTarget.cloneInDirection(direction);
	}
	else
	{
		// We need to find position of right hex of double-hex creature (or left for defending side)
		// | TOP_LEFT |TOP_RIGHT |   RIGHT  |BOTTOM_RIGHT|BOTTOM_LEFT|  LEFT    |    TOP   |BOTTOM
		// |  o o -   |   - o o  |    - -   |   - -      |    - -    |   - -    |    o o   |   - -
		// |   - x -  |  - x -   |   - x o o|  - x -     |   - x -   |o o x -   |   - x -  |  - x -
		// |    - -   |   - -    |    - -   |   - o o    |  o o -    |   - -    |    - -   |   o o

		switch (direction)
		{
		case BattleHex::TOP_LEFT:
		case BattleHex::LEFT:
		case BattleHex::BOTTOM_LEFT:
		{
			if ( attacker->unitSide() == BattleSide::ATTACKER )
				return attackTarget.cloneInDirection(direction);
			else
				return attackTarget.cloneInDirection(direction).cloneInDirection(BattleHex::LEFT);
		}

		case BattleHex::TOP_RIGHT:
		case BattleHex::RIGHT:
		case BattleHex::BOTTOM_RIGHT:
		{
			if ( attacker->unitSide() == BattleSide::ATTACKER )
				return attackTarget.cloneInDirection(direction).cloneInDirection(BattleHex::RIGHT);
			else
				return attackTarget.cloneInDirection(direction);
		}

		case BattleHex::TOP:
		{
			if ( attacker->unitSide() == BattleSide::ATTACKER )
				return attackTarget.cloneInDirection(BattleHex::TOP_RIGHT);
			else
				return attackTarget.cloneInDirection(BattleHex::TOP_LEFT);
		}

		case BattleHex::BOTTOM:
		{
			if ( attacker->unitSide() == BattleSide::ATTACKER )
				return attackTarget.cloneInDirection(BattleHex::BOTTOM_RIGHT);
			else
				return attackTarget.cloneInDirection(BattleHex::BOTTOM_LEFT);
		}
		default:
			assert(0);
			return BattleHex::INVALID;
		}
	}
}

bool BattleFieldController::isTileAttackable(const BattleHex & number) const
{
	for (auto & elem : occupiableHexes)
	{
		if (BattleHex::mutualPosition(elem, number) != -1 || elem == number)
			return true;
	}
	return false;
}

void BattleFieldController::updateAccessibleHexes()
{
	auto accessibility = owner.curInt->cb->getAccesibility();

	for(int i = 0; i < accessibility.size(); i++)
		stackCountOutsideHexes[i] = (accessibility[i] == EAccessibility::ACCESSIBLE || (accessibility[i] == EAccessibility::SIDE_COLUMN));
}

bool BattleFieldController::stackCountOutsideHex(const BattleHex & number) const
{
	return stackCountOutsideHexes[number];
}

void BattleFieldController::showAll(SDL_Surface * to)
{
	show(to);
}

void BattleFieldController::show(SDL_Surface * to)
{
	updateAccessibleHexes();
	owner.stacksController->update();
	owner.obstacleController->update();

	Canvas canvas(to);
	CSDL_Ext::CClipRectGuard guard(to, pos);

	renderBattlefield(canvas);
}
