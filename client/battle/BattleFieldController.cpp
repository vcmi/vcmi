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

#include "BattleActionsController.h"
#include "BattleEffectsController.h"
#include "BattleInterface.h"
#include "BattleHero.h"
#include "BattleObstacleController.h"
#include "BattleProjectileController.h"
#include "BattleRenderer.h"
#include "BattleSiegeController.h"
#include "BattleStacksController.h"
#include "BattleWindow.h"

#include "../CPlayerInterface.h"
#include "../GameEngine.h"
#include "../GameInstance.h"
#include "../adventureMap/CInGameConsole.h"
#include "../client/render/CAnimation.h"
#include "../gui/CursorHandler.h"
#include "../render/CAnimation.h"
#include "../render/Canvas.h"
#include "../render/IImage.h"
#include "../render/IRenderHandler.h"

#include "../../lib/BattleFieldHandler.h"
#include "../../lib/CConfigHandler.h"
#include "../../lib/CStack.h"
#include "../../lib/battle/CPlayerBattleCallback.h"
#include "../../lib/spells/ISpellMechanics.h"

namespace HexMasks
{
	// mask definitions that has set to 1 the edges present in the hex edges highlight image
	/*
	    /\
	   0  1
	  /    \
	 |      |
	 5      2
	 |      |
	  \    /
	   4  3
	    \/
	*/
	enum HexEdgeMasks {
		empty                 = 0b000000, // empty used when wanting to keep indexes the same but no highlight should be displayed
		topLeft               = 0b000001,
		topRight              = 0b000010,
		right                 = 0b000100,
		bottomRight           = 0b001000,
		bottomLeft            = 0b010000,
		left                  = 0b100000,
						  
		top                   = 0b000011,
		bottom                = 0b011000,
		topRightHalfCorner    = 0b000110,
		bottomRightHalfCorner = 0b001100,
		bottomLeftHalfCorner  = 0b110000,
		topLeftHalfCorner     = 0b100001,

		rightTopAndBottom     = 0b001010, // special case, right half can be drawn instead of only top and bottom
		leftTopAndBottom      = 0b010001, // special case, left half can be drawn instead of only top and bottom
						  
		rightHalf             = 0b001110,
		leftHalf              = 0b110001,
						  
		topRightCorner        = 0b000111,
		bottomRightCorner     = 0b011100,
		bottomLeftCorner      = 0b111000,
		topLeftCorner         = 0b100011
	};
}

static const std::map<int, int> hexEdgeMaskToFrameIndex =
{
    { HexMasks::empty, 0 },
    { HexMasks::topLeft, 1 },
    { HexMasks::topRight, 2 },
    { HexMasks::right, 3 },
    { HexMasks::bottomRight, 4 },
    { HexMasks::bottomLeft, 5 },
    { HexMasks::left, 6 },
    { HexMasks::top, 7 },
    { HexMasks::bottom, 8 },
    { HexMasks::topRightHalfCorner, 9 },
    { HexMasks::bottomRightHalfCorner, 10 },
    { HexMasks::bottomLeftHalfCorner, 11 },
    { HexMasks::topLeftHalfCorner, 12 },
    { HexMasks::rightTopAndBottom, 13 },
    { HexMasks::leftTopAndBottom, 14 },
    { HexMasks::rightHalf, 13 },
    { HexMasks::leftHalf, 14 },
    { HexMasks::topRightCorner, 15 },
    { HexMasks::bottomRightCorner, 16 },
    { HexMasks::bottomLeftCorner, 17 },
    { HexMasks::topLeftCorner, 18 }
};

