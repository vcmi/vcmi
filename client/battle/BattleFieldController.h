/*
 * BattleFieldController.h, part of VCMI engine
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

VCMI_LIB_NAMESPACE_BEGIN

class CStack;
class Rect;
class Point;

VCMI_LIB_NAMESPACE_END

class BattleHero;
class Canvas;
class IImage;
class BattleInterface;

/// Handles battlefield grid as well as rendering of background layer of battle interface
class BattleFieldController : public CIntObject
{
	BattleInterface & owner;

	std::shared_ptr<IImage> background;
	std::shared_ptr<IImage> cellBorder;
	std::shared_ptr<IImage> cellUnitMovementHighlight;
	std::shared_ptr<IImage> cellUnitMaxMovementHighlight;

	std::shared_ptr<IImage> cellShade;

	// key in image map is a mask that has set to 1 the edges present in the image 0..5
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
	std::map<uint8_t, std::shared_ptr<IImage>> fullDamageRangeLimitImages; 

	/// Canvas that contains background, hex grid (if enabled), absolute obstacles and movement range of active stack
	std::unique_ptr<Canvas> backgroundWithHexes;

	/// hex from which the stack would perform attack with current cursor
	BattleHex attackingHex;

	/// hexes to which currently active stack can move
	std::vector<BattleHex> occupiableHexes;

	/// hexes that when in front of a unit cause it's amount box to move back
	std::array<bool, GameConstants::BFIELD_SIZE> stackCountOutsideHexes;

	void showHighlightedHex(Canvas & to, std::shared_ptr<IImage> highlight, BattleHex hex, bool darkBorder);

	std::set<BattleHex> getHighlightedHexesForActiveStack();
	std::set<BattleHex> getMovementRangeForHoveredStack();
	std::set<BattleHex> getHighlightedHexesForSpellRange();
	std::set<BattleHex> getHighlightedHexesForMovementTarget();

	/// get all hexes where a ranged unit can do full damage
	std::vector<BattleHex> getRangedFullDamageHexes();

	/// get only hexes at the limit of a ranged unit's full damage range
	std::vector<BattleHex> getRangedFullDamageLimitHexes(std::vector<BattleHex> rangedFullDamageHexes);

	/// get an array that has for each hex in range, an aray with all directions where an ouside neighbour hex exists
	std::vector<std::vector<BattleHex::EDir>> getOutsideNeighbourDirectionsForLimitHexes(std::vector<BattleHex> rangedFullDamageHexes, std::vector<BattleHex> rangedFullDamageLimitHexes);

	/// calculates what image to use as range limit, depending on the direction of neighbors
	/// a mask is used internally to mark the directions of all neighbours
	/// based on this mask the corresponding image is selected
	std::vector<std::shared_ptr<IImage>> calculateFullRangedDamageHighlightImages(std::vector<std::vector<BattleHex::EDir>> fullRangeLimitHexesNeighbourDirections);

	void showBackground(Canvas & canvas);
	void showBackgroundImage(Canvas & canvas);
	void showBackgroundImageWithHexes(Canvas & canvas);
	void showHighlightedHexes(Canvas & canvas);
	void updateAccessibleHexes();

	BattleHex::EDir selectAttackDirection(BattleHex myNumber, const Point & point);

	void mouseMoved(const Point & cursorPosition) override;
	void clickLeft(tribool down, bool previousState) override;
	void clickRight(tribool down, bool previousState) override;
	void activate() override;

	void showAll(SDL_Surface * to) override;
	void show(SDL_Surface * to) override;
public:
	BattleFieldController(BattleInterface & owner);

	void createHeroes();

	void redrawBackgroundWithHexes();
	void renderBattlefield(Canvas & canvas);

	/// Returns position of hex relative to owner (BattleInterface)
	Rect hexPositionLocal(BattleHex hex) const;

	/// Returns position of hex relative to game window
	Rect hexPositionAbsolute(BattleHex hex) const;

	/// Checks whether selected pixel is transparent, uses local coordinates of a hex
	bool isPixelInHex(Point const & position);

	/// Returns ID of currently hovered hex or BattleHex::INVALID if none
	BattleHex getHoveredHex();

	/// returns true if selected tile can be attacked in melee by current stack
	bool isTileAttackable(const BattleHex & number) const;

	/// returns true if stack should render its stack count image in default position - outside own hex
	bool stackCountOutsideHex(const BattleHex & number) const;

	void setBattleCursor(BattleHex myNumber);
	BattleHex fromWhichHexAttack(BattleHex myNumber);
};
