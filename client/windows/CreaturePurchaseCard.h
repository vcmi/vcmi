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

	// This just wraps a clickeable area. There's a weird layout scheme in the file and
	// it's easier to just add a separate invisble box on top
	class CCreatureClickArea : public CIntObject
	{
	public:
		CCreatureClickArea(const Point & pos, const std::shared_ptr<CCreaturePic> creaturePic, const CCreature * creatureOnTheCard);
		void clickRight(tribool down, bool previousState) override;
		const CCreature * creatureOnTheCard;

		// These are obtained by guessing and checking. I'm not sure how the other numbers
		// used to set positions were obtained; commit messages don't document it
		static constexpr int CREATURE_WIDTH = 110;
		static constexpr int CREATURE_HEIGHT = 132;
		static constexpr int CREATURE_X_POS = 15;
		static constexpr int CREATURE_Y_POS = 44;
	};

	std::shared_ptr<CButton> maxButton, minButton, creatureSwitcher;
	std::shared_ptr<CLabel> availableAmount, purchaseAmount;
	std::shared_ptr<CCreaturePic> picture;
	std::shared_ptr<CreatureCostBox> cost;
	std::vector<CreatureID> upgradesID;
	std::shared_ptr<CPicture> background;
	std::shared_ptr<CCreatureClickArea> creatureClickArea;
};
