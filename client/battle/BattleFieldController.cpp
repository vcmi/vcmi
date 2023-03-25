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

#include <SDL_events.h>

BattleFieldController::BattleFieldController(BattleInterface & owner):
	owner(owner)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL_NO_DISPOSE;
	strongInterest = true;

	//preparing cells and hexes
	cellBorder = IImage::createFromFile("CCELLGRD.BMP", EImageBlitMode::COLORKEY);
	cellShade = IImage::createFromFile("CCELLSHD.BMP");

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

	auto accessibility = owner.curInt->cb->getAccesibility();
	for(int i = 0; i < accessibility.size(); i++)
		stackCountOutsideHexes[i] = (accessibility[i] == EAccessibility::ACCESSIBLE);

	addUsedEvents(LCLICK | RCLICK | MOVE);
	LOCPLINT->cingconsole->pos = this->pos;
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
	if (owner.stacksController->getActiveStack() != nullptr ) //&& creAnims[stacksController->getActiveStack()->ID]->isIdle() //show everything with range
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
	if (activeStack)
		occupyableHexes = owner.curInt->cb->battleGetAvailableHexes(activeStack, true, &attackableHexes);

	auto accessibility = owner.curInt->cb->getAccesibility();

	for(int i = 0; i < accessibility.size(); i++)
		stackCountOutsideHexes[i] = (accessibility[i] == EAccessibility::ACCESSIBLE);

	//prepare background graphic with hexes and shaded hexes
	backgroundWithHexes->draw(background, Point(0,0));
	owner.obstacleController->showAbsoluteObstacles(*backgroundWithHexes);
	if ( owner.siegeController )
		owner.siegeController->showAbsoluteObstacles(*backgroundWithHexes);

	if (settings["battle"]["stackRange"].Bool())
	{
		std::vector<BattleHex> hexesToShade = occupyableHexes;
		hexesToShade.insert(hexesToShade.end(), attackableHexes.begin(), attackableHexes.end());
		for (BattleHex hex : hexesToShade)
		{
			backgroundWithHexes->draw(cellShade, hexPositionLocal(hex).topLeft());
		}
	}

	if(settings["battle"]["cellBorders"].Bool())
	{
		for (int i=0; i<GameConstants::BFIELD_SIZE; ++i)
		{
			if ( i % GameConstants::BFIELD_WIDTH == 0)
				continue;
			if ( i % GameConstants::BFIELD_WIDTH == GameConstants::BFIELD_WIDTH - 1)
				continue;

			backgroundWithHexes->draw(cellBorder, hexPositionLocal(i).topLeft());
		}
	}
}

void BattleFieldController::showHighlightedHex(Canvas & canvas, BattleHex hex, bool darkBorder)
{
	Point hexPos = hexPositionLocal(hex).topLeft();

	canvas.draw(cellShade, hexPos);
	if(!darkBorder && settings["battle"]["cellBorders"].Bool())
		canvas.draw(cellBorder, hexPos);
}

std::set<BattleHex> BattleFieldController::getHighlightedHexesStackRange()
{
	std::set<BattleHex> result;

	if ( !owner.stacksController->getActiveStack())
		return result;

	if ( !settings["battle"]["stackRange"].Bool())
		return result;

	auto hoveredHex = getHoveredHex();

	std::set<BattleHex> set = owner.curInt->cb->battleGetAttackedHexes(owner.stacksController->getActiveStack(), hoveredHex, attackingHex);
	for(BattleHex hex : set)
		result.insert(hex);

	// display the movement shadow of stack under mouse
	const CStack * const hoveredStack = owner.curInt->cb->battleGetStackByPos(hoveredHex, true);
	if(hoveredStack && hoveredStack != owner.stacksController->getActiveStack())
	{
		std::vector<BattleHex> v = owner.curInt->cb->battleGetAvailableHexes(hoveredStack, true, nullptr);
		for(BattleHex hex : v)
			result.insert(hex);
	}
	return result;
}

std::set<BattleHex> BattleFieldController::getHighlightedHexesSpellRange()
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
		auto shaded = spell->battleMechanics(&event)->rangeInHexes(hoveredHex);

		for(BattleHex shadedHex : shaded)
		{
			if((shadedHex.getX() != 0) && (shadedHex.getX() != GameConstants::BFIELD_WIDTH - 1))
				result.insert(shadedHex);
		}
	}
	return result;
}

