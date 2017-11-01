/*
 * QuickRecruitmentWindow.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../widgets/CGarrisonInt.h"

class CButton;
class CreatureCostBox;
class CreaturePurhaseCard;
class CFilledTexture;

class QuickRecruitmentWindow : public CWindowObject
{
public:
	int getAvailableCreatures();
	void updateAllSliders();
	QuickRecruitmentWindow(const CGTownInstance * townd);

private:
	void initWindow();

	void setButtons();
	void setCancelButton();
	void setBuyButton();
	void setMaxButton();

	void setCreaturePurhaseCards();

	void maxAllCards(std::vector<std::shared_ptr<CreaturePurhaseCard>> cards);
	void maxAllSlidersAmount(std::vector<std::shared_ptr<CreaturePurhaseCard>> cards);
	void purhaseUnits();
	const CGTownInstance * town;
	std::shared_ptr<CButton> maxButton, buyButton, cancelButton;
	std::shared_ptr<CreatureCostBox> totalCost;
	std::vector<std::shared_ptr<CreaturePurhaseCard>> cards;
	std::shared_ptr<CFilledTexture> backgroundTexture;
};
