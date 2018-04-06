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
#include "CAdvmapInterface.h"
#include "CHeroWindow.h"
#include "../widgets/Buttons.h"
#include "../../CCallback.h"
#include "../CreatureCostBox.h"
#include "QuickRecruitmentWindow.h"
#include "../gui/CGuiHandler.h"
#include "../../lib/CCreatureHandler.h"

void CreaturePurchaseCard::initButtons()
{
	initMaxButton();
	initMinButton();
	initCreatureSwitcherButton();
}

void CreaturePurchaseCard::initMaxButton()
{
	maxButton = std::make_shared<CButton>(Point(pos.x + 52, pos.y + 180), "QuickRecruitmentWindow/QuickRecruitmentAllButton.def", CButton::tooltip(), std::bind(&CSlider::moveToMax,slider), SDLK_LSHIFT);
}

void CreaturePurchaseCard::initMinButton()
{
	minButton = std::make_shared<CButton>(Point(pos.x, pos.y + 180), "QuickRecruitmentWindow/QuickRecruitmentNoneButton.def", CButton::tooltip(), std::bind(&CSlider::moveToMin,slider), SDLK_LCTRL);
}

void CreaturePurchaseCard::initCreatureSwitcherButton()
{
	creatureSwitcher = std::make_shared<CButton>(Point(pos.x + 18, pos.y-37), "iDv6432.def", CButton::tooltip(), [&](){ switchCreatureLevel(); });
}

void CreaturePurchaseCard::switchCreatureLevel()
{
	OBJECT_CONSTRUCTION_CAPTURING(ACTIVATE + DEACTIVATE + UPDATE + SHOWALL + SHARE_POS);
	auto index = vstd::find_pos(upgradesID, creatureOnTheCard->idNumber);
	auto nextCreatureId = vstd::circularAt(upgradesID, ++index);
	creatureOnTheCard = nextCreatureId.toCreature();
	picture = std::make_shared<CCreaturePic>(parent->pos.x, parent->pos.y, creatureOnTheCard);
	parent->updateAllSliders();
	cost->set(creatureOnTheCard->cost * slider->getValue());
}

void CreaturePurchaseCard::initAmountInfo()
{
	availableAmount = std::make_shared<CLabel>(pos.x + 25, pos.y + 146, FONT_SMALL, CENTER, Colors::YELLOW);
	purhaseAmount = std::make_shared<CLabel>(pos.x + 76, pos.y + 146, FONT_SMALL, CENTER, Colors::WHITE);
	updateAmountInfo(0);
}

void CreaturePurchaseCard::updateAmountInfo(int value)
{
	availableAmount->setText(boost::lexical_cast<std::string>(maxAmount-value));
	purhaseAmount->setText(boost::lexical_cast<std::string>(value));
}

void CreaturePurchaseCard::initSlider()
{
	slider = std::make_shared<CSlider>(Point(pos.x, pos.y + 158), 102, std::bind(&CreaturePurchaseCard::sliderMoved, this , _1), 0, maxAmount, 0);
}

void CreaturePurchaseCard::initCostBox()
{
	cost = std::make_shared<CreatureCostBox>(Rect(pos.x+2, pos.y + 194, 97, 74), "");
	cost->createItems(creatureOnTheCard->cost);
}

void CreaturePurchaseCard::sliderMoved(int to)
{
	updateAmountInfo(to);
	cost->set(creatureOnTheCard->cost * to);
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
	initAmountInfo();
	initSlider();
	initButtons();
	initCostBox();
}
