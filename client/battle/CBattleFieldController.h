/*
 * CBattleFieldController.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../../lib/battle/BattleHex.h"
#include "../gui/CIntObject.h"

struct SDL_Surface;
struct Rect;
struct Point;

class CClickableHex;
class CStack;
class CBattleInterface;

class CBattleFieldController : public CIntObject
{
	CBattleInterface * owner;

	SDL_Surface *background;
	SDL_Surface *backgroundWithHexes;
	SDL_Surface *cellBorders;
	SDL_Surface *cellBorder;
	SDL_Surface *cellShade;

	BattleHex previouslyHoveredHex; //number of hex that was hovered by the cursor a while ago
	BattleHex currentlyHoveredHex; //number of hex that is supposed to be hovered (for a while it may be inappropriately set, but will be renewed soon)
	BattleHex attackingHex; //hex from which the stack would perform attack with current cursor

	std::vector<BattleHex> occupyableHexes; //hexes available for active stack
	std::vector<BattleHex> attackableHexes; //hexes attackable by active stack
	std::array<bool, GameConstants::BFIELD_SIZE> stackCountOutsideHexes; // hexes that when in front of a unit cause it's amount box to move back

	std::vector<std::shared_ptr<CClickableHex>> bfield; //11 lines, 17 hexes on each

public:
	CBattleFieldController(CBattleInterface * owner);
	~CBattleFieldController();

	void showBackgroundImage(SDL_Surface *to);
	void showBackgroundImageWithHexes(SDL_Surface *to);

	void redrawBackgroundWithHexes(const CStack *activeStack);

	void showHighlightedHexes(SDL_Surface *to);
	void showHighlightedHex(SDL_Surface *to, BattleHex hex, bool darkBorder);

	Rect hexPosition(BattleHex hex) const;
	bool isPixelInHex(Point const & position);

	BattleHex getHoveredHex();

	void setBattleCursor(BattleHex myNumber);
	BattleHex fromWhichHexAttack(BattleHex myNumber);
	bool isTileAttackable(const BattleHex & number) const;
	bool stackCountOutsideHex(const BattleHex & number) const;
};