BattleFieldController::BattleFieldController(BattleInterface & owner):
	owner(owner)
{
	OBJECT_CONSTRUCTION;

	//preparing cells and hexes
	cellBorder = ENGINE->renderHandler().loadImage(ImagePath::builtin("CCELLGRD.BMP"), EImageBlitMode::COLORKEY);
	cellShade = ENGINE->renderHandler().loadImage(ImagePath::builtin("CCELLSHD.BMP"), EImageBlitMode::SIMPLE);
	cellUnitMovementHighlight = ENGINE->renderHandler().loadImage(ImagePath::builtin("UnitMovementHighlight.PNG"), EImageBlitMode::COLORKEY);
	cellUnitMaxMovementHighlight = ENGINE->renderHandler().loadImage(ImagePath::builtin("UnitMaxMovementHighlight.PNG"), EImageBlitMode::COLORKEY);

	rangedFullDamageLimitImages = ENGINE->renderHandler().loadAnimation(AnimationPath::builtin("battle/rangeHighlights/rangeHighlightsGreen.json"), EImageBlitMode::COLORKEY);
	shootingRangeLimitImages = ENGINE->renderHandler().loadAnimation(AnimationPath::builtin("battle/rangeHighlights/rangeHighlightsRed.json"), EImageBlitMode::COLORKEY);

	if(!owner.siegeController)
	{
		auto bfieldType = owner.getBattle()->battleGetBattlefieldType();

		if(bfieldType == BattleField::NONE)
			logGlobal->error("Invalid battlefield returned for current battle");
		else
			background = ENGINE->renderHandler().loadImage(bfieldType.getInfo()->graphics, EImageBlitMode::OPAQUE);
	}
	else
	{
		auto backgroundName = owner.siegeController->getBattleBackgroundName();
		background = ENGINE->renderHandler().loadImage(backgroundName, EImageBlitMode::OPAQUE);
	}

	pos.w = background->width();
	pos.h = background->height();

	backgroundWithHexes = std::make_unique<Canvas>(Point(background->width(), background->height()), CanvasScalingPolicy::AUTO);

	updateAccessibleHexes();
	addUsedEvents(LCLICK | SHOW_POPUP | MOVE | TIME | GESTURE);
}

void BattleFieldController::activate()
{
	GAME->interface()->cingconsole->pos = this->pos;
	CIntObject::activate();
}

void BattleFieldController::createHeroes()
{
	OBJECT_CONSTRUCTION;

	// create heroes as part of our constructor for correct positioning inside battlefield
	if(owner.attackingHeroInstance)
		owner.attackingHero = std::make_shared<BattleHero>(owner, owner.attackingHeroInstance, false);

	if(owner.defendingHeroInstance)
		owner.defendingHero = std::make_shared<BattleHero>(owner, owner.defendingHeroInstance, true);
}

void BattleFieldController::gesture(bool on, const Point & initialPosition, const Point & finalPosition)
{
	if (!on && pos.isInside(finalPosition))
		clickPressed(finalPosition);
}

void BattleFieldController::gesturePanning(const Point & initialPosition, const Point & currentPosition, const Point & lastUpdateDistance)
{
	Point distance = currentPosition - initialPosition;

	if (distance.length() < settings["battle"]["swipeAttackDistance"].Float())
		hoveredHex = getHexAtPosition(initialPosition);
	else
		hoveredHex = BattleHex::INVALID;

	currentAttackOriginPoint = currentPosition;

	if (pos.isInside(initialPosition))
		owner.actionsController->onHexHovered(getHoveredHex());
}

void BattleFieldController::mouseMoved(const Point & cursorPosition, const Point & lastUpdateDistance)
{
	hoveredHex = getHexAtPosition(cursorPosition);
	currentAttackOriginPoint = cursorPosition;

	if (pos.isInside(cursorPosition))
		owner.actionsController->onHexHovered(getHoveredHex());
	else
		owner.actionsController->onHoverEnded();
}

void BattleFieldController::clickPressed(const Point & cursorPosition)
{
	BattleHex selectedHex = getHoveredHex();

	if (selectedHex != BattleHex::INVALID)
		owner.actionsController->onHexLeftClicked(selectedHex);
}

void BattleFieldController::showPopupWindow(const Point & cursorPosition)
{
	BattleHex selectedHex = getHoveredHex();

	if (selectedHex != BattleHex::INVALID)
		owner.actionsController->onHexRightClicked(selectedHex);
}

void BattleFieldController::renderBattlefield(Canvas & canvas)
{
	Canvas clippedCanvas(canvas, pos);

	showBackground(clippedCanvas);

	BattleRenderer renderer(owner);

	renderer.execute(clippedCanvas);

	owner.projectilesController->render(clippedCanvas);
}

void BattleFieldController::showBackground(Canvas & canvas)
{
	if (owner.stacksController->getActiveStack() != nullptr )
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
	canvas.draw(*backgroundWithHexes, Point(0, 0));
}

