/*
 * CHeroWindow.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../widgets/CWindowWithArtifacts.h"
#include "CWindowObject.h"

#include "../../lib/bonuses/IBonusBearer.h"

#include <vcmi/FactionMember.h>

VCMI_LIB_NAMESPACE_BEGIN

class CGHeroInstance;

VCMI_LIB_NAMESPACE_END

class CButton;
class CHeroWindow;
class LClickableAreaHero;
class LRClickableAreaWText;
class LRClickableAreaWTextComp;
class CArtifactsOfHeroMain;
class MoraleLuckBox;
class CToggleButton;
class CToggleGroup;
class CGStatusBar;
class CTextBox;
class CGarrisonInt;

/// Button which switches hero selection
class CHeroSwitcher : public CIntObject
{
	const CGHeroInstance * hero;
	std::shared_ptr<CAnimImage> image;
	CHeroWindow * owner;
public:
	void clickPressed(const Point & cursorPosition) override;

	CHeroSwitcher(CHeroWindow * owner_, Point pos_, const CGHeroInstance * hero_);
};

class CHeroWindow : public CStatusbarWindow, public IGarrisonHolder, public CWindowWithArtifacts
{
	std::shared_ptr<CLabel> name;
	std::shared_ptr<CLabel> title;

	std::shared_ptr<CAnimImage> banner;

	std::vector<std::shared_ptr<CHeroSwitcher>> heroList;
	std::shared_ptr<CPicture> listSelection;

	std::shared_ptr<LRClickableAreaWText> portraitArea;
	std::shared_ptr<CAnimImage> portraitImage;

	std::vector<std::shared_ptr<LRClickableAreaWTextComp>> primSkillAreas;
	std::vector<std::shared_ptr<CAnimImage>> primSkillImages;
	std::vector<std::shared_ptr<CLabel>> primSkillValues;

	std::shared_ptr<CLabel> expValue;
	std::shared_ptr<LRClickableAreaWText> expArea;

	std::shared_ptr<CLabel> manaValue;
	std::shared_ptr<LRClickableAreaWText> spellPointsArea;

	std::shared_ptr<LRClickableAreaWText> specArea;
	std::shared_ptr<CAnimImage> specImage;
	std::shared_ptr<CLabel> specName;
	std::shared_ptr<MoraleLuckBox> morale;
	std::shared_ptr<MoraleLuckBox> luck;
	std::vector<std::shared_ptr<LRClickableAreaWTextComp>> secSkillAreas;
	std::vector<std::shared_ptr<CAnimImage>> secSkillImages;
	std::vector<std::shared_ptr<CLabel>> secSkillNames;
	std::vector<std::shared_ptr<CLabel>> secSkillValues;

	std::shared_ptr<CButton> quitButton;
	std::shared_ptr<CTextBox> dismissLabel;
	std::shared_ptr<CButton> dismissButton;
	std::shared_ptr<CTextBox> questlogLabel;
	std::shared_ptr<CButton> questlogButton;
	std::shared_ptr<CButton> commanderButton;
	std::shared_ptr<CButton> backpackButton;

	std::shared_ptr<CToggleButton> tacticsButton;
	std::shared_ptr<CToggleGroup> formations;

	std::shared_ptr<CGarrisonInt> garr;
	std::shared_ptr<CArtifactsOfHeroMain> arts;

	std::vector<std::shared_ptr<CLabel>> labels;

public:
	const CGHeroInstance * curHero;

	CHeroWindow(const CGHeroInstance * hero);

	void update(const CGHeroInstance * hero, bool redrawNeeded = false); //sets main displayed hero

	void dismissCurrent(); //dissmissed currently displayed hero (curHero)
	void commanderWindow();
	void switchHero(); //changes displayed hero
	void updateGarrisons() override;
	bool holdsGarrison(const CArmedInstance * army) override;
	void createBackpackWindow();

	//friends
	friend class CPlayerInterface;
};
