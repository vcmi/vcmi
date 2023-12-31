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
#include "../render/CAnimation.h"
#include "../render/Canvas.h"
#include "../render/IImage.h"
#include "../renderSDL/SDL_Extensions.h"
#include "../render/IRenderHandler.h"
#include "../gui/CGuiHandler.h"
#include "../gui/CursorHandler.h"
#include "../adventureMap/CInGameConsole.h"
#include "../client/render/CAnimation.h"

#include "../../CCallback.h"
#include "../../lib/BattleFieldHandler.h"
#include "../../lib/CConfigHandler.h"
#include "../../lib/CStack.h"
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

std::map<int, int> hexEdgeMaskToFrameIndex;

// Maps HexEdgesMask to "Frame" indexes for range highligt images
void initializeHexEdgeMaskToFrameIndex()
{
	hexEdgeMaskToFrameIndex[HexMasks::empty] = 0;

    hexEdgeMaskToFrameIndex[HexMasks::topLeft] = 1;
    hexEdgeMaskToFrameIndex[HexMasks::topRight] = 2;
    hexEdgeMaskToFrameIndex[HexMasks::right] = 3;
    hexEdgeMaskToFrameIndex[HexMasks::bottomRight] = 4;
    hexEdgeMaskToFrameIndex[HexMasks::bottomLeft] = 5;
    hexEdgeMaskToFrameIndex[HexMasks::left] = 6;

    hexEdgeMaskToFrameIndex[HexMasks::top] = 7;
    hexEdgeMaskToFrameIndex[HexMasks::bottom] = 8;

    hexEdgeMaskToFrameIndex[HexMasks::topRightHalfCorner] = 9;
    hexEdgeMaskToFrameIndex[HexMasks::bottomRightHalfCorner] = 10;
    hexEdgeMaskToFrameIndex[HexMasks::bottomLeftHalfCorner] = 11;
    hexEdgeMaskToFrameIndex[HexMasks::topLeftHalfCorner] = 12;

    hexEdgeMaskToFrameIndex[HexMasks::rightTopAndBottom] = 13;
    hexEdgeMaskToFrameIndex[HexMasks::leftTopAndBottom] = 14;
	
    hexEdgeMaskToFrameIndex[HexMasks::rightHalf] = 13;
    hexEdgeMaskToFrameIndex[HexMasks::leftHalf] = 14;

    hexEdgeMaskToFrameIndex[HexMasks::topRightCorner] = 15;
    hexEdgeMaskToFrameIndex[HexMasks::bottomRightCorner] = 16;
    hexEdgeMaskToFrameIndex[HexMasks::bottomLeftCorner] = 17;
    hexEdgeMaskToFrameIndex[HexMasks::topLeftCorner] = 18;
}

