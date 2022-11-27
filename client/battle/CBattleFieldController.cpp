/*
 * CBattleFieldController.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "CBattleFieldController.h"
#include "CBattleInterface.h"
#include "CBattleActionsController.h"
#include "CBattleInterfaceClasses.h"
#include "CBattleSiegeController.h"
#include "CBattleStacksController.h"
#include "CBattleObstacleController.h"
#include "../CGameInfo.h"
#include "../../CCallback.h"
#include "../gui/CAnimation.h"
#include "../gui/CCanvas.h"
#include "../gui/CGuiHandler.h"
#include "../CPlayerInterface.h"
#include "../gui/CCursorHandler.h"
#include "../../lib/BattleFieldHandler.h"
#include "../../lib/CConfigHandler.h"
#include "../../lib/CStack.h"
#include "../../lib/spells/ISpellMechanics.h"

CBattleFieldController::CBattleFieldController(CBattleInterface * owner):
	owner(owner),
	attackingHex(BattleHex::INVALID)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL_NO_DISPOSE;
	pos.w = owner->pos.w;
	pos.h = owner->pos.h;

	//preparing cells and hexes
	cellBorder = IImage::createFromFile("CCELLGRD.BMP");
	cellShade = IImage::createFromFile("CCELLSHD.BMP");

	if(!owner->siegeController)
	{
		auto bfieldType = owner->curInt->cb->battleGetBattlefieldType();

		if(bfieldType == BattleField::NONE)
			logGlobal->error("Invalid battlefield returned for current battle");
		else
			background = IImage::createFromFile(bfieldType.getInfo()->graphics);
	}
	else
	{
		std::string backgroundName = owner->siegeController->getBattleBackgroundName();
		background = IImage::createFromFile(backgroundName);
	}

	//preparing graphic with cell borders
	cellBorders = std::make_shared<CCanvas>(Point(background->width(), background->height()));

	for (int i=0; i<GameConstants::BFIELD_SIZE; ++i)
	{
		if ( i % GameConstants::BFIELD_WIDTH == 0)
			continue;
		if ( i % GameConstants::BFIELD_WIDTH == GameConstants::BFIELD_WIDTH - 1)
			continue;

		cellBorders->draw(cellBorder, hexPositionLocal(i).topLeft());
	}

	backgroundWithHexes = std::make_shared<CCanvas>(Point(background->width(), background->height()));

	for (int h = 0; h < GameConstants::BFIELD_SIZE; ++h)
	{
		auto hex = std::make_shared<CClickableHex>();
		hex->myNumber = h;
		hex->pos = hexPosition(h);
		hex->myInterface = owner;
		bfield.push_back(hex);
	}

	auto accessibility = owner->curInt->cb->getAccesibility();
	for(int i = 0; i < accessibility.size(); i++)
		stackCountOutsideHexes[i] = (accessibility[i] == EAccessibility::ACCESSIBLE);
}

void CBattleFieldController::showBackgroundImage(std::shared_ptr<CCanvas> canvas)
{
	canvas->draw(background, owner->pos.topLeft());

	if (settings["battle"]["cellBorders"].Bool())
		canvas->draw(cellBorders, owner->pos.topLeft());
}

void CBattleFieldController::showBackgroundImageWithHexes(std::shared_ptr<CCanvas> canvas)
{
	canvas->draw(backgroundWithHexes, owner->pos.topLeft());
}

void CBattleFieldController::redrawBackgroundWithHexes()
{
	const CStack *activeStack = owner->stacksController->getActiveStack();
	std::vector<BattleHex> attackableHexes;
	if (activeStack)
		occupyableHexes = owner->curInt->cb->battleGetAvailableHexes(activeStack, true, &attackableHexes);

	auto accessibility = owner->curInt->cb->getAccesibility();

	for(int i = 0; i < accessibility.size(); i++)
		stackCountOutsideHexes[i] = (accessibility[i] == EAccessibility::ACCESSIBLE);

	//prepare background graphic with hexes and shaded hexes
	backgroundWithHexes->draw(background, Point(0,0));
	owner->obstacleController->showAbsoluteObstacles(backgroundWithHexes, Point(0,0));
	if ( owner->siegeController )
		owner->siegeController->showAbsoluteObstacles(backgroundWithHexes, Point(0,0));

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
		backgroundWithHexes->draw(cellBorders, Point(0, 0));
}

void CBattleFieldController::showHighlightedHex(std::shared_ptr<CCanvas> to, BattleHex hex, bool darkBorder)
{
	Point hexPos = hexPosition(hex).topLeft();

	to->draw(cellShade, hexPos);
	if(!darkBorder && settings["battle"]["cellBorders"].Bool())
		to->draw(cellBorder, hexPos);
}

std::set<BattleHex> CBattleFieldController::getHighlightedHexesStackRange()
{
	std::set<BattleHex> result;

	if ( !owner->stacksController->getActiveStack())
		return result;

	if ( !settings["battle"]["stackRange"].Bool())
		return result;

	auto hoveredHex = getHoveredHex();

	std::set<BattleHex> set = owner->curInt->cb->battleGetAttackedHexes(owner->stacksController->getActiveStack(), hoveredHex, attackingHex);
	for(BattleHex hex : set)
		result.insert(hex);

	// display the movement shadow of the stack at b (i.e. stack under mouse)
	const CStack * const shere = owner->curInt->cb->battleGetStackByPos(hoveredHex, false);
	if(shere && shere != owner->stacksController->getActiveStack() && shere->alive())
	{
		std::vector<BattleHex> v = owner->curInt->cb->battleGetAvailableHexes(shere, true, nullptr);
		for(BattleHex hex : v)
			result.insert(hex);
	}
	return result;
}

std::set<BattleHex> CBattleFieldController::getHighlightedHexesSpellRange()
{
	std::set<BattleHex> result;
	auto hoveredHex = getHoveredHex();

	if(!settings["battle"]["mouseShadow"].Bool())
		return result;

	const spells::Caster *caster = nullptr;
	const CSpell *spell = nullptr;

	spells::Mode mode = spells::Mode::HERO;

	if(owner->actionsController->spellcastingModeActive())//hero casts spell
	{
		spell = owner->actionsController->selectedSpell().toSpell();
		caster = owner->getActiveHero();
	}
	else if(owner->stacksController->activeStackSpellToCast() != SpellID::NONE)//stack casts spell
	{
		spell = SpellID(owner->stacksController->activeStackSpellToCast()).toSpell();
		caster = owner->stacksController->getActiveStack();
		mode = spells::Mode::CREATURE_ACTIVE;
	}

	if(caster && spell) //when casting spell
	{
		// printing shaded hex(es)
		spells::BattleCast event(owner->curInt->cb.get(), caster, mode, spell);
		auto shaded = spell->battleMechanics(&event)->rangeInHexes(hoveredHex);

		for(BattleHex shadedHex : shaded)
		{
			if((shadedHex.getX() != 0) && (shadedHex.getX() != GameConstants::BFIELD_WIDTH - 1))
				result.insert(shadedHex);
		}
	}
	else if(owner->active) //always highlight pointed hex
	{
		if(hoveredHex.getX() != 0 && hoveredHex.getX() != GameConstants::BFIELD_WIDTH - 1)
			result.insert(hoveredHex);
	}

	return result;
}

void CBattleFieldController::showHighlightedHexes(std::shared_ptr<CCanvas> canvas)
{
	std::set<BattleHex> hoveredStack = getHighlightedHexesStackRange();
	std::set<BattleHex> hoveredMouse = getHighlightedHexesSpellRange();

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

Rect CBattleFieldController::hexPositionLocal(BattleHex hex) const
{
	int x = 14 + ((hex.getY())%2==0 ? 22 : 0) + 44*hex.getX();
	int y = 86 + 42 *hex.getY();
	int w = cellShade->width();
	int h = cellShade->height();
	return Rect(x, y, w, h);
}

Rect CBattleFieldController::hexPosition(BattleHex hex) const
{
	return hexPositionLocal(hex) + owner->pos.topLeft();
}

bool CBattleFieldController::isPixelInHex(Point const & position)
{
	return !cellShade->isTransparent(position);
}

BattleHex CBattleFieldController::getHoveredHex()
{
	for ( auto const & hex : bfield)
		if (hex->hovered && hex->strictHovered)
			return hex->myNumber;

	return BattleHex::INVALID;
}

void CBattleFieldController::setBattleCursor(BattleHex myNumber)
{
	Rect hoveredHexPos = hexPosition(myNumber);
	CCursorHandler *cursor = CCS->curh;

	const double subdividingAngle = 2.0*M_PI/6.0; // Divide a hex into six sectors.
	const double hexMidX = hoveredHexPos.x + hoveredHexPos.w/2.0;
	const double hexMidY = hoveredHexPos.y + hoveredHexPos.h/2.0;
	const double cursorHexAngle = M_PI - atan2(hexMidY - cursor->ypos, cursor->xpos - hexMidX) + subdividingAngle/2; //TODO: refactor this nightmare
	const double sector = fmod(cursorHexAngle/subdividingAngle, 6.0);
	const int zigzagCorrection = !((myNumber/GameConstants::BFIELD_WIDTH)%2); // Off-by-one correction needed to deal with the odd battlefield rows.

	std::vector<int> sectorCursor; // From left to bottom left.
	sectorCursor.push_back(8);
	sectorCursor.push_back(9);
	sectorCursor.push_back(10);
	sectorCursor.push_back(11);
	sectorCursor.push_back(12);
	sectorCursor.push_back(7);

	const bool doubleWide = owner->stacksController->getActiveStack()->doubleWide();
	bool aboveAttackable = true, belowAttackable = true;

	// Exclude directions which cannot be attacked from.
	// Check to the left.
	if (myNumber%GameConstants::BFIELD_WIDTH <= 1 || !vstd::contains(occupyableHexes, myNumber - 1))
	{
		sectorCursor[0] = -1;
	}
	// Check top left, top right as well as above for 2-hex creatures.
	if (myNumber/GameConstants::BFIELD_WIDTH == 0)
	{
			sectorCursor[1] = -1;
			sectorCursor[2] = -1;
			aboveAttackable = false;
	}
	else
	{
		if (doubleWide)
		{
			bool attackRow[4] = {true, true, true, true};

			if (myNumber%GameConstants::BFIELD_WIDTH <= 1 || !vstd::contains(occupyableHexes, myNumber - GameConstants::BFIELD_WIDTH - 2 + zigzagCorrection))
				attackRow[0] = false;
			if (!vstd::contains(occupyableHexes, myNumber - GameConstants::BFIELD_WIDTH - 1 + zigzagCorrection))
				attackRow[1] = false;
			if (!vstd::contains(occupyableHexes, myNumber - GameConstants::BFIELD_WIDTH + zigzagCorrection))
				attackRow[2] = false;
			if (myNumber%GameConstants::BFIELD_WIDTH >= GameConstants::BFIELD_WIDTH - 2 || !vstd::contains(occupyableHexes, myNumber - GameConstants::BFIELD_WIDTH + 1 + zigzagCorrection))
				attackRow[3] = false;

			if (!(attackRow[0] && attackRow[1]))
				sectorCursor[1] = -1;
			if (!(attackRow[1] && attackRow[2]))
				aboveAttackable = false;
			if (!(attackRow[2] && attackRow[3]))
				sectorCursor[2] = -1;
		}
		else
		{
			if (!vstd::contains(occupyableHexes, myNumber - GameConstants::BFIELD_WIDTH - 1 + zigzagCorrection))
				sectorCursor[1] = -1;
			if (!vstd::contains(occupyableHexes, myNumber - GameConstants::BFIELD_WIDTH + zigzagCorrection))
				sectorCursor[2] = -1;
		}
	}
	// Check to the right.
	if (myNumber%GameConstants::BFIELD_WIDTH >= GameConstants::BFIELD_WIDTH - 2 || !vstd::contains(occupyableHexes, myNumber + 1))
	{
		sectorCursor[3] = -1;
	}
	// Check bottom right, bottom left as well as below for 2-hex creatures.
	if (myNumber/GameConstants::BFIELD_WIDTH == GameConstants::BFIELD_HEIGHT - 1)
	{
		sectorCursor[4] = -1;
		sectorCursor[5] = -1;
		belowAttackable = false;
	}
	else
	{
		if (doubleWide)
		{
			bool attackRow[4] = {true, true, true, true};

			if (myNumber%GameConstants::BFIELD_WIDTH <= 1 || !vstd::contains(occupyableHexes, myNumber + GameConstants::BFIELD_WIDTH - 2 + zigzagCorrection))
				attackRow[0] = false;
			if (!vstd::contains(occupyableHexes, myNumber + GameConstants::BFIELD_WIDTH - 1 + zigzagCorrection))
				attackRow[1] = false;
			if (!vstd::contains(occupyableHexes, myNumber + GameConstants::BFIELD_WIDTH + zigzagCorrection))
				attackRow[2] = false;
			if (myNumber%GameConstants::BFIELD_WIDTH >= GameConstants::BFIELD_WIDTH - 2 || !vstd::contains(occupyableHexes, myNumber + GameConstants::BFIELD_WIDTH + 1 + zigzagCorrection))
				attackRow[3] = false;

			if (!(attackRow[0] && attackRow[1]))
				sectorCursor[5] = -1;
			if (!(attackRow[1] && attackRow[2]))
				belowAttackable = false;
			if (!(attackRow[2] && attackRow[3]))
				sectorCursor[4] = -1;
		}
		else
		{
			if (!vstd::contains(occupyableHexes, myNumber + GameConstants::BFIELD_WIDTH + zigzagCorrection))
				sectorCursor[4] = -1;
			if (!vstd::contains(occupyableHexes, myNumber + GameConstants::BFIELD_WIDTH - 1 + zigzagCorrection))
				sectorCursor[5] = -1;
		}
	}

	// Determine index from sector.
	int cursorIndex;
	if (doubleWide)
	{
		sectorCursor.insert(sectorCursor.begin() + 5, belowAttackable ? 13 : -1);
		sectorCursor.insert(sectorCursor.begin() + 2, aboveAttackable ? 14 : -1);

		if (sector < 1.5)
			cursorIndex = static_cast<int>(sector);
		else if (sector >= 1.5 && sector < 2.5)
			cursorIndex = 2;
		else if (sector >= 2.5 && sector < 4.5)
			cursorIndex = (int) sector + 1;
		else if (sector >= 4.5 && sector < 5.5)
			cursorIndex = 6;
		else
			cursorIndex = (int) sector + 2;
	}
	else
	{
		cursorIndex = static_cast<int>(sector);
	}

	// Generally should NEVER happen, but to avoid the possibility of having endless loop below... [#1016]
	if (!vstd::contains_if (sectorCursor, [](int sc) { return sc != -1; }))
	{
		logGlobal->error("Error: for hex %d cannot find a hex to attack from!", myNumber);
		attackingHex = -1;
		return;
	}

	// Find the closest direction attackable, starting with the right one.
	// FIXME: Is this really how the original H3 client does it?
	int i = 0;
	while (sectorCursor[(cursorIndex + i)%sectorCursor.size()] == -1) //Why hast thou forsaken me?
		i = i <= 0 ? 1 - i : -i; // 0, 1, -1, 2, -2, 3, -3 etc..
	int index = (cursorIndex + i)%sectorCursor.size(); //hopefully we get elements from sectorCursor
	cursor->changeGraphic(ECursor::COMBAT, sectorCursor[index]);
	switch (index)
	{
		case 0:
			attackingHex = myNumber - 1; //left
			break;
		case 1:
			attackingHex = myNumber - GameConstants::BFIELD_WIDTH - 1 + zigzagCorrection; //top left
			break;
		case 2:
			attackingHex = myNumber - GameConstants::BFIELD_WIDTH + zigzagCorrection; //top right
			break;
		case 3:
			attackingHex = myNumber + 1; //right
			break;
		case 4:
			attackingHex = myNumber + GameConstants::BFIELD_WIDTH + zigzagCorrection; //bottom right
			break;
		case 5:
			attackingHex = myNumber + GameConstants::BFIELD_WIDTH - 1 + zigzagCorrection; //bottom left
			break;
	}
	BattleHex hex(attackingHex);
	if (!hex.isValid())
		attackingHex = -1;
}

BattleHex CBattleFieldController::fromWhichHexAttack(BattleHex myNumber)
{
	//TODO far too much repeating code
	BattleHex destHex;
	switch(CCS->curh->frame)
	{
	case 12: //from bottom right
		{
			bool doubleWide = owner->stacksController->getActiveStack()->doubleWide();
			destHex = myNumber + ( (myNumber/GameConstants::BFIELD_WIDTH)%2 ? GameConstants::BFIELD_WIDTH : GameConstants::BFIELD_WIDTH+1 ) +
				(owner->stacksController->getActiveStack()->side == BattleSide::ATTACKER && doubleWide ? 1 : 0);
			if(vstd::contains(occupyableHexes, destHex))
				return destHex;
			else if(owner->stacksController->getActiveStack()->side == BattleSide::ATTACKER)
			{
				if (vstd::contains(occupyableHexes, destHex+1))
					return destHex+1;
			}
			else //if we are defender
			{
				if(vstd::contains(occupyableHexes, destHex-1))
					return destHex-1;
			}
			break;
		}
	case 7: //from bottom left
		{
			destHex = myNumber + ( (myNumber/GameConstants::BFIELD_WIDTH)%2 ? GameConstants::BFIELD_WIDTH-1 : GameConstants::BFIELD_WIDTH );
			if (vstd::contains(occupyableHexes, destHex))
				return destHex;
			else if(owner->stacksController->getActiveStack()->side == BattleSide::ATTACKER)
			{
				if(vstd::contains(occupyableHexes, destHex+1))
					return destHex+1;
			}
			else //we are defender
			{
				if(vstd::contains(occupyableHexes, destHex-1))
					return destHex-1;
			}
			break;
		}
	case 8: //from left
		{
			if(owner->stacksController->getActiveStack()->doubleWide() && owner->stacksController->getActiveStack()->side == BattleSide::DEFENDER)
			{
				std::vector<BattleHex> acc = owner->curInt->cb->battleGetAvailableHexes(owner->stacksController->getActiveStack());
				if (vstd::contains(acc, myNumber))
					return myNumber - 1;
				else
					return myNumber - 2;
			}
			else
			{
				return myNumber - 1;
			}
			break;
		}
	case 9: //from top left
		{
			destHex = myNumber - ((myNumber/GameConstants::BFIELD_WIDTH) % 2 ? GameConstants::BFIELD_WIDTH + 1 : GameConstants::BFIELD_WIDTH);
			if(vstd::contains(occupyableHexes, destHex))
				return destHex;
			else if(owner->stacksController->getActiveStack()->side == BattleSide::ATTACKER)
			{
				if(vstd::contains(occupyableHexes, destHex+1))
					return destHex+1;
			}
			else //if we are defender
			{
				if(vstd::contains(occupyableHexes, destHex-1))
					return destHex-1;
			}
			break;
		}
	case 10: //from top right
		{
			bool doubleWide = owner->stacksController->getActiveStack()->doubleWide();
			destHex = myNumber - ( (myNumber/GameConstants::BFIELD_WIDTH)%2 ? GameConstants::BFIELD_WIDTH : GameConstants::BFIELD_WIDTH-1 ) +
				(owner->stacksController->getActiveStack()->side == BattleSide::ATTACKER && doubleWide ? 1 : 0);
			if(vstd::contains(occupyableHexes, destHex))
				return destHex;
			else if(owner->stacksController->getActiveStack()->side == BattleSide::ATTACKER)
			{
				if(vstd::contains(occupyableHexes, destHex+1))
					return destHex+1;
			}
			else //if we are defender
			{
				if(vstd::contains(occupyableHexes, destHex-1))
					return destHex-1;
			}
			break;
		}
	case 11: //from right
		{
			if(owner->stacksController->getActiveStack()->doubleWide() && owner->stacksController->getActiveStack()->side == BattleSide::ATTACKER)
			{
				std::vector<BattleHex> acc = owner->curInt->cb->battleGetAvailableHexes(owner->stacksController->getActiveStack());
				if(vstd::contains(acc, myNumber))
					return myNumber + 1;
				else
					return myNumber + 2;
			}
			else
			{
				return myNumber + 1;
			}
			break;
		}
	case 13: //from bottom
		{
			destHex = myNumber + ( (myNumber/GameConstants::BFIELD_WIDTH)%2 ? GameConstants::BFIELD_WIDTH : GameConstants::BFIELD_WIDTH+1 );
			if(vstd::contains(occupyableHexes, destHex))
				return destHex;
			else if(owner->stacksController->getActiveStack()->side == BattleSide::ATTACKER)
			{
				if(vstd::contains(occupyableHexes, destHex+1))
					return destHex+1;
			}
			else //if we are defender
			{
				if(vstd::contains(occupyableHexes, destHex-1))
					return destHex-1;
			}
			break;
		}
	case 14: //from top
		{
			destHex = myNumber - ( (myNumber/GameConstants::BFIELD_WIDTH)%2 ? GameConstants::BFIELD_WIDTH : GameConstants::BFIELD_WIDTH-1 );
			if (vstd::contains(occupyableHexes, destHex))
				return destHex;
			else if(owner->stacksController->getActiveStack()->side == BattleSide::ATTACKER)
			{
				if(vstd::contains(occupyableHexes, destHex+1))
					return destHex+1;
			}
			else //if we are defender
			{
				if(vstd::contains(occupyableHexes, destHex-1))
					return destHex-1;
			}
			break;
		}
	}
	return -1;
}

bool CBattleFieldController::isTileAttackable(const BattleHex & number) const
{
	for (auto & elem : occupyableHexes)
	{
		if (BattleHex::mutualPosition(elem, number) != -1 || elem == number)
			return true;
	}
	return false;
}

bool CBattleFieldController::stackCountOutsideHex(const BattleHex & number) const
{
	return stackCountOutsideHexes[number];
}