std::set<BattleHex> BattleFieldController::getHighlightedHexesMovementTarget()
{
	const CStack * stack = owner.stacksController->getActiveStack();
	auto hoveredHex = getHoveredHex();

	if (stack)
	{
		std::vector<BattleHex> v = owner.curInt->cb->battleGetAvailableHexes(stack, false, nullptr);

		auto hoveredStack = owner.curInt->cb->battleGetStackByPos(hoveredHex, true);
		if(owner.curInt->cb->battleCanAttack(stack, hoveredStack, hoveredHex))
		{
			if (isTileAttackable(hoveredHex))
			{
				BattleHex attackFromHex = fromWhichHexAttack(hoveredHex);

				if (stack->doubleWide())
					return {attackFromHex, stack->occupiedHex(attackFromHex)};
				else
					return {attackFromHex};
			}
		}

		if (vstd::contains(v,hoveredHex))
		{
			if (stack->doubleWide())
				return {hoveredHex, stack->occupiedHex(hoveredHex)};
			else
				return {hoveredHex};
		}
		if (stack->doubleWide())
		{
			for (auto const & hex : v)
			{
				if (stack->occupiedHex(hex) == hoveredHex)
					return { hoveredHex, hex };
			}
		}
	}
	return {};
}

void BattleFieldController::showHighlightedHexes(Canvas & canvas)
{
	std::set<BattleHex> hoveredStack = getHighlightedHexesStackRange();
	std::set<BattleHex> hoveredSpell = getHighlightedHexesSpellRange();
	std::set<BattleHex> hoveredMove  = getHighlightedHexesMovementTarget();

	if (getHoveredHex() == BattleHex::INVALID)
		return;

	auto const & hoveredMouse = owner.actionsController->currentActionSpellcasting(getHoveredHex()) ? hoveredSpell : hoveredMove;

	for(int b=0; b<GameConstants::BFIELD_SIZE; ++b)
	{
		bool stack = hoveredStack.count(b);
		bool mouse = hoveredMouse.count(b);

		if ( stack && mouse )
		{
			// area where enemy stack can move AND affected by mouse cursor - create darker highlight by blitting twice
			showHighlightedHex(canvas, b, true);
			showHighlightedHex(canvas, b, true);
		}
		if ( !stack && mouse )
		{
			showHighlightedHex(canvas, b, true);
		}
		if ( stack && !mouse )
		{
			showHighlightedHex(canvas, b, false);
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
			attackAvailability[i] = vstd::contains(occupyableHexes, neighbours[i]) && vstd::contains(occupyableHexes, neighbours[i].cloneInDirection(BattleHex::RIGHT, false));

		for (size_t i : { 4, 5, 0})
			attackAvailability[i] = vstd::contains(occupyableHexes, neighbours[i]) && vstd::contains(occupyableHexes, neighbours[i].cloneInDirection(BattleHex::LEFT, false));

		attackAvailability[6] = vstd::contains(occupyableHexes, neighbours[0]) && vstd::contains(occupyableHexes, neighbours[1]);
		attackAvailability[7] = vstd::contains(occupyableHexes, neighbours[3]) && vstd::contains(occupyableHexes, neighbours[4]);
	}
	else
	{
		for (size_t i = 0; i < 6; ++i)
			attackAvailability[i] = vstd::contains(occupyableHexes, neighbours[i]);

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
			if ( attacker->side == BattleSide::ATTACKER )
				return attackTarget.cloneInDirection(direction);
			else
				return attackTarget.cloneInDirection(direction).cloneInDirection(BattleHex::LEFT);
		}

		case BattleHex::TOP_RIGHT:
		case BattleHex::RIGHT:
		case BattleHex::BOTTOM_RIGHT:
		{
			if ( attacker->side == BattleSide::ATTACKER )
				return attackTarget.cloneInDirection(direction).cloneInDirection(BattleHex::RIGHT);
			else
				return attackTarget.cloneInDirection(direction);
		}

		case BattleHex::TOP:
		{
			if ( attacker->side == BattleSide::ATTACKER )
				return attackTarget.cloneInDirection(BattleHex::TOP_RIGHT);
			else
				return attackTarget.cloneInDirection(BattleHex::TOP_LEFT);
		}

		case BattleHex::BOTTOM:
		{
			if ( attacker->side == BattleSide::ATTACKER )
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
	for (auto & elem : occupyableHexes)
	{
		if (BattleHex::mutualPosition(elem, number) != -1 || elem == number)
			return true;
	}
	return false;
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
	owner.stacksController->update();
	owner.obstacleController->update();

	Canvas canvas(to);
	CSDL_Ext::CClipRectGuard guard(to, pos);

	renderBattlefield(canvas);
}
