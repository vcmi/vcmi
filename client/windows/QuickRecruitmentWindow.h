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

#include "../windows/CWindowObject.h"

VCMI_LIB_NAMESPACE_BEGIN
class CGTownInstance;
VCMI_LIB_NAMESPACE_END

class CButton;
class CreatureCostBox;
class CreaturePurchaseCard;
class CFilledTexture;

class QuickRecruitmentWindow : public CWindowObject
{
public:
	int getAvailableCreatures();
	void updateAllSliders();
	QuickRecruitmentWindow(const CGTownInstance * townd, Rect startupPosition);

private:
	void initWindow(Rect startupPosition);

	void setButtons();
	void setCancelButton();
	void setBuyButton();
	void setMaxButton();

	void setCreaturePurchaseCards();

	void maxAllCards(std::vector<std::shared_ptr<CreaturePurchaseCard>> cards);
	void maxAllSlidersAmount(std::vector<std::shared_ptr<CreaturePurchaseCard>> cards);
	void purchaseUnits();

	const CGTownInstance * town;
	std::shared_ptr<CButton> maxButton, buyButton, cancelButton;
	std::shared_ptr<CreatureCostBox> totalCost;
	std::vector<std::shared_ptr<CreaturePurchaseCard>> cards;
	std::shared_ptr<CFilledTexture> backgroundTexture;
	std::shared_ptr<CPicture> costBackground;
};