void BattleFieldController::redrawBackgroundWithHexes()
{
	const CStack *activeStack = owner.stacksController->getActiveStack();
	BattleHexArray attackableHexes;
	if(activeStack)
		occupiableHexes = owner.getBattle()->battleGetAvailableHexes(activeStack, false, true, &attackableHexes);

	// prepare background graphic with hexes and shaded hexes
	backgroundWithHexes->draw(background, Point(0,0));
	owner.obstacleController->showAbsoluteObstacles(*backgroundWithHexes);
	if(owner.siegeController)
		owner.siegeController->showAbsoluteObstacles(*backgroundWithHexes);

	// show shaded hexes for active's stack valid movement and the hexes that it can attack
	if(settings["battle"]["stackRange"].Bool())
	{
		BattleHexArray hexesToShade = occupiableHexes;
		hexesToShade.insert(attackableHexes);
		for(const BattleHex & hex : hexesToShade)
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

void BattleFieldController::showHighlightedHex(Canvas & canvas, std::shared_ptr<IImage> highlight, const BattleHex & hex, bool darkBorder)
{
	Point hexPos = hexPositionLocal(hex).topLeft();

	canvas.draw(highlight, hexPos);
	if(!darkBorder && settings["battle"]["cellBorders"].Bool())
		canvas.draw(cellBorder, hexPos);
}

BattleHexArray BattleFieldController::getHighlightedHexesForActiveStack()
{
	if(!owner.stacksController->getActiveStack())
		return BattleHexArray();

	if(!settings["battle"]["stackRange"].Bool())
		return BattleHexArray();

	auto hoveredHex = getHoveredHex();

	return owner.getBattle()->battleGetAttackedHexes(owner.stacksController->getActiveStack(), hoveredHex);
}

BattleHexArray BattleFieldController::getMovementRangeForHoveredStack()
{
	if (!owner.stacksController->getActiveStack())
		return BattleHexArray();

	if (!settings["battle"]["movementHighlightOnHover"].Bool() && !ENGINE->isKeyboardShiftDown())
		return BattleHexArray();

	auto hoveredStack = getHoveredStack();
	if(hoveredStack)
		return owner.getBattle()->battleGetAvailableHexes(hoveredStack, true, true, nullptr);
	else
		return BattleHexArray();
}

BattleHexArray BattleFieldController::getHighlightedHexesForSpellRange()
{
	BattleHexArray result;
	auto hoveredHex = getHoveredHex();

	const spells::Caster *caster = nullptr;
	const CSpell *spell = nullptr;

	spells::Mode mode = owner.actionsController->getCurrentCastMode();
	spell = owner.actionsController->getCurrentSpell(hoveredHex);
	caster = owner.actionsController->getCurrentSpellcaster();

	if(caster && spell) //when casting spell
	{
		// printing shaded hex(es)
		spells::BattleCast event(owner.getBattle().get(), caster, mode, spell);
		auto shadedHexes = spell->battleMechanics(&event)->rangeInHexes(hoveredHex);

		for(const BattleHex & shadedHex : shadedHexes)
		{
			if((shadedHex.getX() != 0) && (shadedHex.getX() != GameConstants::BFIELD_WIDTH - 1))
				result.insert(shadedHex);
		}
	}
	return result;
}

BattleHexArray BattleFieldController::getHighlightedHexesForMovementTarget()
{
	const CStack * stack = owner.stacksController->getActiveStack();
	auto hoveredHex = getHoveredHex();

	if(!stack)
		return {};

	BattleHexArray availableHexes = owner.getBattle()->battleGetAvailableHexes(stack, false, false, nullptr);

	auto hoveredStack = owner.getBattle()->battleGetStackByPos(hoveredHex, true);

	if(isTileAttackable(hoveredHex))
	{
		BattleHex attackFromHex = fromWhichHexAttack(hoveredHex);
		if(owner.getBattle()->battleCanAttack(stack, hoveredStack, attackFromHex))
		{
			if(stack->doubleWide())
				return {attackFromHex, stack->occupiedHex(attackFromHex)};
			else
				return {attackFromHex};
		}
	}

	if (stack->doubleWide())
	{
		const bool canMoveHeadHere = hoveredHex.isAvailable() && availableHexes.contains(hoveredHex);
		const bool canMoveTailHere = hoveredHex.isAvailable() && availableHexes.contains(hoveredHex.cloneInDirection(stack->destShiftDir()));
		const bool backwardsMove = stack->unitSide() == BattleSide::ATTACKER ?
									   hoveredHex.getX() < stack->getPosition().getX():
									   hoveredHex.getX() > stack->getPosition().getX();

		if(canMoveTailHere && (backwardsMove || !canMoveHeadHere))
			return {hoveredHex, hoveredHex.cloneInDirection(stack->destShiftDir())};

		if (canMoveHeadHere)
			return {hoveredHex, stack->occupiedHex(hoveredHex)};

		return {};
	}
	else
	{
		if (availableHexes.contains(hoveredHex))
			return {hoveredHex};
		else
			return {};
	}
}

// Range limit highlight helpers

BattleHexArray BattleFieldController::getRangeHexes(const BattleHex & sourceHex, uint8_t distance) const
{
	BattleHexArray rangeHexes;

	if (!settings["battle"]["rangeLimitHighlightOnHover"].Bool() && !ENGINE->isKeyboardShiftDown())
		return rangeHexes;

	// get only battlefield hexes that are within the given distance
	for(auto i = 0; i < GameConstants::BFIELD_SIZE; i++)
	{
		BattleHex hex(i);
		if(hex.isAvailable() && BattleHex::getDistance(sourceHex, hex) <= distance)
			rangeHexes.insert(hex);
	}

	return rangeHexes;
}

BattleHexArray BattleFieldController::getRangeLimitHexes(const BattleHex & sourceHex, const BattleHexArray & rangeHexes, uint8_t distanceToLimit) const
{
	BattleHexArray rangeLimitHexes;

	// from range hexes get only the ones at the limit
	for(auto & hex : rangeHexes)
	{
		if(BattleHex::getDistance(sourceHex, hex) == distanceToLimit)
			rangeLimitHexes.insert(hex);
	}

	return rangeLimitHexes;
}

bool BattleFieldController::isHexInRangeLimit(const BattleHex & hex, const BattleHexArray & rangeLimitHexes, int * hexIndexInRangeLimit) const
{
	bool  hexInRangeLimit = false;

	if(!rangeLimitHexes.empty())
	{
		auto pos = std::find(rangeLimitHexes.begin(), rangeLimitHexes.end(), hex);
		*hexIndexInRangeLimit = std::distance(rangeLimitHexes.begin(), pos);
		hexInRangeLimit = pos != rangeLimitHexes.end();
	}

	return hexInRangeLimit;
}

std::vector<std::vector<BattleHex::EDir>> BattleFieldController::getOutsideNeighbourDirectionsForLimitHexes(
	const BattleHexArray & wholeRangeHexes, const BattleHexArray & rangeLimitHexes) const
{
	std::vector<std::vector<BattleHex::EDir>> output;

	if(wholeRangeHexes.empty())
		return output;

	for(const auto & hex : rangeLimitHexes)
	{
		// get all neighbours and their directions
		
		const BattleHexArray & neighbouringTiles = hex.getAllNeighbouringTiles();

		std::vector<BattleHex::EDir> outsideNeighbourDirections;

		// for each neighbour add to output only the valid ones and only that are not found in range Hexes
		for(auto direction = 0; direction < 6; direction++)
		{
			if(!neighbouringTiles[direction].isAvailable())
				continue;

			if(!wholeRangeHexes.contains(neighbouringTiles[direction]))
				outsideNeighbourDirections.push_back(BattleHex::EDir(direction)); // push direction
		}

		output.push_back(outsideNeighbourDirections);
	}

	return output;
}

std::vector<std::shared_ptr<IImage>> BattleFieldController::calculateRangeLimitHighlightImages(std::vector<std::vector<BattleHex::EDir>> hexesNeighbourDirections, std::shared_ptr<CAnimation> limitImages)
{
	std::vector<std::shared_ptr<IImage>> output; // if no image is to be shown an empty image is still added to help with traverssing the range

	if(hexesNeighbourDirections.empty())
		return output;

	for(auto & directions : hexesNeighbourDirections)
	{
		std::bitset<6> mask;
		
		// convert directions to mask
		for(auto direction : directions)
			mask.set(direction);

		uint8_t imageKey = static_cast<uint8_t>(mask.to_ulong());
		output.push_back(limitImages->getImage(hexEdgeMaskToFrameIndex.at(imageKey)));
	}

	return output;
}

void BattleFieldController::calculateRangeLimitAndHighlightImages(uint8_t distance, std::shared_ptr<CAnimation> rangeLimitImages, BattleHexArray & rangeLimitHexes, std::vector<std::shared_ptr<IImage>> & rangeLimitHexesHighlights)
{
		BattleHexArray rangeHexes = getRangeHexes(hoveredHex, distance);
		rangeLimitHexes = getRangeLimitHexes(hoveredHex, rangeHexes, distance);
		std::vector<std::vector<BattleHex::EDir>> rangeLimitNeighbourDirections = getOutsideNeighbourDirectionsForLimitHexes(rangeHexes, rangeLimitHexes);
		rangeLimitHexesHighlights = calculateRangeLimitHighlightImages(rangeLimitNeighbourDirections, rangeLimitImages);
}

void BattleFieldController::showHighlightedHexes(Canvas & canvas)
{
	BattleHexArray rangedFullDamageLimitHexes;
	BattleHexArray shootingRangeLimitHexes;

	std::vector<std::shared_ptr<IImage>> rangedFullDamageLimitHexesHighlights;
	std::vector<std::shared_ptr<IImage>> shootingRangeLimitHexesHighlights;

	BattleHexArray hoveredStackMovementRangeHexes = getMovementRangeForHoveredStack();
	BattleHexArray hoveredSpellHexes = getHighlightedHexesForSpellRange();
	BattleHexArray hoveredMoveHexes  = getHighlightedHexesForMovementTarget();

	BattleHex hoveredHex = getHoveredHex();
	BattleHexArray hoveredMouseHex = hoveredHex.isAvailable() ? BattleHexArray({ hoveredHex }) : BattleHexArray();

	const CStack * hoveredStack = getHoveredStack();
	if(!hoveredStack && hoveredHex == BattleHex::INVALID)
		return;

	// skip range limit calculations if unit hovered is not a shooter
	if(hoveredStack && hoveredStack->isShooter())
	{
		// calculate array with highlight images for ranged full damage limit
		auto rangedFullDamageDistance = hoveredStack->getRangedFullDamageDistance();
		calculateRangeLimitAndHighlightImages(rangedFullDamageDistance, rangedFullDamageLimitImages, rangedFullDamageLimitHexes, rangedFullDamageLimitHexesHighlights);

		// calculate array with highlight images for shooting range limit
		auto shootingRangeDistance = hoveredStack->getShootingRangeDistance();
		calculateRangeLimitAndHighlightImages(shootingRangeDistance, shootingRangeLimitImages, shootingRangeLimitHexes, shootingRangeLimitHexesHighlights);
	}

	bool useSpellRangeForMouse = hoveredHex != BattleHex::INVALID
		&& (owner.actionsController->currentActionSpellcasting(getHoveredHex())
			|| owner.actionsController->creatureSpellcastingModeActive()); //at least shooting with SPELL_LIKE_ATTACK can operate in spellcasting mode without being actual spellcast
	bool useMoveRangeForMouse = !hoveredMoveHexes.empty() || !settings["battle"]["mouseShadow"].Bool();

	const auto & hoveredMouseHexes = useSpellRangeForMouse ? hoveredSpellHexes : ( useMoveRangeForMouse ? hoveredMoveHexes : hoveredMouseHex);

	for(int hex = 0; hex < GameConstants::BFIELD_SIZE; ++hex)
	{
		bool stackMovement = hoveredStackMovementRangeHexes.contains(hex);
		bool mouse = hoveredMouseHexes.contains(hex);

		// calculate if hex is Ranged Full Damage Limit and its position in highlight array
		int hexIndexInRangedFullDamageLimit = 0;
		bool hexInRangedFullDamageLimit = isHexInRangeLimit(hex, rangedFullDamageLimitHexes, &hexIndexInRangedFullDamageLimit);

		// calculate if hex is Shooting Range Limit and its position in highlight array
		int hexIndexInShootingRangeLimit = 0;
		bool hexInShootingRangeLimit = isHexInRangeLimit(hex, shootingRangeLimitHexes, &hexIndexInShootingRangeLimit);

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
		if(hexInRangedFullDamageLimit)
		{
			showHighlightedHex(canvas, rangedFullDamageLimitHexesHighlights[hexIndexInRangedFullDamageLimit], hex, false);
		}
		if(hexInShootingRangeLimit)
		{
			showHighlightedHex(canvas, shootingRangeLimitHexesHighlights[hexIndexInShootingRangeLimit], hex, false);
		}
	}
}

Rect BattleFieldController::hexPositionLocal(const BattleHex & hex) const
{
	int x = 14 + ((hex.getY())%2==0 ? 22 : 0) + 44*hex.getX();
	int y = 86 + 42 *hex.getY();
	int w = cellShade->width();
	int h = cellShade->height();
	return Rect(x, y, w, h);
}

Rect BattleFieldController::hexPositionAbsolute(const BattleHex & hex) const
{
	return hexPositionLocal(hex) + pos.topLeft();
}

bool BattleFieldController::isPixelInHex(Point const & position)
{
	return !cellShade->isTransparent(position);
}

BattleHex BattleFieldController::getHoveredHex()
{
	return hoveredHex;
}

const CStack* BattleFieldController::getHoveredStack()
{
	auto hoveredHex = getHoveredHex();
	const CStack* hoveredStack = owner.getBattle()->battleGetStackByPos(hoveredHex, true);

	if(owner.windowObject->getQueueHoveredUnitId().has_value())
	{
		auto stacks = owner.getBattle()->battleGetAllStacks();
		for(const CStack * stack : stacks)
			if(stack->unitId() == *owner.windowObject->getQueueHoveredUnitId())
				hoveredStack = stack;
	}

	return hoveredStack;
}

BattleHex BattleFieldController::getHexAtPosition(Point hoverPos)
{
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

BattleHex::EDir BattleFieldController::selectAttackDirection(const BattleHex & myNumber) const
{
	const bool doubleWide = owner.stacksController->getActiveStack()->doubleWide();
	const BattleHexArray & neighbours = myNumber.getAllNeighbouringTiles();
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
		{
			BattleHex target = neighbours[i].cloneInDirection(BattleHex::RIGHT, false);
			attackAvailability[i] = neighbours[i].isValid() && occupiableHexes.contains(neighbours[i]) && target.isValid() && occupiableHexes.contains(target);
		}

		for (size_t i : { 4, 5, 0})
		{
			BattleHex target = neighbours[i].cloneInDirection(BattleHex::LEFT, false);
			attackAvailability[i] = neighbours[i].isValid() && occupiableHexes.contains(neighbours[i]) && target.isValid() && occupiableHexes.contains(target);
		}

		attackAvailability[6] = neighbours[0].isValid() && neighbours[1].isValid() && occupiableHexes.contains(neighbours[0]) && occupiableHexes.contains(neighbours[1]);
		attackAvailability[7] = neighbours[3].isValid() && neighbours[4].isValid() && occupiableHexes.contains(neighbours[3]) && occupiableHexes.contains(neighbours[4]);
	}
	else
	{
		for (size_t i = 0; i < 6; ++i)
			attackAvailability[i] = neighbours[i].isValid() && occupiableHexes.contains(neighbours[i]);

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
			distance2[i] = (testPoint[i].y - currentAttackOriginPoint.y)*(testPoint[i].y - currentAttackOriginPoint.y) + (testPoint[i].x - currentAttackOriginPoint.x)*(testPoint[i].x - currentAttackOriginPoint.x);

	size_t nearest = -1;
	for (size_t i = 0; i < 8; ++i)
		if (attackAvailability[i] && (nearest == -1 || distance2[i] < distance2[nearest]) )
			nearest = i;

	assert(nearest != -1);
	return BattleHex::EDir(nearest);
}

BattleHex BattleFieldController::fromWhichHexAttack(const BattleHex & attackTarget)
{
	BattleHex::EDir direction = selectAttackDirection(attackTarget);

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
	if(!number.isValid())
		return false;

	for (auto & elem : occupiableHexes)
	{
		if (BattleHex::mutualPosition(elem, number) != BattleHex::EDir::NONE || elem == number)
			return true;
	}
	return false;
}

void BattleFieldController::updateAccessibleHexes()
{
	auto accessibility = owner.getBattle()->getAccessibility();

	for(int i = 0; i < accessibility.size(); i++)
		stackCountOutsideHexes[i] = (accessibility[i] == EAccessibility::ACCESSIBLE || (accessibility[i] == EAccessibility::SIDE_COLUMN));
}

bool BattleFieldController::stackCountOutsideHex(const BattleHex & number) const
{
	return stackCountOutsideHexes[number.toInt()];
}

void BattleFieldController::showAll(Canvas & to)
{
	show(to);
}

void BattleFieldController::tick(uint32_t msPassed)
{
	updateAccessibleHexes();
	owner.stacksController->tick(msPassed);
	owner.obstacleController->tick(msPassed);
	owner.projectilesController->tick(msPassed);
}

void BattleFieldController::show(Canvas & to)
{
	CanvasClipRectGuard guard(to, pos);

	renderBattlefield(to);

	if (isActive() && isGesturing() && getHoveredHex() != BattleHex::INVALID)
		to.draw(ENGINE->cursor().getCurrentImage(), hexPositionAbsolute(getHoveredHex()).center() - ENGINE->cursor().getPivotOffset());
}

bool BattleFieldController::receiveEvent(const Point & position, int eventType) const
{
	if (eventType == HOVER)
		return true;
	return CIntObject::receiveEvent(position, eventType);
}
