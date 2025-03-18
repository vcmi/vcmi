/*
 * CExchangeWindow.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "CExchangeWindow.h"

#include "CHeroBackpackWindow.h"

#include "../CPlayerInterface.h"

#include "../GameEngine.h"
#include "../GameInstance.h"
#include "../gui/CursorHandler.h"
#include "../gui/Shortcut.h"
#include "../gui/WindowHandler.h"

#include "../widgets/CGarrisonInt.h"
#include "../widgets/Images.h"
#include "../widgets/Buttons.h"
#include "../widgets/TextControls.h"

#include "../render/IRenderHandler.h"

#include "../../CCallback.h"

#include "../lib/CSkillHandler.h"
#include "../lib/GameLibrary.h"
#include "../lib/entities/hero/CHeroHandler.h"
#include "../lib/filesystem/Filesystem.h"
#include "../lib/mapObjects/CGHeroInstance.h"
#include "../lib/texts/CGeneralTextHandler.h"
#include "../lib/texts/TextOperations.h"

static const std::string QUICK_EXCHANGE_BG = "quick-exchange/TRADEQE";

static bool isQuickExchangeLayoutAvailable()
{
	return CResourceHandler::get()->existsResource(ImagePath::builtin("SPRITES/" + QUICK_EXCHANGE_BG));
}

CExchangeWindow::CExchangeWindow(ObjectInstanceID hero1, ObjectInstanceID hero2, QueryID queryID)
	: CWindowObject(PLAYER_COLORED | BORDERED, ImagePath::builtin(isQuickExchangeLayoutAvailable() ? QUICK_EXCHANGE_BG : "TRADE2")),
	controller(hero1, hero2)
{
	const bool qeLayout = isQuickExchangeLayoutAvailable();

	OBJECT_CONSTRUCTION;
	addUsedEvents(KEYBOARD);

	heroInst[0] = GAME->interface()->cb->getHero(hero1);
	heroInst[1] = GAME->interface()->cb->getHero(hero2);

	auto genTitle = [](const CGHeroInstance * h)
	{
		boost::format fmt(LIBRARY->generaltexth->allTexts[138]);
		fmt % h->getNameTranslated() % h->level % h->getClassNameTranslated();
		return boost::str(fmt);
	};

	titles[0] = std::make_shared<CLabel>(147, 25, FONT_SMALL, ETextAlignment::CENTER, Colors::WHITE, genTitle(heroInst[0]));
	titles[1] = std::make_shared<CLabel>(653, 25, FONT_SMALL, ETextAlignment::CENTER, Colors::WHITE, genTitle(heroInst[1]));

	for(int g = 0; g < 4; ++g)
	{
		if (qeLayout)
			primSkillImages.push_back(std::make_shared<CAnimImage>(AnimationPath::builtin("PSKIL32"), g, Rect(389, 12 + 26 * g, 22, 22)));
		else
			primSkillImages.push_back(std::make_shared<CAnimImage>(AnimationPath::builtin("PSKIL32"), g, 0, 385, 19 + 36 * g));
	}

	for(int leftRight : {0, 1})
	{
		const CGHeroInstance * hero = heroInst.at(leftRight);

		for(int m=0; m<GameConstants::PRIMARY_SKILLS; ++m)
			primSkillValues[leftRight].push_back(std::make_shared<CLabel>(352 + (qeLayout ? 96 : 93) * leftRight, (qeLayout ? 22 : 35) + (qeLayout ? 26 : 36) * m, FONT_SMALL, ETextAlignment::CENTER, Colors::WHITE));


		for(int m=0; m < hero->secSkills.size(); ++m)
			secSkills[leftRight].push_back(std::make_shared<CSecSkillPlace>(Point(32 + 36 * m + 454 * leftRight, qeLayout ? 83 : 88), CSecSkillPlace::ImageSize::SMALL,
				hero->secSkills[m].first, hero->secSkills[m].second));

		specImages[leftRight] = std::make_shared<CAnimImage>(AnimationPath::builtin("UN32"), hero->getHeroType()->imageIndex, 0, 67 + 490 * leftRight, qeLayout ? 41 : 45);

		expImages[leftRight] = std::make_shared<CAnimImage>(AnimationPath::builtin("PSKIL32"), 4, 0, 103 + 490 * leftRight, qeLayout ? 41 : 45);
		expValues[leftRight] = std::make_shared<CLabel>(119 + 490 * leftRight, qeLayout ? 66 : 71, FONT_SMALL, ETextAlignment::CENTER, Colors::WHITE);

		manaImages[leftRight] = std::make_shared<CAnimImage>(AnimationPath::builtin("PSKIL32"), 5, 0, 139 + 490 * leftRight, qeLayout ? 41 : 45);
		manaValues[leftRight] = std::make_shared<CLabel>(155 + 490 * leftRight, qeLayout ? 66 : 71, FONT_SMALL, ETextAlignment::CENTER, Colors::WHITE);
	}

	artifs[0] = std::make_shared<CArtifactsOfHeroMain>(Point(-334, 151));
	artifs[0]->clickPressedCallback = [this, hero = heroInst[0]](const CArtPlace & artPlace, const Point & cursorPosition){clickPressedOnArtPlace(hero, artPlace.slot, true, false, false, cursorPosition);};
	artifs[0]->showPopupCallback = [this, heroArts = artifs[0]](CArtPlace & artPlace, const Point & cursorPosition){showArtifactAssembling(*heroArts, artPlace, cursorPosition);};
	artifs[0]->gestureCallback = [this, hero = heroInst[0]](const CArtPlace & artPlace, const Point & cursorPosition){showQuickBackpackWindow(hero, artPlace.slot, cursorPosition);};
	artifs[0]->setHero(heroInst[0]);
	artifs[1] = std::make_shared<CArtifactsOfHeroMain>(Point(98, 151));
	artifs[1]->clickPressedCallback = [this, hero = heroInst[1]](const CArtPlace & artPlace, const Point & cursorPosition){clickPressedOnArtPlace(hero, artPlace.slot, true, false, false, cursorPosition);};
	artifs[1]->showPopupCallback = [this, heroArts = artifs[1]](CArtPlace & artPlace, const Point & cursorPosition){showArtifactAssembling(*heroArts, artPlace, cursorPosition);};
	artifs[1]->gestureCallback = [this, hero = heroInst[1]](const CArtPlace & artPlace, const Point & cursorPosition){showQuickBackpackWindow(hero, artPlace.slot, cursorPosition);};
	artifs[1]->setHero(heroInst[1]);


	addSet(artifs[0]);
	addSet(artifs[1]);

	for(int g=0; g<4; ++g)
	{
		primSkillAreas.push_back(std::make_shared<LRClickableAreaWTextComp>());
		if (qeLayout)
			primSkillAreas[g]->pos = Rect(Point(pos.x + 324, pos.y + 12 + 26 * g), Point(152, 22));
		else
			primSkillAreas[g]->pos = Rect(Point(pos.x + 329, pos.y + 19 + 36 * g), Point(140, 32));
		primSkillAreas[g]->text = LIBRARY->generaltexth->arraytxt[2+g];
		primSkillAreas[g]->component = Component( ComponentType::PRIM_SKILL, PrimarySkill(g));
		primSkillAreas[g]->hoverText = LIBRARY->generaltexth->heroscrn[1];
		boost::replace_first(primSkillAreas[g]->hoverText, "%s", LIBRARY->generaltexth->primarySkillNames[g]);
	}

	//heroes related thing
	for(int b=0; b < heroInst.size(); b++)
	{
		const CGHeroInstance * hero = heroInst.at(b);

		heroAreas[b] = std::make_shared<CHeroArea>(257 + 228 * b, 13, hero);
		heroAreas[b]->addClickCallback([this, hero]() -> void
									   {
										   if(getPickedArtifact() == nullptr)
											   GAME->interface()->openHeroWindow(hero);
									   });

		specialtyAreas[b] = std::make_shared<LRClickableAreaWText>();
		specialtyAreas[b]->pos = Rect(Point(pos.x + 69 + 490 * b, pos.y + (qeLayout ? 41 : 45)), Point(32, 32));
		specialtyAreas[b]->hoverText = LIBRARY->generaltexth->heroscrn[27];
		specialtyAreas[b]->text = hero->getHeroType()->getSpecialtyDescriptionTranslated();

		experienceAreas[b] = std::make_shared<LRClickableAreaWText>();
		experienceAreas[b]->pos = Rect(Point(pos.x + 105 + 490 * b, pos.y + (qeLayout ? 41 : 45)), Point(32, 32));
		experienceAreas[b]->hoverText = LIBRARY->generaltexth->heroscrn[9];
		experienceAreas[b]->text = LIBRARY->generaltexth->allTexts[2];
		boost::algorithm::replace_first(experienceAreas[b]->text, "%d", std::to_string(hero->level));
		boost::algorithm::replace_first(experienceAreas[b]->text, "%d", std::to_string(LIBRARY->heroh->reqExp(hero->level+1)));
		boost::algorithm::replace_first(experienceAreas[b]->text, "%d", std::to_string(hero->exp));

		spellPointsAreas[b] = std::make_shared<LRClickableAreaWText>();
		spellPointsAreas[b]->pos = Rect(Point(pos.x + 141 + 490 * b, pos.y + (qeLayout ? 41 : 45)), Point(32, 32));
		spellPointsAreas[b]->hoverText = LIBRARY->generaltexth->heroscrn[22];
		spellPointsAreas[b]->text = LIBRARY->generaltexth->allTexts[205];
		boost::algorithm::replace_first(spellPointsAreas[b]->text, "%s", hero->getNameTranslated());
		boost::algorithm::replace_first(spellPointsAreas[b]->text, "%d", std::to_string(hero->mana));
		boost::algorithm::replace_first(spellPointsAreas[b]->text, "%d", std::to_string(hero->manaLimit()));

		morale[b] = std::make_shared<MoraleLuckBox>(true, Rect(Point(176 + 490 * b, 39), Point(32, 32)), true);
		luck[b] = std::make_shared<MoraleLuckBox>(false,  Rect(Point(212 + 490 * b, 39), Point(32, 32)), true);
	}

	quit = std::make_shared<CButton>(Point(732, 567), AnimationPath::builtin("IOKAY.DEF"), LIBRARY->generaltexth->zelp[600], std::bind(&CExchangeWindow::close, this), EShortcut::GLOBAL_ACCEPT);
	if(queryID.getNum() > 0)
		quit->addCallback([=](){ GAME->interface()->cb->selectionMade(0, queryID); });

	questlogButton[0] = std::make_shared<CButton>(Point( 10, qeLayout ? 39 : 44), AnimationPath::builtin("hsbtns4.def"), CButton::tooltip(LIBRARY->generaltexth->heroscrn[0]), std::bind(&CExchangeWindow::questLogShortcut, this));
	questlogButton[1] = std::make_shared<CButton>(Point(740, qeLayout ? 39 : 44), AnimationPath::builtin("hsbtns4.def"), CButton::tooltip(LIBRARY->generaltexth->heroscrn[0]), std::bind(&CExchangeWindow::questLogShortcut, this));

	Rect barRect(5, 578, 725, 18);
	statusbar = CGStatusBar::create(std::make_shared<CPicture>(background->getSurface(), barRect, 5, 578));

	//garrison interface

	garr = std::make_shared<CGarrisonInt>(Point(69, qeLayout ? 122 : 131), 4, Point(418,0), heroInst[0], heroInst[1], true, true);
	auto splitButtonCallback = [&](){ garr->splitClick(); };
	garr->addSplitBtn(std::make_shared<CButton>( Point( 10, qeLayout ? 122 : 132), AnimationPath::builtin("TSBTNS.DEF"), CButton::tooltip(LIBRARY->generaltexth->tcommands[3]), splitButtonCallback, EShortcut::HERO_ARMY_SPLIT));
	garr->addSplitBtn(std::make_shared<CButton>( Point(744, qeLayout ? 122 : 132), AnimationPath::builtin("TSBTNS.DEF"), CButton::tooltip(LIBRARY->generaltexth->tcommands[3]), splitButtonCallback, EShortcut::HERO_ARMY_SPLIT));

	if(qeLayout)
	{
		buttonMoveUnitsFromLeftToRight = std::make_shared<CButton>(
			Point(325, 118),
			AnimationPath::builtin("quick-exchange/armRight.DEF"),
			CButton::tooltip(LIBRARY->generaltexth->translate("vcmi.quickExchange.moveAllUnits")),
			[this](){ this->moveUnitsShortcut(true); });

		buttonMoveUnitsFromRightToLeft = std::make_shared<CButton>(
			Point(425, 118),
			AnimationPath::builtin("quick-exchange/armLeft.DEF"),
			CButton::tooltip(LIBRARY->generaltexth->translate("vcmi.quickExchange.moveAllUnits")),
			[this](){ this->moveUnitsShortcut(false); });

		buttonMoveArtifactsFromLeftToRight = std::make_shared<CButton>(
			Point(325, 154), AnimationPath::builtin("quick-exchange/artRight.DEF"),
			CButton::tooltip(LIBRARY->generaltexth->translate("vcmi.quickExchange.moveAllArtifacts")),
			[this](){ this->moveArtifactsCallback(true);});

		buttonMoveArtifactsFromRightToLeft = std::make_shared<CButton>(
			Point(425, 154), AnimationPath::builtin("quick-exchange/artLeft.DEF"),
			CButton::tooltip(LIBRARY->generaltexth->translate("vcmi.quickExchange.moveAllArtifacts")),
			[this](){ this->moveArtifactsCallback(false);});

		exchangeUnitsButton = std::make_shared<CButton>(
			Point(377, 118),
			AnimationPath::builtin("quick-exchange/swapAll.DEF"),
			CButton::tooltip(LIBRARY->generaltexth->translate("vcmi.quickExchange.swapAllUnits")),
			[this](){ controller.swapArmy(); });

		exchangeArtifactsButton  = std::make_shared<CButton>(
			Point(377, 154),
			AnimationPath::builtin("quick-exchange/swapAll.DEF"),
			CButton::tooltip(LIBRARY->generaltexth->translate("vcmi.quickExchange.swapAllArtifacts")),
			[this](){ this->swapArtifactsCallback(); });

		backpackButtonLeft = std::make_shared<CButton>(
			Point(325, 518),
			AnimationPath::builtin("heroBackpack"),
			CButton::tooltipLocalized("vcmi.heroWindow.openBackpack"),
			[this](){ this->backpackShortcut(true); });

		backpackButtonRight = std::make_shared<CButton>(
			Point(419, 518),
			AnimationPath::builtin("heroBackpack"),
			CButton::tooltipLocalized("vcmi.heroWindow.openBackpack"),
			[this](){ this->backpackShortcut(false); });

		backpackButtonLeft->setOverlay(std::make_shared<CPicture>(ImagePath::builtin("heroWindow/backpackButtonIcon")));
		backpackButtonRight->setOverlay(std::make_shared<CPicture>(ImagePath::builtin("heroWindow/backpackButtonIcon")));

		auto leftHeroBlock = heroInst[0]->tempOwner != GAME->interface()->cb->getPlayerID();
		auto rightHeroBlock = heroInst[1]->tempOwner != GAME->interface()->cb->getPlayerID();

		buttonMoveUnitsFromLeftToRight->block(leftHeroBlock);
		buttonMoveUnitsFromRightToLeft->block(rightHeroBlock);
		buttonMoveArtifactsFromLeftToRight->block(leftHeroBlock);
		buttonMoveArtifactsFromRightToLeft->block(rightHeroBlock);

		exchangeUnitsButton->block(leftHeroBlock || rightHeroBlock);
		exchangeArtifactsButton->block(leftHeroBlock || rightHeroBlock);

		backpackButtonLeft->block(leftHeroBlock);
		backpackButtonRight->block(rightHeroBlock);

		for(int i = 0; i < GameConstants::ARMY_SIZE; i++)
		{
			moveUnitFromRightToLeftButtons.push_back(
				std::make_shared<CButton>(
					Point(484 + 35 * i, 154),
					AnimationPath::builtin("quick-exchange/unitLeft.DEF"),
					CButton::tooltip(LIBRARY->generaltexth->translate("vcmi.quickExchange.moveUnit")),
					[this, i]() { creatureArrowButtonCallback(false, SlotID(i)); }));
			moveUnitFromRightToLeftButtons.back()->block(leftHeroBlock);

			moveUnitFromLeftToRightButtons.push_back(
				std::make_shared<CButton>(
					Point(66 + 35 * i, 154),
					AnimationPath::builtin("quick-exchange/unitRight.DEF"),
					CButton::tooltip(LIBRARY->generaltexth->translate("vcmi.quickExchange.moveUnit")),
					[this, i]() { creatureArrowButtonCallback(true, SlotID(i)); }));
			moveUnitFromLeftToRightButtons.back()->block(rightHeroBlock);
		}
	}

	CExchangeWindow::update();
}

void CExchangeWindow::creatureArrowButtonCallback(bool leftToRight, SlotID slotId)
{
	if (ENGINE->isKeyboardAltDown())
		controller.moveArmy(leftToRight, slotId);
	else if (ENGINE->isKeyboardCtrlDown())
		controller.moveSingleStackCreature(leftToRight, slotId, true);
	else if (ENGINE->isKeyboardShiftDown())
		controller.moveSingleStackCreature(leftToRight, slotId, false);
	else
		controller.moveStack(leftToRight, slotId);
}

void CExchangeWindow::moveArtifactsCallback(bool leftToRight)
{
	bool moveEquipped = !ENGINE->isKeyboardShiftDown();
	bool moveBackpack = !ENGINE->isKeyboardCmdDown();
	controller.moveArtifacts(leftToRight, moveEquipped, moveBackpack);
};

void CExchangeWindow::swapArtifactsCallback()
{
	bool moveEquipped = !ENGINE->isKeyboardShiftDown();
	bool moveBackpack = !ENGINE->isKeyboardCmdDown();
	controller.swapArtifacts(moveEquipped, moveBackpack);
}

void CExchangeWindow::moveUnitsShortcut(bool leftToRight)
{
	std::optional<SlotID> slotId = std::nullopt;
	if(const auto * slot = getSelectedSlotID())
		slotId = slot->getSlot();
	controller.moveArmy(leftToRight, slotId);
};

void CExchangeWindow::backpackShortcut(bool leftHero)
{
	ENGINE->windows().createAndPushWindow<CHeroBackpackWindow>(heroInst[leftHero ? 0 : 1], artSets);
};

void CExchangeWindow::keyPressed(EShortcut key)
{
	switch (key)
	{
		case EShortcut::EXCHANGE_ARMY_TO_LEFT:
			moveUnitsShortcut(false);
		break;
		case EShortcut::EXCHANGE_ARMY_TO_RIGHT:
			moveUnitsShortcut(true);
		break;
		case EShortcut::EXCHANGE_ARMY_SWAP:
			controller.swapArmy();
		break;
		case EShortcut::EXCHANGE_ARTIFACTS_TO_LEFT:
			controller.moveArtifacts(false, true, true);
		break;
		case EShortcut::EXCHANGE_ARTIFACTS_TO_RIGHT:
			controller.moveArtifacts(true, true, true);
		break;
		case EShortcut::EXCHANGE_ARTIFACTS_SWAP:
			controller.swapArtifacts(true, true);
		break;
		case EShortcut::EXCHANGE_EQUIPPED_TO_LEFT:
			controller.moveArtifacts(false, true, false);
		break;
		case EShortcut::EXCHANGE_EQUIPPED_TO_RIGHT:
			controller.moveArtifacts(true, true, false);
		break;
		case EShortcut::EXCHANGE_EQUIPPED_SWAP:
			controller.swapArtifacts(true, false);
		break;
		case EShortcut::EXCHANGE_BACKPACK_TO_LEFT:
			controller.moveArtifacts(false, false, true);
		break;
		case EShortcut::EXCHANGE_BACKPACK_TO_RIGHT:
			controller.moveArtifacts(true, false, true);
		break;
		case EShortcut::EXCHANGE_BACKPACK_SWAP:
			controller.swapArtifacts(false, true);
		break;
		case EShortcut::EXCHANGE_BACKPACK_LEFT:
			backpackShortcut(true);
		break;
		case EShortcut::EXCHANGE_BACKPACK_RIGHT:
			backpackShortcut(false);
		break;
	}
}

const CGarrisonSlot * CExchangeWindow::getSelectedSlotID() const
{
	return garr->getSelection();
}

void CExchangeWindow::updateGarrisons()
{
	garr->recreateSlots();

	update();
}

bool CExchangeWindow::holdsGarrison(const CArmedInstance * army)
{
	return garr->upperArmy() == army || garr->lowerArmy() == army;
}

void CExchangeWindow::questLogShortcut()
{
	ENGINE->cursor().dragAndDropCursor(nullptr);
	GAME->interface()->showQuestLog();
}

void CExchangeWindow::update()
{
	CWindowWithArtifacts::update();

	for(size_t leftRight : {0, 1})
	{
		const CGHeroInstance * hero = heroInst.at(leftRight);

		for(int m=0; m<GameConstants::PRIMARY_SKILLS; ++m)
		{
			auto value = heroInst[leftRight]->getPrimSkillLevel(static_cast<PrimarySkill>(m));
			primSkillValues[leftRight][m]->setText(std::to_string(value));
		}

		for(int m=0; m < hero->secSkills.size(); ++m)
		{
			int id = hero->secSkills[m].first;
			int level = hero->secSkills[m].second;

			secSkills[leftRight][m]->setSkill(id, level);
		}

		expValues[leftRight]->setText(TextOperations::formatMetric(hero->exp, 3));
		manaValues[leftRight]->setText(TextOperations::formatMetric(hero->mana, 3));

		morale[leftRight]->set(hero);
		luck[leftRight]->set(hero);
	}
}

