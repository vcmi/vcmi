/*
 * CreaturePurchaseCard.h, part of VCMI engine
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

class CreaturePurchaseCard : public CIntObject
{
public:
	const CCreature * creatureOnTheCard;
	std::shared_ptr<CSlider> slider;
	QuickRecruitmentWindow * parent;
	int maxAmount;
	void sliderMoved(int to);
	CreaturePurchaseCard(const std::vector<CreatureID> & creaturesID, Point position, int creaturesMaxAmount, QuickRecruitmentWindow * parents);
private:
	void initView();

	void initButtons();
	void initMaxButton();
	void initMinButton();
	void initCreatureSwitcherButton();
	void switchCreatureLevel();

	void initAmountInfo();
	void updateAmountInfo(int value);

	void initSlider();

	void initCostBox();

	std::shared_ptr<CButton> maxButton, minButton, creatureSwitcher;
	std::shared_ptr<CLabel> availableAmount,  purhaseAmount;
	std::shared_ptr<CCreaturePic> picture;
	std::shared_ptr<CreatureCostBox> cost;
	std::vector<CreatureID> upgradesID;
	std::shared_ptr<CPicture> background;
};
