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

#include "../../lib/battle/BattleHexArray.h"
#include "../../lib/Point.h"
#include "../gui/CIntObject.h"

VCMI_LIB_NAMESPACE_BEGIN
class CStack;
class Rect;
VCMI_LIB_NAMESPACE_END

class BattleHero;
class CAnimation;
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
	std::shared_ptr<CAnimation> rangedFullDamageLimitImages;
	std::shared_ptr<CAnimation> shootingRangeLimitImages;

	/// Canvas that contains background, hex grid (if enabled), absolute obstacles and movement range of active stack
	std::unique_ptr<Canvas> backgroundWithHexes;

	/// direction which will be used to perform attack with current cursor position
	Point currentAttackOriginPoint;

	/// hex currently under mouse hover
	BattleHex hoveredHex;

	/// hexes to which the currently active stack can move (for double-wide units only the head is considered)
	BattleHexArray availableHexes;

	/// hexes that when in front of a unit cause it's amount box to move back
	std::array<bool, GameConstants::BFIELD_SIZE> stackCountOutsideHexes;

	void showHighlightedHex(Canvas & to, std::shared_ptr<IImage> highlight, const BattleHex & hex, bool darkBorder);

	BattleHexArray getHighlightedHexesForActiveStack();
	BattleHexArray getMovementRangeForHoveredStack();
	BattleHexArray getHighlightedHexesForSpellRange();
	BattleHexArray getHighlightedHexesForMovementTarget();

	// Range limit highlight helpers

	/// get all hexes within a certain distance of given hex
	BattleHexArray getRangeHexes(const BattleHex & sourceHex, uint8_t distance) const;

	/// get only hexes at the limit of a range
	BattleHexArray getRangeLimitHexes(const BattleHex & hoveredHex, const BattleHexArray & hexRange, uint8_t distanceToLimit) const;

	/// calculate if a hex is in range limit and return its index in range
	bool isHexInRangeLimit(const BattleHex & hex, const BattleHexArray & rangeLimitHexes, int * hexIndexInRangeLimit) const;

	/// get an array that has for each hex in range, an array with all directions where an outside neighbour hex exists
	std::vector<std::vector<BattleHex::EDir>> getOutsideNeighbourDirectionsForLimitHexes(const BattleHexArray & rangeHexes, const BattleHexArray & rangeLimitHexes) const;

	/// calculates what image to use as range limit, depending on the direction of neighbours
	/// a mask is used internally to mark the directions of all neighbours
	/// based on this mask the corresponding image is selected
	std::vector<std::shared_ptr<IImage>> calculateRangeLimitHighlightImages(std::vector<std::vector<BattleHex::EDir>> hexesNeighbourDirections, std::shared_ptr<CAnimation> limitImages);

	/// calculates all hexes for a range limit and what images to be shown as highlight for each of the hexes
	void calculateRangeLimitAndHighlightImages(uint8_t distance, std::shared_ptr<CAnimation> rangeLimitImages, BattleHexArray & rangeLimitHexes, std::vector<std::shared_ptr<IImage>> & rangeLimitHexesHighlights);

	void showBackground(Canvas & canvas);
	void showBackgroundImage(Canvas & canvas);
	void showBackgroundImageWithHexes(Canvas & canvas);
	void showHighlightedHexes(Canvas & canvas);
	void updateAccessibleHexes();

	BattleHex getHexAtPosition(Point hoverPosition);

	/// Checks whether selected pixel is transparent, uses local coordinates of a hex
	bool isPixelInHex(Point const & position);
	size_t selectBattleCursor(const BattleHex & myNumber);

	void gesture(bool on, const Point & initialPosition, const Point & finalPosition) override;
	void gesturePanning(const Point & initialPosition, const Point & currentPosition, const Point & lastUpdateDistance) override;
	void mouseMoved(const Point & cursorPosition, const Point & lastUpdateDistance) override;
	void clickPressed(const Point & cursorPosition) override;
	void showPopupWindow(const Point & cursorPosition) override;
	void activate() override;

	void showAll(Canvas & to) override;
	void show(Canvas & to) override;
	void tick(uint32_t msPassed) override;

	bool receiveEvent(const Point & position, int eventType) const override;

public:
	BattleFieldController(BattleInterface & owner);

	void createHeroes();

	void redrawBackgroundWithHexes();
	void renderBattlefield(Canvas & canvas);

	/// Returns position of hex relative to owner (BattleInterface)
	Rect hexPositionLocal(const BattleHex & hex) const;

	/// Returns position of hex relative to game window
	Rect hexPositionAbsolute(const BattleHex & hex) const;

	/// Returns ID of currently hovered hex or BattleHex::INVALID if none
	BattleHex getHoveredHex();

	/// Returns the currently hovered stack
	const CStack* getHoveredStack();

	/// returns true if stack should render its stack count image in default position - outside own hex
	bool stackCountOutsideHex(const BattleHex & number) const;

	BattleHex::EDir selectAttackDirection(const BattleHex & myNumber) const;
};