BattleFieldController::BattleFieldController(BattleInterface & owner):
	owner(owner)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL_NO_DISPOSE;

	//preparing cells and hexes
	cellBorder = GH.renderHandler().loadImage(ImagePath::builtin("CCELLGRD.BMP"), EImageBlitMode::COLORKEY);
	cellShade = GH.renderHandler().loadImage(ImagePath::builtin("CCELLSHD.BMP"));
	cellUnitMovementHighlight = GH.renderHandler().loadImage(ImagePath::builtin("UnitMovementHighlight.PNG"), EImageBlitMode::COLORKEY);
	cellUnitMaxMovementHighlight = GH.renderHandler().loadImage(ImagePath::builtin("UnitMaxMovementHighlight.PNG"), EImageBlitMode::COLORKEY);

	attackCursors = GH.renderHandler().loadAnimation(AnimationPath::builtin("CRCOMBAT"));
	attackCursors->preload();

	spellCursors = GH.renderHandler().loadAnimation(AnimationPath::builtin("CRSPELL"));
	spellCursors->preload();

	initializeHexEdgeMaskToFrameIndex();

	rangedFullDamageLimitImages = GH.renderHandler().loadAnimation(AnimationPath::builtin("battle/rangeHighlights/rangeHighlightsGreen.json"));
	rangedFullDamageLimitImages->preload();

	shootingRangeLimitImages = GH.renderHandler().loadAnimation(AnimationPath::builtin("battle/rangeHighlights/rangeHighlightsRed.json"));
	shootingRangeLimitImages->preload();

	flipRangeLimitImagesIntoPositions(rangedFullDamageLimitImages);
	flipRangeLimitImagesIntoPositions(shootingRangeLimitImages);

	if(!owner.siegeController)
	{
		auto bfieldType = owner.getBattle()->battleGetBattlefieldType();

		if(bfieldType == BattleField::NONE)
			logGlobal->error("Invalid battlefield returned for current battle");
		else
			background = GH.renderHandler().loadImage(bfieldType.getInfo()->graphics, EImageBlitMode::OPAQUE);
	}
	else
	{
		auto backgroundName = owner.siegeController->getBattleBackgroundName();
		background = GH.renderHandler().loadImage(backgroundName, EImageBlitMode::OPAQUE);
	}

	pos.w = background->width();
	pos.h = background->height();

	backgroundWithHexes = std::make_unique<Canvas>(Point(background->width(), background->height()));

	updateAccessibleHexes();
	addUsedEvents(LCLICK | SHOW_POPUP | MOVE | TIME | GESTURE);
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
	std::vector<BattleHex> attackableHexes;
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

	std::set<BattleHex> set = owner.getBattle()->battleGetAttackedHexes(owner.stacksController->getActiveStack(), hoveredHex);
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
	const CStack * const hoveredStack = owner.getBattle()->battleGetStackByPos(hoveredHex, true);
	if(hoveredStack)
	{
		std::vector<BattleHex> v = owner.getBattle()->battleGetAvailableHexes(hoveredStack, true, true, nullptr);
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
		spells::BattleCast event(owner.getBattle().get(), caster, mode, spell);
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

	std::vector<BattleHex> availableHexes = owner.getBattle()->battleGetAvailableHexes(stack, false, false, nullptr);

	auto hoveredStack = owner.getBattle()->battleGetStackByPos(hoveredHex, true);
	if(owner.getBattle()->battleCanAttack(stack, hoveredStack, hoveredHex))
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

// Range limit highlight helpers

std::vector<BattleHex> BattleFieldController::getRangeHexes(BattleHex sourceHex, uint8_t distance)
{
	std::vector<BattleHex> rangeHexes;

	if (!settings["battle"]["rangeLimitHighlightOnHover"].Bool() && !GH.isKeyboardShiftDown())
		return rangeHexes;

	// get only battlefield hexes that are within the given distance
	for(auto i = 0; i < GameConstants::BFIELD_SIZE; i++)
	{
		BattleHex hex(i);
		if(hex.isAvailable() && BattleHex::getDistance(sourceHex, hex) <= distance)
			rangeHexes.push_back(hex);
	}

	return rangeHexes;
}

std::vector<BattleHex> BattleFieldController::getRangeLimitHexes(BattleHex hoveredHex, std::vector<BattleHex> rangeHexes, uint8_t distanceToLimit)
{
	std::vector<BattleHex> rangeLimitHexes;

	// from range hexes get only the ones at the limit
	for(auto & hex : rangeHexes)
	{
		if(BattleHex::getDistance(hoveredHex, hex) == distanceToLimit)
			rangeLimitHexes.push_back(hex);
	}

	return rangeLimitHexes;
}

bool BattleFieldController::IsHexInRangeLimit(BattleHex hex, std::vector<BattleHex> & rangeLimitHexes, int * hexIndexInRangeLimit)
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

std::vector<std::vector<BattleHex::EDir>> BattleFieldController::getOutsideNeighbourDirectionsForLimitHexes(std::vector<BattleHex> wholeRangeHexes, std::vector<BattleHex> rangeLimitHexes)
{
	std::vector<std::vector<BattleHex::EDir>> output;

	if(wholeRangeHexes.empty())
		return output;

	for(auto & hex : rangeLimitHexes)
	{
		// get all neighbours and their directions
		
		auto neighbouringTiles = hex.allNeighbouringTiles();

		std::vector<BattleHex::EDir> outsideNeighbourDirections;

		// for each neighbour add to output only the valid ones and only that are not found in range Hexes
		for(auto direction = 0; direction < 6; direction++)
		{
			if(!neighbouringTiles[direction].isAvailable())
				continue;

			auto it = std::find(wholeRangeHexes.begin(), wholeRangeHexes.end(), neighbouringTiles[direction]);

			if(it == wholeRangeHexes.end())
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
		output.push_back(limitImages->getImage(hexEdgeMaskToFrameIndex[imageKey]));
	}

	return output;
}

void BattleFieldController::calculateRangeLimitAndHighlightImages(uint8_t distance, std::shared_ptr<CAnimation> rangeLimitImages, std::vector<BattleHex> & rangeLimitHexes, std::vector<std::shared_ptr<IImage>> & rangeLimitHexesHighligts)
{
		std::vector<BattleHex> rangeHexes = getRangeHexes(hoveredHex, distance);
		rangeLimitHexes = getRangeLimitHexes(hoveredHex, rangeHexes, distance);
		std::vector<std::vector<BattleHex::EDir>> rangeLimitNeighbourDirections = getOutsideNeighbourDirectionsForLimitHexes(rangeHexes, rangeLimitHexes);
		rangeLimitHexesHighligts = calculateRangeLimitHighlightImages(rangeLimitNeighbourDirections, rangeLimitImages);
}

void BattleFieldController::flipRangeLimitImagesIntoPositions(std::shared_ptr<CAnimation> images)
{
	images->getImage(hexEdgeMaskToFrameIndex[HexMasks::topRight])->verticalFlip();
	images->getImage(hexEdgeMaskToFrameIndex[HexMasks::right])->verticalFlip();
	images->getImage(hexEdgeMaskToFrameIndex[HexMasks::bottomRight])->doubleFlip();
	images->getImage(hexEdgeMaskToFrameIndex[HexMasks::bottomLeft])->horizontalFlip();

	images->getImage(hexEdgeMaskToFrameIndex[HexMasks::bottom])->horizontalFlip();

	images->getImage(hexEdgeMaskToFrameIndex[HexMasks::topRightHalfCorner])->verticalFlip();
	images->getImage(hexEdgeMaskToFrameIndex[HexMasks::bottomRightHalfCorner])->doubleFlip();
	images->getImage(hexEdgeMaskToFrameIndex[HexMasks::bottomLeftHalfCorner])->horizontalFlip();

	images->getImage(hexEdgeMaskToFrameIndex[HexMasks::rightHalf])->verticalFlip();

	images->getImage(hexEdgeMaskToFrameIndex[HexMasks::topRightCorner])->verticalFlip();
	images->getImage(hexEdgeMaskToFrameIndex[HexMasks::bottomRightCorner])->doubleFlip();
	images->getImage(hexEdgeMaskToFrameIndex[HexMasks::bottomLeftCorner])->horizontalFlip();
}

void BattleFieldController::showHighlightedHexes(Canvas & canvas)
{
	std::vector<BattleHex> rangedFullDamageLimitHexes;
	std::vector<BattleHex> shootingRangeLimitHexes;

	std::vector<std::shared_ptr<IImage>> rangedFullDamageLimitHexesHighligts;
	std::vector<std::shared_ptr<IImage>> shootingRangeLimitHexesHighligts;

	std::set<BattleHex> hoveredStackMovementRangeHexes = getMovementRangeForHoveredStack();
	std::set<BattleHex> hoveredSpellHexes = getHighlightedHexesForSpellRange();
	std::set<BattleHex> hoveredMoveHexes  = getHighlightedHexesForMovementTarget();

	BattleHex hoveredHex = getHoveredHex();
	if(hoveredHex == BattleHex::INVALID)
		return;

	const CStack * hoveredStack = getHoveredStack();

	// skip range limit calculations if unit hovered is not a shooter
	if(hoveredStack && hoveredStack->isShooter())
	{
		// calculate array with highlight images for ranged full damage limit
		auto rangedFullDamageDistance = hoveredStack->getRangedFullDamageDistance();
		calculateRangeLimitAndHighlightImages(rangedFullDamageDistance, rangedFullDamageLimitImages, rangedFullDamageLimitHexes, rangedFullDamageLimitHexesHighligts);

		// calculate array with highlight images for shooting range limit
		auto shootingRangeDistance = hoveredStack->getShootingRangeDistance();
		calculateRangeLimitAndHighlightImages(shootingRangeDistance, shootingRangeLimitImages, shootingRangeLimitHexes, shootingRangeLimitHexesHighligts);
	}

	auto const & hoveredMouseHexes = owner.actionsController->currentActionSpellcasting(getHoveredHex()) ? hoveredSpellHexes : hoveredMoveHexes;

	for(int hex = 0; hex < GameConstants::BFIELD_SIZE; ++hex)
	{
		bool stackMovement = hoveredStackMovementRangeHexes.count(hex);
		bool mouse = hoveredMouseHexes.count(hex);

		// calculate if hex is Ranged Full Damage Limit and its position in highlight array
		int hexIndexInRangedFullDamageLimit = 0;
		bool hexInRangedFullDamageLimit = IsHexInRangeLimit(hex, rangedFullDamageLimitHexes, &hexIndexInRangedFullDamageLimit);

		// calculate if hex is Shooting Range Limit and its position in highlight array
		int hexIndexInShootingRangeLimit = 0;
		bool hexInShootingRangeLimit = IsHexInRangeLimit(hex, shootingRangeLimitHexes, &hexIndexInShootingRangeLimit);

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
			showHighlightedHex(canvas, rangedFullDamageLimitHexesHighligts[hexIndexInRangedFullDamageLimit], hex, false);
		}
		if(hexInShootingRangeLimit)
		{
			showHighlightedHex(canvas, shootingRangeLimitHexesHighligts[hexIndexInShootingRangeLimit], hex, false);
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
	return hoveredHex;
}

const CStack* BattleFieldController::getHoveredStack()
{
	auto hoveredHex = getHoveredHex();
	const CStack* hoveredStack = owner.getBattle()->battleGetStackByPos(hoveredHex, true);

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

BattleHex::EDir BattleFieldController::selectAttackDirection(BattleHex myNumber)
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
			distance2[i] = (testPoint[i].y - currentAttackOriginPoint.y)*(testPoint[i].y - currentAttackOriginPoint.y) + (testPoint[i].x - currentAttackOriginPoint.x)*(testPoint[i].x - currentAttackOriginPoint.x);

	size_t nearest = -1;
	for (size_t i = 0; i < 8; ++i)
		if (attackAvailability[i] && (nearest == -1 || distance2[i] < distance2[nearest]) )
			nearest = i;

	assert(nearest != -1);
	return BattleHex::EDir(nearest);
}

BattleHex BattleFieldController::fromWhichHexAttack(BattleHex attackTarget)
{
	BattleHex::EDir direction = selectAttackDirection(getHoveredHex());

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
	auto accessibility = owner.getBattle()->getAccesibility();

	for(int i = 0; i < accessibility.size(); i++)
		stackCountOutsideHexes[i] = (accessibility[i] == EAccessibility::ACCESSIBLE || (accessibility[i] == EAccessibility::SIDE_COLUMN));
}

bool BattleFieldController::stackCountOutsideHex(const BattleHex & number) const
{
	return stackCountOutsideHexes[number];
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
	CSDL_Ext::CClipRectGuard guard(to.getInternalSurface(), pos);

	renderBattlefield(to);

	if (isActive() && isGesturing() && getHoveredHex() != BattleHex::INVALID)
	{
		auto combatCursorIndex = CCS->curh->get<Cursor::Combat>();
		if (combatCursorIndex)
		{
			auto combatImageIndex = static_cast<size_t>(*combatCursorIndex);
			to.draw(attackCursors->getImage(combatImageIndex), hexPositionAbsolute(getHoveredHex()).center() - CCS->curh->getPivotOffsetCombat(combatImageIndex));
			return;
		}

		auto spellCursorIndex = CCS->curh->get<Cursor::Spellcast>();
		if (spellCursorIndex)
		{
			auto spellImageIndex = static_cast<size_t>(*spellCursorIndex);
			to.draw(spellCursors->getImage(spellImageIndex), hexPositionAbsolute(getHoveredHex()).center() - CCS->curh->getPivotOffsetSpellcast());
			return;
		}

	}
}

bool BattleFieldController::receiveEvent(const Point & position, int eventType) const
{
	if (eventType == HOVER)
		return true;
	return CIntObject::receiveEvent(position, eventType);
}
