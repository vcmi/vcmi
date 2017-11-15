/*
 * CreaturePurhaseCard.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../widgets/Images.h"

class CCreaturePic;
class CSlider;
class CButton;
class CreatureCostBox;
class QuickRecruitmentWindow;

class CreaturePurhaseCard : public CIntObject
{
public:
	const CCreature * creatureOnTheCard;
	std::shared_ptr<CSlider> slider;
	QuickRecruitmentWindow * parent;
	int maxAmount;
	void sliderMoved(int to);
	CreaturePurhaseCard(const CCreature * creature, Point position, int creaturesMaxAmount, QuickRecruitmentWindow * parents);

private:
	void initView();

	void initButtons();
	void initMaxButton();
	void initMinButton();

	void initAmountInfo();
	void updateAmountInfo(int value);

	void initSlider();

	void initCostBox();

	std::shared_ptr<CButton> maxButton, minButton;
	std::shared_ptr<CLabel> availableAmount,  purhaseAmount;
	std::shared_ptr<CCreaturePic> picture;
	std::shared_ptr<CreatureCostBox> cost;

};
