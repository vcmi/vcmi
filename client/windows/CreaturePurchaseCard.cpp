/*
 * CreaturePurchaseCard.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "CreaturePurchaseCard.h"

#include "CHeroWindow.h"
#include "QuickRecruitmentWindow.h"
#include "CCreatureWindow.h"

#include "../gui/CGuiHandler.h"
#include "../gui/Shortcut.h"
#include "../gui/TextAlignment.h"
#include "../gui/WindowHandler.h"
#include "../widgets/Buttons.h"
#include "../widgets/Slider.h"
#include "../widgets/TextControls.h"
#include "../widgets/CreatureCostBox.h"

#include "../../CCallback.h"
#include "../../lib/CCreatureHandler.h"

void CreaturePurchaseCard::initButtons()
{
	initMaxButton();
	initMinButton();
	initCreatureSwitcherButton();
}

void CreaturePurchaseCard::initMaxButton()
{
	maxButton = std::make_shared<CButton>(Point(pos.x + 52, pos.y + 180), "QuickRecruitmentWindow/QuickRecruitmentAllButton.def", CButton::tooltip(), std::bind(&CSlider::moveToMax,slider), EShortcut::RECRUITMENT_MAX);
}

void CreaturePurchaseCard::initMinButton()
{
	minButton = std::make_shared<CButton>(Point(pos.x, pos.y + 180), "QuickRecruitmentWindow/QuickRecruitmentNoneButton.def", CButton::tooltip(), std::bind(&CSlider::moveToMin,slider), EShortcut::RECRUITMENT_MIN);
}

void CreaturePurchaseCard::initCreatureSwitcherButton()
{
	creatureSwitcher = std::make_shared<CButton>(Point(pos.x + 18, pos.y-37), "iDv6432.def", CButton::tooltip(), [&](){ switchCreatureLevel(); });
}

void CreaturePurchaseCard::switchCreatureLevel()
{
	OBJECT_CONSTRUCTION_CAPTURING(ACTIVATE + DEACTIVATE + UPDATE + SHOWALL + SHARE_POS);
	auto index = vstd::find_pos(upgradesID, creatureOnTheCard->getId());
	auto nextCreatureId = vstd::circularAt(upgradesID, ++index);
	creatureOnTheCard = nextCreatureId.toCreature();
	picture = std::make_shared<CCreaturePic>(parent->pos.x, parent->pos.y, creatureOnTheCard);
	creatureClickArea = std::make_shared<CCreatureClickArea>(Point(parent->pos.x, parent->pos.y), picture, creatureOnTheCard);
	parent->updateAllSliders();
	cost->set(creatureOnTheCard->getFullRecruitCost() * slider->getValue());
}

void CreaturePurchaseCard::initAmountInfo()
{
	availableAmount = std::make_shared<CLabel>(pos.x + 25, pos.y + 146, FONT_SMALL, ETextAlignment::CENTER, Colors::YELLOW);
	purchaseAmount = std::make_shared<CLabel>(pos.x + 76, pos.y + 146, FONT_SMALL, ETextAlignment::CENTER, Colors::WHITE);
	updateAmountInfo(0);
}

void CreaturePurchaseCard::updateAmountInfo(int value)
{
	availableAmount->setText(std::to_string(maxAmount-value));
	purchaseAmount->setText(std::to_string(value));
}

void CreaturePurchaseCard::initSlider()
{
	slider = std::make_shared<CSlider>(Point(pos.x, pos.y + 158), 102, std::bind(&CreaturePurchaseCard::sliderMoved, this , _1), 0, maxAmount, 0);
}

void CreaturePurchaseCard::initCostBox()
{
	cost = std::make_shared<CreatureCostBox>(Rect(pos.x+2, pos.y + 194, 97, 74), "");
	cost->createItems(creatureOnTheCard->getFullRecruitCost());
}

void CreaturePurchaseCard::sliderMoved(int to)
{
	updateAmountInfo(to);
	cost->set(creatureOnTheCard->getFullRecruitCost() * to);
	parent->updateAllSliders();
}

CreaturePurchaseCard::CreaturePurchaseCard(const std::vector<CreatureID> & creaturesID, Point position, int creaturesMaxAmount, QuickRecruitmentWindow * parents)
	: upgradesID(creaturesID),
	parent(parents),
	maxAmount(creaturesMaxAmount)
{
	creatureOnTheCard = upgradesID.back().toCreature();
	moveTo(Point(position.x, position.y));
	initView();
}

void CreaturePurchaseCard::initView()
{
	picture = std::make_shared<CCreaturePic>(pos.x, pos.y, creatureOnTheCard);
	background = std::make_shared<CPicture>("QuickRecruitmentWindow/CreaturePurchaseCard.png", pos.x-4, pos.y-50);
	creatureClickArea = std::make_shared<CCreatureClickArea>(Point(pos.x, pos.y), picture, creatureOnTheCard);

	initAmountInfo();
	initSlider();
	initButtons(); // order important! buttons need slider!
	initCostBox();
}

CreaturePurchaseCard::CCreatureClickArea::CCreatureClickArea(const Point & position, const std::shared_ptr<CCreaturePic> creaturePic, const CCreature * creatureOnTheCard)
	: CIntObject(RCLICK),
	creatureOnTheCard(creatureOnTheCard)
{
	pos.x += position.x;
	pos.y += position.y;
	pos.w = CREATURE_WIDTH;
	pos.h = CREATURE_HEIGHT;
}

void CreaturePurchaseCard::CCreatureClickArea::clickRight(tribool down, bool previousState)
{
	if (down)
		GH.windows().createAndPushWindow<CStackWindow>(creatureOnTheCard, true);
}
