/*
 * CreaturePurhaseCard.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "CreaturePurhaseCard.h"
#include "CAdvmapInterface.h"
#include "CHeroWindow.h"
#include "../widgets/Buttons.h"
#include "../../CCallback.h"
#include "../CreatureCostBox.h"
#include "QuickRecruitmentWindow.h"

void CreaturePurhaseCard::initButtons()
{
	initMaxButton();
	initMinButton();
}

void CreaturePurhaseCard::initMaxButton()
{
	maxButton = std::make_shared<CButton>(Point(pos.x + 52, pos.y + 178), "iam014.def", CButton::tooltip(), std::bind(&CSlider::moveToMax,slider), SDLK_m);
}

void CreaturePurhaseCard::initMinButton()
{
	minButton = std::make_shared<CButton>(Point(pos.x, pos.y + 178), "iam015.def", CButton::tooltip(), std::bind(&CSlider::moveToMin,slider), SDLK_m);
}

void CreaturePurhaseCard::initAmountInfo()
{
	availableAmount = std::make_shared<CLabel>(pos.x + 27, pos.y + 146, FONT_SMALL, CENTER, Colors::YELLOW);
	purhaseAmount = std::make_shared<CLabel>(pos.x + 77, pos.y + 146, FONT_SMALL, CENTER, Colors::WHITE);
	updateAmountInfo(0);
}

void CreaturePurhaseCard::updateAmountInfo(int value)
{
	availableAmount->setText(boost::lexical_cast<std::string>(maxamount-value));
	purhaseAmount->setText(boost::lexical_cast<std::string>(value));
}

void CreaturePurhaseCard::initSlider()
{
	slider = std::make_shared<CSlider>(Point(pos.x, pos.y + 158), 102, std::bind(&CreaturePurhaseCard::sliderMoved, this , _1), 0, maxamount, 0);
}

void CreaturePurhaseCard::initCostBox()
{
	cost = std::make_shared<CreatureCostBox>(Rect(pos.x, pos.y + 194, 97, 74), "");
	cost->createItems(creature->cost);
}


void CreaturePurhaseCard::sliderMoved(int to)
{
	updateAmountInfo(to);
	cost->set(creature->cost * to);
	parent->updateAllSliders();
}

CreaturePurhaseCard::CreaturePurhaseCard(const CCreature * monster, Point p, int amount, QuickRecruitmentWindow * par) : creature(monster), parent(par), maxamount(amount)
{
	moveTo(Point(p.x, p.y));
	initView();
}

void CreaturePurhaseCard::initView()
{
	picture = std::make_shared<CCreaturePic>(pos.x, pos.y, creature);
	initAmountInfo();
	initSlider();
	initButtons();
	initCostBox();
}
