/*
 * StackQueue.h, part of VCMI engine
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

namespace battle
{
class Unit;
}

class CLabel;
class TransparentFilledRectangle;
class CAnimImage;
class CFilledTexture;
class BattleInterface;

/// Shows the stack queue
class StackQueue : public CIntObject
{
	class StackBox : public CIntObject
	{
		StackQueue * owner;
		std::optional<uint32_t> boundUnitID;

		std::shared_ptr<CPicture> background;
		std::shared_ptr<CAnimImage> icon;
		std::shared_ptr<CLabel> amount;
		std::shared_ptr<CPicture> waitIcon;
		std::shared_ptr<CPicture> defendIcon;
		std::shared_ptr<CLabel> round;
		std::shared_ptr<TransparentFilledRectangle> roundRect;

		void show(Canvas & to) override;
		void showAll(Canvas & to) override;
		void showPopupWindow(const Point & cursorPosition) override;
		void clickPressed(const Point & cursorPosition) override;
		bool isBoundUnitHighlighted() const;

		/// battlefield hex occupied by the bound stack, or BattleHex::INVALID if none
		BattleHex getBoundUnitHex() const;

	public:
		StackBox(StackQueue * owner);
		void setUnit(const battle::Unit * unit, size_t turn = 0, std::optional<ui32> currentTurn = std::nullopt);
		std::optional<uint32_t> getBoundUnitID() const;
	};

	static const int QUEUE_SIZE_BIG = 10;
	std::shared_ptr<CFilledTexture> background;
	std::vector<std::shared_ptr<StackBox>> stackBoxes;
	BattleInterface & owner;

	int32_t getSiegeShooterIconID() const;

public:
	const bool embedded;

	StackQueue(bool Embedded, BattleInterface & owner);
	void update();
	std::optional<uint32_t> getHoveredUnitIdIfAny() const;

	void show(Canvas & to) override;
};
