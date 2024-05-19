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

#include "../CGameInfo.h"
#include "../CPlayerInterface.h"

#include "../gui/CGuiHandler.h"
#include "../gui/CursorHandler.h"
#include "../gui/Shortcut.h"
#include "../gui/WindowHandler.h"

#include "../widgets/CGarrisonInt.h"
#include "../widgets/Images.h"
#include "../widgets/Buttons.h"
#include "../widgets/TextControls.h"

#include "../render/IRenderHandler.h"
#include "../render/CAnimation.h"

#include "../../CCallback.h"

#include "../lib/mapObjects/CGHeroInstance.h"
#include "../lib/CGeneralTextHandler.h"
#include "../lib/CHeroHandler.h"
#include "../lib/filesystem/Filesystem.h"
#include "../lib/CSkillHandler.h"
#include "../lib/TextOperations.h"

static const std::string QUICK_EXCHANGE_MOD_PREFIX = "quick-exchange";
static const std::string QUICK_EXCHANGE_BG = QUICK_EXCHANGE_MOD_PREFIX + "/TRADEQE";

static bool isQuickExchangeLayoutAvailable()
{
	return CResourceHandler::get()->existsResource(ImagePath::builtin("SPRITES/" + QUICK_EXCHANGE_BG));
}

CExchangeWindow::CExchangeWindow(ObjectInstanceID hero1, ObjectInstanceID hero2, QueryID queryID)
	: CWindowObject(PLAYER_COLORED | BORDERED, ImagePath::builtin(isQuickExchangeLayoutAvailable() ? QUICK_EXCHANGE_BG : "TRADE2")),
	controller(hero1, hero2),
	moveStackLeftButtons(),
	moveStackRightButtons()
{
	const bool qeLayout = isQuickExchangeLayoutAvailable();

	OBJECT_CONSTRUCTION_CAPTURING(255-DISPOSE);

	heroInst[0] = LOCPLINT->cb->getHero(hero1);
	heroInst[1] = LOCPLINT->cb->getHero(hero2);

	auto genTitle = [](const CGHeroInstance * h)
	{
		boost::format fmt(CGI->generaltexth->allTexts[138]);
		fmt % h->getNameTranslated() % h->level % h->getClassNameTranslated();
		return boost::str(fmt);
	};

	titles[0] = std::make_shared<CLabel>(147, 25, FONT_SMALL, ETextAlignment::CENTER, Colors::WHITE, genTitle(heroInst[0]));
	titles[1] = std::make_shared<CLabel>(653, 25, FONT_SMALL, ETextAlignment::CENTER, Colors::WHITE, genTitle(heroInst[1]));

	auto PSKIL32 = GH.renderHandler().loadAnimation(AnimationPath::builtin("PSKIL32"));
	PSKIL32->preload();

	auto SECSK32 = GH.renderHandler().loadAnimation(AnimationPath::builtin("SECSK32"));

	for(int g = 0; g < 4; ++g)
	{
		if (qeLayout)
			primSkillImages.push_back(std::make_shared<CAnimImage>(PSKIL32, g, Rect(389, 12 + 26 * g, 22, 22)));
		else
			primSkillImages.push_back(std::make_shared<CAnimImage>(PSKIL32, g, 0, 385, 19 + 36 * g));
	}

	for(int leftRight : {0, 1})
	{
		const CGHeroInstance * hero = heroInst.at(leftRight);

		for(int m=0; m<GameConstants::PRIMARY_SKILLS; ++m)
			primSkillValues[leftRight].push_back(std::make_shared<CLabel>(352 + (qeLayout ? 96 : 93) * leftRight, (qeLayout ? 22 : 35) + (qeLayout ? 26 : 36) * m, FONT_SMALL, ETextAlignment::CENTER, Colors::WHITE));


		for(int m=0; m < hero->secSkills.size(); ++m)
			secSkillIcons[leftRight].push_back(std::make_shared<CAnimImage>(SECSK32, 0, 0, 32 + 36 * m + 454 * leftRight, qeLayout ? 83 : 88));

		specImages[leftRight] = std::make_shared<CAnimImage>(AnimationPath::builtin("UN32"), hero->type->imageIndex, 0, 67 + 490 * leftRight, qeLayout ? 41 : 45);

		expImages[leftRight] = std::make_shared<CAnimImage>(PSKIL32, 4, 0, 103 + 490 * leftRight, qeLayout ? 41 : 45);
		expValues[leftRight] = std::make_shared<CLabel>(119 + 490 * leftRight, qeLayout ? 66 : 71, FONT_SMALL, ETextAlignment::CENTER, Colors::WHITE);

		manaImages[leftRight] = std::make_shared<CAnimImage>(PSKIL32, 5, 0, 139 + 490 * leftRight, qeLayout ? 41 : 45);
		manaValues[leftRight] = std::make_shared<CLabel>(155 + 490 * leftRight, qeLayout ? 66 : 71, FONT_SMALL, ETextAlignment::CENTER, Colors::WHITE);
	}

	artifs[0] = std::make_shared<CArtifactsOfHeroMain>(Point(-334, 151));
	artifs[0]->setHero(heroInst[0]);
	artifs[1] = std::make_shared<CArtifactsOfHeroMain>(Point(98, 151));
	artifs[1]->setHero(heroInst[1]);

	addSetAndCallbacks(artifs[0]);
	addSetAndCallbacks(artifs[1]);

	for(int g=0; g<4; ++g)
	{
		primSkillAreas.push_back(std::make_shared<LRClickableAreaWTextComp>());
		if (qeLayout)
			primSkillAreas[g]->pos = Rect(Point(pos.x + 324, pos.y + 12 + 26 * g), Point(152, 22));
		else
			primSkillAreas[g]->pos = Rect(Point(pos.x + 329, pos.y + 19 + 36 * g), Point(140, 32));
		primSkillAreas[g]->text = CGI->generaltexth->arraytxt[2+g];
		primSkillAreas[g]->component = Component( ComponentType::PRIM_SKILL, PrimarySkill(g));
		primSkillAreas[g]->hoverText = CGI->generaltexth->heroscrn[1];
		boost::replace_first(primSkillAreas[g]->hoverText, "%s", CGI->generaltexth->primarySkillNames[g]);
	}

	//heroes related thing
	for(int b=0; b < heroInst.size(); b++)
	{
		const CGHeroInstance * hero = heroInst.at(b);

		//secondary skill's clickable areas
		for(int g=0; g<hero->secSkills.size(); ++g)
		{
			SecondarySkill skill = hero->secSkills[g].first;
			int level = hero->secSkills[g].second; // <1, 3>
			secSkillAreas[b].push_back(std::make_shared<LRClickableAreaWTextComp>());
			secSkillAreas[b][g]->pos = Rect(Point(pos.x + 32 + g * 36 + b * 454 , pos.y + (qeLayout ? 83 : 88)), Point(32, 32) );
			secSkillAreas[b][g]->component = Component(ComponentType::SEC_SKILL, skill, level);
			secSkillAreas[b][g]->text = CGI->skillh->getByIndex(skill)->getDescriptionTranslated(level);

			secSkillAreas[b][g]->hoverText = CGI->generaltexth->heroscrn[21];
			boost::algorithm::replace_first(secSkillAreas[b][g]->hoverText, "%s", CGI->generaltexth->levels[level - 1]);
			boost::algorithm::replace_first(secSkillAreas[b][g]->hoverText, "%s", CGI->skillh->getByIndex(skill)->getNameTranslated());
		}

		heroAreas[b] = std::make_shared<CHeroArea>(257 + 228 * b, 13, hero);
		heroAreas[b]->addClickCallback([this, hero]() -> void
									   {
										   if(getPickedArtifact() == nullptr)
											   LOCPLINT->openHeroWindow(hero);
									   });

		specialtyAreas[b] = std::make_shared<LRClickableAreaWText>();
		specialtyAreas[b]->pos = Rect(Point(pos.x + 69 + 490 * b, pos.y + (qeLayout ? 41 : 45)), Point(32, 32));
		specialtyAreas[b]->hoverText = CGI->generaltexth->heroscrn[27];
		specialtyAreas[b]->text = hero->type->getSpecialtyDescriptionTranslated();

		experienceAreas[b] = std::make_shared<LRClickableAreaWText>();
		experienceAreas[b]->pos = Rect(Point(pos.x + 105 + 490 * b, pos.y + (qeLayout ? 41 : 45)), Point(32, 32));
		experienceAreas[b]->hoverText = CGI->generaltexth->heroscrn[9];
		experienceAreas[b]->text = CGI->generaltexth->allTexts[2];
		boost::algorithm::replace_first(experienceAreas[b]->text, "%d", std::to_string(hero->level));
		boost::algorithm::replace_first(experienceAreas[b]->text, "%d", std::to_string(CGI->heroh->reqExp(hero->level+1)));
		boost::algorithm::replace_first(experienceAreas[b]->text, "%d", std::to_string(hero->exp));

		spellPointsAreas[b] = std::make_shared<LRClickableAreaWText>();
		spellPointsAreas[b]->pos = Rect(Point(pos.x + 141 + 490 * b, pos.y + (qeLayout ? 41 : 45)), Point(32, 32));
		spellPointsAreas[b]->hoverText = CGI->generaltexth->heroscrn[22];
		spellPointsAreas[b]->text = CGI->generaltexth->allTexts[205];
		boost::algorithm::replace_first(spellPointsAreas[b]->text, "%s", hero->getNameTranslated());
		boost::algorithm::replace_first(spellPointsAreas[b]->text, "%d", std::to_string(hero->mana));
		boost::algorithm::replace_first(spellPointsAreas[b]->text, "%d", std::to_string(hero->manaLimit()));

		morale[b] = std::make_shared<MoraleLuckBox>(true, Rect(Point(176 + 490 * b, 39), Point(32, 32)), true);
		luck[b] = std::make_shared<MoraleLuckBox>(false,  Rect(Point(212 + 490 * b, 39), Point(32, 32)), true);
	}

	quit = std::make_shared<CButton>(Point(732, 567), AnimationPath::builtin("IOKAY.DEF"), CGI->generaltexth->zelp[600], std::bind(&CExchangeWindow::close, this), EShortcut::GLOBAL_ACCEPT);
	if(queryID.getNum() > 0)
		quit->addCallback([=](){ LOCPLINT->cb->selectionMade(0, queryID); });

	questlogButton[0] = std::make_shared<CButton>(Point( 10, qeLayout ? 39 : 44), AnimationPath::builtin("hsbtns4.def"), CButton::tooltip(CGI->generaltexth->heroscrn[0]), std::bind(&CExchangeWindow::questlog, this, 0), EShortcut::ADVENTURE_QUEST_LOG);
	questlogButton[1] = std::make_shared<CButton>(Point(740, qeLayout ? 39 : 44), AnimationPath::builtin("hsbtns4.def"), CButton::tooltip(CGI->generaltexth->heroscrn[0]), std::bind(&CExchangeWindow::questlog, this, 1), EShortcut::ADVENTURE_QUEST_LOG);

	Rect barRect(5, 578, 725, 18);
	statusbar = CGStatusBar::create(std::make_shared<CPicture>(background->getSurface(), barRect, 5, 578));

	//garrison interface

	garr = std::make_shared<CGarrisonInt>(Point(69, qeLayout ? 122 : 131), 4, Point(418,0), heroInst[0], heroInst[1], true, true);
	auto splitButtonCallback = [&](){ garr->splitClick(); };
	garr->addSplitBtn(std::make_shared<CButton>( Point( 10, qeLayout ? 122 : 132), AnimationPath::builtin("TSBTNS.DEF"), CButton::tooltip(CGI->generaltexth->tcommands[3]), splitButtonCallback, EShortcut::HERO_ARMY_SPLIT));
	garr->addSplitBtn(std::make_shared<CButton>( Point(744, qeLayout ? 122 : 132), AnimationPath::builtin("TSBTNS.DEF"), CButton::tooltip(CGI->generaltexth->tcommands[3]), splitButtonCallback, EShortcut::HERO_ARMY_SPLIT));

	if(qeLayout)
	{
		auto moveArtifacts = [](const std::function<void(bool, bool)> moveRoutine) -> void
		{
			bool moveEquipped = true;
			bool moveBackpack = true;

			if(GH.isKeyboardCmdDown())
				moveBackpack = false;
			else if(GH.isKeyboardShiftDown())
				moveEquipped = false;
			moveRoutine(moveEquipped, moveBackpack);
		};

		auto moveArmy = [this](const bool leftToRight) -> void
		{
			std::optional<SlotID> slotId = std::nullopt;
			if(auto slot = getSelectedSlotID())
				slotId = slot->getSlot();
			controller.moveArmy(leftToRight, slotId);
		};

		auto openBackpack = [this](const CGHeroInstance * hero) -> void
		{
			GH.windows().createAndPushWindow<CHeroBackpackWindow>(hero, artSets);
		};

		moveAllGarrButtonLeft    = std::make_shared<CButton>(Point(325, 118), AnimationPath::builtin(QUICK_EXCHANGE_MOD_PREFIX + "/armRight.DEF"), CButton::tooltip(CGI->generaltexth->qeModCommands[1]),
														  std::bind(moveArmy, true), EShortcut::EXCHANGE_ARMY_TO_LEFT);
		exchangeGarrButton       = std::make_shared<CButton>(Point(377, 118), AnimationPath::builtin(QUICK_EXCHANGE_MOD_PREFIX + "/swapAll.DEF"), CButton::tooltip(CGI->generaltexth->qeModCommands[2]),
													   std::bind(&CExchangeController::swapArmy, &controller), EShortcut::EXCHANGE_ARMY_SWAP);
		moveAllGarrButtonRight   = std::make_shared<CButton>(Point(425, 118), AnimationPath::builtin(QUICK_EXCHANGE_MOD_PREFIX + "/armLeft.DEF"), CButton::tooltip(CGI->generaltexth->qeModCommands[1]),
														   std::bind(moveArmy, false), EShortcut::EXCHANGE_ARMY_TO_RIGHT);
		moveArtifactsButtonLeft  = std::make_shared<CButton>(Point(325, 154), AnimationPath::builtin(QUICK_EXCHANGE_MOD_PREFIX + "/artRight.DEF"), CButton::tooltip(CGI->generaltexth->qeModCommands[3]),
															std::bind(moveArtifacts, [this](bool equipped, bool baclpack) -> void {controller.moveArtifacts(true, equipped, baclpack);}), EShortcut::EXCHANGE_ARTIFACTS_TO_LEFT);
		exchangeArtifactsButton  = std::make_shared<CButton>(Point(377, 154), AnimationPath::builtin(QUICK_EXCHANGE_MOD_PREFIX + "/swapAll.DEF"), CButton::tooltip(CGI->generaltexth->qeModCommands[4]),
															std::bind(moveArtifacts, [this](bool equipped, bool baclpack) -> void {controller.swapArtifacts(equipped, baclpack);}), EShortcut::EXCHANGE_ARTIFACTS_SWAP);
		moveArtifactsButtonRight = std::make_shared<CButton>(Point(425, 154), AnimationPath::builtin(QUICK_EXCHANGE_MOD_PREFIX + "/artLeft.DEF"), CButton::tooltip(CGI->generaltexth->qeModCommands[3]),
															 std::bind(moveArtifacts, [this](bool equipped, bool baclpack) -> void {controller.moveArtifacts(false, equipped, baclpack);}), EShortcut::EXCHANGE_ARTIFACTS_TO_RIGHT);
		backpackButtonLeft       = std::make_shared<CButton>(Point(325, 518), AnimationPath::builtin("heroBackpack"), CButton::tooltipLocalized("vcmi.heroWindow.openBackpack"),
													   std::bind(openBackpack, heroInst[0]), EShortcut::EXCHANGE_BACKPACK_LEFT);
		backpackButtonLeft->setOverlay(std::make_shared<CPicture>(ImagePath::builtin("heroWindow/backpackButtonIcon")));
		backpackButtonRight      = std::make_shared<CButton>(Point(419, 518), AnimationPath::builtin("heroBackpack"), CButton::tooltipLocalized("vcmi.heroWindow.openBackpack"),
														std::bind(openBackpack, heroInst[1]), EShortcut::EXCHANGE_BACKPACK_RIGHT);
		backpackButtonRight->setOverlay(std::make_shared<CPicture>(ImagePath::builtin("heroWindow/backpackButtonIcon")));

		auto leftHeroBlock = heroInst[0]->tempOwner != LOCPLINT->cb->getPlayerID();
		auto rightHeroBlock = heroInst[1]->tempOwner != LOCPLINT->cb->getPlayerID();
		moveAllGarrButtonLeft->block(leftHeroBlock);
		exchangeGarrButton->block(leftHeroBlock || rightHeroBlock);
		moveAllGarrButtonRight->block(rightHeroBlock);
		moveArtifactsButtonLeft->block(leftHeroBlock);
		exchangeArtifactsButton->block(leftHeroBlock || rightHeroBlock);
		moveArtifactsButtonRight->block(rightHeroBlock);
		backpackButtonLeft->block(leftHeroBlock);
		backpackButtonRight->block(rightHeroBlock);

		for(int i = 0; i < GameConstants::ARMY_SIZE; i++)
		{
			moveStackLeftButtons.push_back(
				std::make_shared<CButton>(
					Point(484 + 35 * i, 154),
					AnimationPath::builtin(QUICK_EXCHANGE_MOD_PREFIX + "/unitLeft.DEF"),
					CButton::tooltip(CGI->generaltexth->qeModCommands[1]),
					std::bind(&CExchangeController::moveStack, &controller, false, SlotID(i))));
			moveStackLeftButtons.back()->block(leftHeroBlock);

			moveStackRightButtons.push_back(
				std::make_shared<CButton>(
					Point(66 + 35 * i, 154),
					AnimationPath::builtin(QUICK_EXCHANGE_MOD_PREFIX + "/unitRight.DEF"),
					CButton::tooltip(CGI->generaltexth->qeModCommands[1]),
					std::bind(&CExchangeController::moveStack, &controller, true, SlotID(i))));
			moveStackLeftButtons.back()->block(rightHeroBlock);
		}
	}

	updateWidgets();
}

const CGarrisonSlot * CExchangeWindow::getSelectedSlotID() const
{
	return garr->getSelection();
}

void CExchangeWindow::updateGarrisons()
{
	garr->recreateSlots();

	updateWidgets();
}

bool CExchangeWindow::holdsGarrison(const CArmedInstance * army)
{
	return garr->upperArmy() == army || garr->lowerArmy() == army;
}

void CExchangeWindow::questlog(int whichHero)
{
	CCS->curh->dragAndDropCursor(nullptr);
	LOCPLINT->showQuestLog();
}

void CExchangeWindow::updateWidgets()
{
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

			secSkillIcons[leftRight][m]->setFrame(2 + id * 3 + level);
		}

		expValues[leftRight]->setText(TextOperations::formatMetric(hero->exp, 3));
		manaValues[leftRight]->setText(TextOperations::formatMetric(hero->mana, 3));

		morale[leftRight]->set(hero);
		luck[leftRight]->set(hero);
	}
}

