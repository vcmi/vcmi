/*
 * CHeroExchangeWindow.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../../lib/HeroBonus.h"
#include "../widgets/CArtifactHolder.h"
#include "../widgets/CGarrisonInt.h"
#include "../gui/GuiController.h"
#include "../../lib/GameConstants.h"
#include "../../lib/ResourceSet.h"
#include "../../lib/CConfigHandler.h"
#include "../widgets/CGarrisonInt.h"
#include "../widgets/Images.h"
#include "../windows/CWindowObject.h"

class CButton;
struct SDL_Surface;
class CGHeroInstance;
class CArtifact;
class CHeroWindow;
class LClickableAreaHero;
class LRClickableAreaWText;
class LRClickableAreaWTextComp;
class CArtifactsOfHero;
class MoraleLuckBox;
class CToggleButton;
class CToggleGroup;
class CGStatusBar;
class CTextBox;
class CGHeroInstance;
class CCallback;
class CExchangeWindow;
class CHeroWithMaybePickedArtifact;

struct HeroArtifact
{
	const CGHeroInstance * hero;
	const CArtifactInstance * artifact;
	ArtifactPosition artPosition;

	HeroArtifact(const CGHeroInstance * hero, const CArtifactInstance * artifact, ArtifactPosition artPosition)
		:hero(hero), artifact(artifact), artPosition(artPosition)
	{
	}
};

class CExchangeController : public IUiController
{
private:
	const CGHeroInstance * left;
	const CGHeroInstance * right;
	std::shared_ptr<CCallback> cb;
	CExchangeWindow * view;

public:
	CExchangeController(CExchangeWindow * view, ObjectInstanceID hero1, ObjectInstanceID hero2);
	virtual std::map<std::string, std::function<void(const THandlerArgs & args)>> getHandlers() override;

private:
	void moveArmy(bool leftToRight);
	void moveArtifacts(bool leftToRight);
	void moveArtifact(const CGHeroInstance * source, const CGHeroInstance * target, ArtifactPosition srcPosition);
	void moveStack(const CGHeroInstance * source, const CGHeroInstance * target, SlotID sourceSlot);
	void swapArtifacts(ArtifactPosition artPosition);
	std::vector<HeroArtifact> moveCompositeArtsToBackpack();
	void swapWornArtifacts();
	void swapBackpackArtifacts();
	void swapArtifacts();
	void swapArmy();
};

class CExchangeWindow : public CStatusbarWindow, public CGarrisonHolder, public CWindowWithArtifacts
{
	std::array<std::shared_ptr<CHeroWithMaybePickedArtifact>, 2> herosWArt;

	std::array<std::shared_ptr<CLabel>, 2> titles;
	std::vector<std::shared_ptr<CAnimImage>> primSkillImages;//shared for both heroes
	std::array<std::vector<std::shared_ptr<CLabel>>, 2> primSkillValues;
	std::array<std::vector<std::shared_ptr<CAnimImage>>, 2> secSkillIcons;
	std::array<std::shared_ptr<CAnimImage>, 2> specImages;
	std::array<std::shared_ptr<CAnimImage>, 2> expImages;
	std::array<std::shared_ptr<CLabel>, 2> expValues;
	std::array<std::shared_ptr<CAnimImage>, 2> manaImages;
	std::array<std::shared_ptr<CLabel>, 2> manaValues;
	std::array<std::shared_ptr<CAnimImage>, 2> portraits;

	std::vector<std::shared_ptr<LRClickableAreaWTextComp>> primSkillAreas;
	std::array<std::vector<std::shared_ptr<LRClickableAreaWTextComp>>, 2> secSkillAreas;

	std::array<std::shared_ptr<CHeroArea>, 2> heroAreas;
	std::array<std::shared_ptr<LRClickableAreaWText>, 2> specialtyAreas;
	std::array<std::shared_ptr<LRClickableAreaWText>, 2> experienceAreas;
	std::array<std::shared_ptr<LRClickableAreaWText>, 2> spellPointsAreas;

	std::array<std::shared_ptr<MoraleLuckBox>, 2> morale;
	std::array<std::shared_ptr<MoraleLuckBox>, 2> luck;

	std::shared_ptr<CButton> quit;
	std::array<std::shared_ptr<CButton>, 2> questlogButton;

	std::shared_ptr<CGarrisonInt> garr;

	std::shared_ptr<CIntObject> root;
	CExchangeController controller;

public:
	std::array<const CGHeroInstance *, 2> heroInst;
	std::array<std::shared_ptr<CArtifactsOfHero>, 2> artifs;

	void updateGarrisons() override;

	void questlog(int whichHero); //questlog button callback; whichHero: 0 - left, 1 - right

	void updateWidgets();

	CExchangeWindow(ObjectInstanceID hero1, ObjectInstanceID hero2, QueryID queryID);
	~CExchangeWindow();
};