/*
 * CExchangeWindow.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "CWindowWithArtifacts.h"
#include "../widgets/CExchangeController.h"

class CGarrisonSlot;
class CMultiLineLabel;
class LRClickableAreaWText;

class CExchangeWindow : public CStatusbarWindow, public IGarrisonHolder, public CWindowWithArtifacts
{
	std::array<std::shared_ptr<CLabel>, 2> titles;
	std::vector<std::shared_ptr<CAnimImage>> primSkillImages;//shared for both heroes
	std::array<std::vector<std::shared_ptr<CLabel>>, 2> primSkillValues;
	std::array<std::shared_ptr<CAnimImage>, 2> specImages;
	std::array<std::shared_ptr<CAnimImage>, 2> expImages;
	std::array<std::shared_ptr<CLabel>, 2> expValues;
	std::array<std::shared_ptr<CAnimImage>, 2> manaImages;
	std::array<std::shared_ptr<CLabel>, 2> manaValues;

	std::vector<std::shared_ptr<LRClickableAreaWTextComp>> primSkillAreas;
	std::array<std::vector<std::shared_ptr<CSecSkillPlace>>, 2> secSkills;
	std::array<std::shared_ptr<CMultiLineLabel>, 2> secSkillsFull;
	std::array<std::shared_ptr<LRClickableAreaWText>, 2> secSkillsFullArea;

	std::array<std::shared_ptr<CHeroArea>, 2> heroAreas;
	std::array<std::shared_ptr<LRClickableAreaWText>, 2> specialtyAreas;
	std::array<std::shared_ptr<LRClickableAreaWText>, 2> experienceAreas;
	std::array<std::shared_ptr<LRClickableAreaWText>, 2> spellPointsAreas;

	std::array<std::shared_ptr<MoraleLuckBox>, 2> morale;
	std::array<std::shared_ptr<MoraleLuckBox>, 2> luck;

	std::shared_ptr<CButton> quit;
	std::array<std::shared_ptr<CButton>, 2> questlogButton;

	std::shared_ptr<CGarrisonInt> garr;
	std::shared_ptr<CButton> buttonMoveUnitsFromLeftToRight;
	std::shared_ptr<CButton> buttonMoveUnitsFromRightToLeft;
	std::shared_ptr<CButton> buttonMoveArtifactsFromLeftToRight;
	std::shared_ptr<CButton> buttonMoveArtifactsFromRightToLeft;

	std::shared_ptr<CButton> exchangeUnitsButton;
	std::shared_ptr<CButton> exchangeArtifactsButton;

	std::vector<std::shared_ptr<CButton>> moveUnitFromLeftToRightButtons;
	std::vector<std::shared_ptr<CButton>> moveUnitFromRightToLeftButtons;
	std::shared_ptr<CButton> backpackButtonLeft;
	std::shared_ptr<CButton> backpackButtonRight;
	CExchangeController controller;

	void creatureArrowButtonCallback(bool leftToRight, SlotID slotID);
	void moveArtifactsCallback(bool leftToRight);
	void swapArtifactsCallback();
	void moveUnitsShortcut(bool leftToRight);
	void backpackShortcut(bool leftHero);
	void questLogShortcut();

	std::array<const CGHeroInstance *, 2> heroInst;
	std::array<std::shared_ptr<CArtifactsOfHeroMain>, 2> artifs;

	const CGarrisonSlot * getSelectedSlotID() const;

public:
	CExchangeWindow(ObjectInstanceID hero1, ObjectInstanceID hero2, QueryID queryID);

	void keyPressed(EShortcut key) override;

	void update() override;

	// IGarrisonHolder impl
	void updateGarrisons() override;
	bool holdsGarrison(const CArmedInstance * army) override;

};
