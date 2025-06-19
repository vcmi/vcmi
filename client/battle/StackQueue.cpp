/*
 * StackQueue.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "StackQueue.h"

#include "BattleInterface.h"
#include "BattleSiegeController.h"
#include "BattleStacksController.h"

#include "../GameEngine.h"
#include "../gui/WindowHandler.h"
#include "../render/Canvas.h"
#include "../render/IFont.h"
#include "../render/IRenderHandler.h"
#include "../widgets/GraphicalPrimitiveCanvas.h"
#include "../widgets/Images.h"
#include "../widgets/TextControls.h"
#include "../windows/CCreatureWindow.h"

#include "../../lib/CConfigHandler.h"
#include "../../lib/CStack.h"
#include "../../lib/battle/CPlayerBattleCallback.h"
#include "../../lib/mapObjects/CGTownInstance.h"
#include "../../lib/texts/TextOperations.h"

StackQueue::StackQueue(bool Embedded, BattleInterface & owner)
	: owner(owner)
	, embedded(Embedded)
{
	OBJECT_CONSTRUCTION;

	uint32_t queueSize = QUEUE_SIZE_BIG;

	if(embedded)
	{
		int32_t queueSmallOutsideYOffset = 65;
		bool queueSmallOutside = settings["battle"]["queueSmallOutside"].Bool() && (pos.y - queueSmallOutsideYOffset) >= 0;
		queueSize = std::clamp(static_cast<int>(settings["battle"]["queueSmallSlots"].Float()), 1, queueSmallOutside ? ENGINE->screenDimensions().x / 41 : 19);

		pos.w = queueSize * 41;
		pos.h = 49;
		pos.x += parent->pos.w/2 - pos.w/2;
		pos.y += queueSmallOutside ? -queueSmallOutsideYOffset : 10;
	}
	else
	{
		pos.w = 800;
		pos.h = 85;
		pos.x += 0;
		pos.y -= pos.h;

		background = std::make_shared<CFilledTexture>(ImagePath::builtin("DIBOXBCK"), Rect(0, 0, pos.w, pos.h));
	}

	stackBoxes.resize(queueSize);
	for (int i = 0; i < stackBoxes.size(); i++)
	{
		stackBoxes[i] = std::make_shared<StackBox>(this);
		stackBoxes[i]->moveBy(Point(1 + (embedded ? 41 : 80) * i, 0));
	}
}

void StackQueue::show(Canvas & to)
{
	if(embedded)
		showAll(to);
	CIntObject::show(to);
}

void StackQueue::update()
{
	std::vector<battle::Units> queueData;

	owner.getBattle()->battleGetTurnOrder(queueData, stackBoxes.size(), 0);

	size_t boxIndex = 0;
	ui32 tmpTurn = -1;

	for(size_t turn = 0; turn < queueData.size() && boxIndex < stackBoxes.size(); turn++)
	{
		for(size_t unitIndex = 0; unitIndex < queueData[turn].size() && boxIndex < stackBoxes.size(); boxIndex++, unitIndex++)
		{
			ui32 currentTurn = owner.round + turn;
			stackBoxes[boxIndex]->setUnit(queueData[turn][unitIndex], turn, tmpTurn != currentTurn && owner.round != 0 && (!embedded || tmpTurn != -1) ? (std::optional<ui32>)currentTurn : std::nullopt);
			tmpTurn = currentTurn;
		}
	}

	for(; boxIndex < stackBoxes.size(); boxIndex++)
		stackBoxes[boxIndex]->setUnit(nullptr);
}

int32_t StackQueue::getSiegeShooterIconID() const
{
	return owner.siegeController->getSiegedTown()->getFactionID().getNum();
}

std::optional<uint32_t> StackQueue::getHoveredUnitIdIfAny() const
{
	for(const auto & stackBox : stackBoxes)
	{
		if(stackBox->isHovered())
		{
			return stackBox->getBoundUnitID();
		}
	}

	return std::nullopt;
}

StackQueue::StackBox::StackBox(StackQueue * owner)
	: CIntObject(SHOW_POPUP | HOVER)
	, owner(owner)
{
	OBJECT_CONSTRUCTION;
	background = std::make_shared<CPicture>(ImagePath::builtin(owner->embedded ? "StackQueueSmall" : "StackQueueLarge"));

	pos.w = background->pos.w;
	pos.h = background->pos.h;

	if(owner->embedded)
	{
		icon = std::make_shared<CAnimImage>(AnimationPath::builtin("CPRSMALL"), 0, 0, 5, 2);
		amount = std::make_shared<CLabel>(pos.w/2, pos.h - 7, FONT_SMALL, ETextAlignment::CENTER, Colors::WHITE);
		roundRect = std::make_shared<TransparentFilledRectangle>(Rect(0, 0, 2, 48), ColorRGBA(0, 0, 0, 255), ColorRGBA(0, 255, 0, 255));
	}
	else
	{
		icon = std::make_shared<CAnimImage>(AnimationPath::builtin("TWCRPORT"), 0, 0, 9, 1);
		amount = std::make_shared<CLabel>(pos.w/2, pos.h - 8, FONT_MEDIUM, ETextAlignment::CENTER, Colors::WHITE);
		roundRect = std::make_shared<TransparentFilledRectangle>(Rect(0, 0, 15, 18), ColorRGBA(0, 0, 0, 255), ColorRGBA(241, 216, 120, 255));
		round = std::make_shared<CLabel>(6, 9, FONT_SMALL, ETextAlignment::CENTER, Colors::WHITE);

		Point iconPos(pos.w - 16, pos.h - 16);

		defendIcon = std::make_shared<CPicture>(ImagePath::builtin("battle/QueueDefend"), iconPos);
		waitIcon = std::make_shared<CPicture>(ImagePath::builtin("battle/QueueWait"), iconPos);

		defendIcon->setEnabled(false);
		waitIcon->setEnabled(false);
	}
	roundRect->disable();
}

void StackQueue::StackBox::setUnit(const battle::Unit * unit, size_t turn, std::optional<ui32> currentTurn)
{
	if(unit)
	{
		boundUnitID = unit->unitId();
		background->setPlayerColor(unit->unitOwner());
		icon->visible = true;

		// temporary code for mod compatibility:
		// first, set icon that should definitely exist (arrow tower icon in base vcmi mod)
		// second, try to switch to icon that should be provided by mod
		// if mod is not up to date and does have arrow tower icon yet - second setFrame call will fail and retain previously set image
		// for 1.2 release & later next line should be moved into 'else' block
		icon->setFrame(unit->creatureIconIndex(), 0);
		if(unit->unitType()->getId() == CreatureID::ARROW_TOWERS)
			icon->setFrame(owner->getSiegeShooterIconID(), 1);

		roundRect->setEnabled(currentTurn.has_value());
		if(!owner->embedded)
			round->setEnabled(currentTurn.has_value());

		amount->setText(TextOperations::formatMetric(unit->getCount(), 4));
		if(currentTurn && !owner->embedded)
		{
			std::string tmp = std::to_string(*currentTurn);
			const auto & font = ENGINE->renderHandler().loadFont(FONT_SMALL);
			int len = font->getStringWidth(tmp);
			roundRect->pos.w = len + 6;
			round->pos = Rect(roundRect->pos.center().x, roundRect->pos.center().y, 0, 0);
			round->setText(tmp);
		}

		if(!owner->embedded)
		{
			bool defended = unit->defended(turn) || (turn > 0 && unit->defended(turn - 1));
			bool waited = unit->waited(turn) && !defended;

			defendIcon->setEnabled(defended);
			waitIcon->setEnabled(waited);
		}
	}
	else
	{
		boundUnitID = std::nullopt;
		background->setPlayerColor(PlayerColor::NEUTRAL);
		icon->visible = false;
		icon->setFrame(0);
		amount->setText("");
		if(!owner->embedded)
		{
			defendIcon->setEnabled(false);
			waitIcon->setEnabled(false);
		}
	}
}

std::optional<uint32_t> StackQueue::StackBox::getBoundUnitID() const
{
	return boundUnitID;
}

bool StackQueue::StackBox::isBoundUnitHighlighted() const
{
	auto unitIdsToHighlight = owner->owner.stacksController->getHoveredStacksUnitIds();
	return vstd::contains(unitIdsToHighlight, getBoundUnitID());
}

void StackQueue::StackBox::showAll(Canvas & to)
{
	CIntObject::showAll(to);

	if(isBoundUnitHighlighted())
		to.drawBorder(background->pos, Colors::CYAN, 2);
}

void StackQueue::StackBox::show(Canvas & to)
{
	CIntObject::show(to);

	if(isBoundUnitHighlighted())
		to.drawBorder(background->pos, Colors::CYAN, 2);
}

void StackQueue::StackBox::showPopupWindow(const Point & cursorPosition)
{
	auto stacks = owner->owner.getBattle()->battleGetAllStacks();
	for(const CStack * stack : stacks)
		if(boundUnitID.has_value() && stack->unitId() == *boundUnitID)
			ENGINE->windows().createAndPushWindow<CStackWindow>(stack, true);
}
