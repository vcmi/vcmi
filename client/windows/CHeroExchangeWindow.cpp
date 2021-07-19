/*
 * CHeroExchangeWindow.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "CHeroExchangeWindow.h"
#include "../../lib/mapObjects/MapObjects.h"
#include "../../lib/CHeroHandler.h"
#include "../../lib/CGeneralTextHandler.h"
#include "../../lib/CSkillHandler.h"
#include "../../lib/CCreatureHandler.h"
#include "../../lib/HeroBonus.h"
#include "../../lib/CModHandler.h"
#include "../../CCallback.h"
#include "../CGameInfo.h"
#include "../gui/CGuiHandler.h"
#include "../gui/SDL_Extensions.h"
#include "../gui/CCursorHandler.h"
#include "../gui/CAnimation.h"
#include "../gui/GuiJsonLoader.h"
#include "../CBitmapHandler.h"
#include "../CMessage.h"
#include "../CMusicHandler.h"
#include "../CPlayerInterface.h"
#include "../CVideoHandler.h"
#include "../Graphics.h"
#include "../mapHandler.h"
#include "../CServerHandler.h"
#include "CAdvmapInterface.h"
#include "CCreatureWindow.h"
#include "CHeroWindow.h"

using namespace CSDL_Ext;

CExchangeController::CExchangeController(CExchangeWindow * view, ObjectInstanceID hero1, ObjectInstanceID hero2)
	:left(LOCPLINT->cb->getHero(hero1)), right(LOCPLINT->cb->getHero(hero2)), cb(LOCPLINT->cb), view(view)
{
}

std::map<std::string, std::function<void(const std::vector<std::string> & args)>> CExchangeController::getHandlers()
{
	return std::map<std::string, std::function<void(const std::vector<std::string> & args)>> {
		{"moveArmyToTheLeft", [this](const THandlerArgs & args) { this->moveArmy(false); }},
		{"moveArmyToTheRight", [this](const THandlerArgs & args) { this->moveArmy(true); }},
		{"swapArmy", [this](const THandlerArgs & args) { this->swapArmy(); }},
		{"moveArtsToTheLeft", [this](const THandlerArgs & args) { this->moveArtifacts(false); }},
		{"moveArtsToTheRight", [this](const THandlerArgs & args) { this->moveArtifacts(true); }},
		{"swapArts", [this](const THandlerArgs & args) { this->swapArtifacts(); }},
		{"moveUnitToTheLeft", [this](const THandlerArgs & args) { this->moveStack(right, left, SlotID(std::stoi(args[0]))); }},
		{"moveUnitToTheRight", [this](const THandlerArgs & args) { this->moveStack(left, right, SlotID(std::stoi(args[0]))); }}
	};
}

void CExchangeController::swapArtifacts(ArtifactPosition slot)
{
	bool leftHasArt = !left->isPositionFree(slot);
	bool rightHasArt = !right->isPositionFree(slot);

	if(!leftHasArt && !rightHasArt)
		return;

	ArtifactLocation leftLocation = ArtifactLocation(left, slot);
	ArtifactLocation rightLocation = ArtifactLocation(right, slot);

	if(leftHasArt && !left->artifactsWorn.at(slot).artifact->canBePutAt(rightLocation, true))
		return;

	if(rightHasArt && !right->artifactsWorn.at(slot).artifact->canBePutAt(leftLocation, true))
		return;

	if(leftHasArt)
	{
		if(rightHasArt)
		{
			auto art = right->getArt(slot);

			cb->swapArtifacts(leftLocation, rightLocation);
			cb->swapArtifacts(ArtifactLocation(right, right->getArtPos(art)), leftLocation);
		}
		else
			cb->swapArtifacts(leftLocation, rightLocation);
	}
	else
	{
		cb->swapArtifacts(rightLocation, leftLocation);
	}
}

std::vector<CArtifactInstance *> getBackpackArts(const CGHeroInstance * hero)
{
	std::vector<CArtifactInstance *> result;

	for(auto slot : hero->artifactsInBackpack)
	{
		result.push_back(slot.artifact);
	}

	return result;
}

const std::vector<ArtifactPosition> unmovablePositions = {ArtifactPosition::SPELLBOOK, ArtifactPosition::MACH4};

bool isArtRemovable(const std::pair<ArtifactPosition, ArtSlotInfo> & slot)
{
	return slot.second.artifact
		&& !slot.second.locked
		&& !vstd::contains(unmovablePositions, slot.first);
}

// Puts all composite arts to backpack and returns their previous location
std::vector<HeroArtifact> CExchangeController::moveCompositeArtsToBackpack()
{
	std::vector<const CGHeroInstance *> sides = {left, right};
	std::vector<HeroArtifact> artPositions;

	for(auto hero : sides)
	{
		for(int i = ArtifactPosition::HEAD; i < ArtifactPosition::AFTER_LAST; i++)
		{
			auto artPosition = ArtifactPosition(i);
			auto art = hero->getArt(artPosition);

			if(art && art->canBeDisassembled())
			{
				cb->swapArtifacts(
					ArtifactLocation(hero, artPosition),
					ArtifactLocation(hero, ArtifactPosition(GameConstants::BACKPACK_START)));

				artPositions.push_back(HeroArtifact(hero, art, artPosition));
			}
		}
	}

	return artPositions;
}

void CExchangeController::swapWornArtifacts()
{
	for(int i = ArtifactPosition::HEAD; i < ArtifactPosition::AFTER_LAST; i++)
	{
		if(vstd::contains(unmovablePositions, i))
			continue;

		swapArtifacts(ArtifactPosition(i));
	}
}

void CExchangeController::swapBackpackArtifacts()
{
	auto leftHeroBackpack = getBackpackArts(left);
	auto rightHeroBackpack = getBackpackArts(right);

	for(auto leftArt : leftHeroBackpack)
	{
		cb->swapArtifacts(
			ArtifactLocation(left, left->getArtPos(leftArt)),
			ArtifactLocation(right, ArtifactPosition(GameConstants::BACKPACK_START)));
	}

	for(auto rightArt : rightHeroBackpack)
	{
		cb->swapArtifacts(
			ArtifactLocation(right, right->getArtPos(rightArt)),
			ArtifactLocation(left, ArtifactPosition(GameConstants::BACKPACK_START)));
	}
}

void CExchangeController::swapArtifacts()
{
	GsThread::run([=]
	{
		// it is not possible directly exchange composite artifacts like Angelic Alliance and Armor of Damned
		auto compositeArtLocations = moveCompositeArtsToBackpack();

		swapWornArtifacts();
		swapBackpackArtifacts();

		for(HeroArtifact artLocation : compositeArtLocations)
		{
			auto target = artLocation.hero == left ? right : left;
			auto currentPos = target->getArtPos(artLocation.artifact);

			cb->swapArtifacts(
				ArtifactLocation(target, currentPos),
				ArtifactLocation(target, artLocation.artPosition));
		}

		view->redraw();
	});
}

std::vector<std::pair<SlotID, CStackInstance *>> getStacks(const CArmedInstance * source)
{
	auto slots = source->Slots();

	return std::vector<std::pair<SlotID, CStackInstance *>>(slots.begin(), slots.end());
}

void CExchangeController::swapArmy()
{
	GsThread::run([=]
	{
		auto leftSlots = getStacks(left);
		auto rightSlots = getStacks(right);

		auto i = leftSlots.begin(), j = rightSlots.begin();

		for(; i != leftSlots.end() && j != rightSlots.end(); i++, j++)
		{
			cb->swapCreatures(left, right, i->first, j->first);
		}

		if(i != leftSlots.end())
		{
			for(; i != leftSlots.end(); i++)
			{
				cb->swapCreatures(left, right, i->first, right->getFreeSlot());
			}
		}
		else if(j != rightSlots.end())
		{
			for(; j != rightSlots.end(); j++)
			{
				cb->swapCreatures(left, right, left->getFreeSlot(), j->first);
			}
		}
	});
}

void CExchangeController::moveStack(
	const CGHeroInstance * source,
	const CGHeroInstance * target,
	SlotID sourceSlot)
{
	auto creature = source->getCreature(sourceSlot);
	SlotID targetSlot = target->getSlotFor(creature);

	if(targetSlot.validSlot())
	{
		if(source->stacksCount() == 1 && source->needsLastStack())
		{
			cb->splitStack(
				source,
				target,
				sourceSlot,
				targetSlot,
				target->getStackCount(targetSlot) + source->getStackCount(sourceSlot) - 1);
		}
		else
		{
			cb->mergeOrSwapStacks(source, target, sourceSlot, targetSlot);
		}
	}
}

void CExchangeController::moveArmy(bool leftToRight)
{
	const CGHeroInstance * source = leftToRight ? left : right;
	const CGHeroInstance * target = leftToRight ? right : left;

	GsThread::run([=]
	{
		auto stacks = getStacks(source);

		std::sort(stacks.begin(), stacks.end(), [](const std::pair<SlotID, CStackInstance *> & a, const std::pair<SlotID, CStackInstance *> & b)
		{
			return a.second->type->level > b.second->type->level;
		});

		for(auto pair : stacks)
		{
			moveStack(source, target, pair.first);
		}
	});
}

void CExchangeController::moveArtifacts(bool leftToRight)
{
	const CGHeroInstance * source = leftToRight ? left : right;
	const CGHeroInstance * target = leftToRight ? right : left;

	GsThread::run([=]
	{
		while(vstd::contains_if(source->artifactsWorn, isArtRemovable))
		{
			auto art = std::find_if(source->artifactsWorn.begin(), source->artifactsWorn.end(), isArtRemovable);

			moveArtifact(source, target, art->first);
		}

		while(!source->artifactsInBackpack.empty())
		{
			moveArtifact(source, target, source->getArtPos(source->artifactsInBackpack.begin()->artifact));
		}

		view->redraw();
	});
}

void CExchangeController::moveArtifact(
	const CGHeroInstance * source,
	const CGHeroInstance * target,
	ArtifactPosition srcPosition)
{
	auto artifact = source->getArt(srcPosition);
	auto srcLocation = ArtifactLocation(source, srcPosition);
	bool changeMade = false;

	for(auto slot : artifact->artType->possibleSlots.at(target->bearerType()))
	{
		auto existingArtifact = target->getArt(slot);
		auto existingArtInfo = target->getSlot(slot);
		ArtifactLocation destLocation(target, slot);

		if(!existingArtifact
			&& (!existingArtInfo || !existingArtInfo->locked)
			&& artifact->canBePutAt(destLocation))
		{
			cb->swapArtifacts(srcLocation, ArtifactLocation(target, slot));

			return;
		}
	}

	cb->swapArtifacts(srcLocation, ArtifactLocation(target, ArtifactPosition(GameConstants::BACKPACK_START)));
}

static const std::string HD_MOD_PREFIX = "HDMOD";
static const std::string HD_EXCHANGE_BG = HD_MOD_PREFIX + "/TRADEHD";

static bool isHdLayoutAvailable()
{
	return CResourceHandler::get()->existsResource(ResourceID(std::string("SPRITES/") + HD_EXCHANGE_BG, EResType::IMAGE));
}

CExchangeWindow::CExchangeWindow(ObjectInstanceID hero1, ObjectInstanceID hero2, QueryID queryID)
	: CStatusbarWindow(PLAYER_COLORED | BORDERED, isHdLayoutAvailable() ? HD_EXCHANGE_BG : "TRADE2"),
	controller(this, hero1, hero2)
{
	const bool hdLayout = isHdLayoutAvailable();

	OBJECT_CONSTRUCTION_CAPTURING(255 - DISPOSE);

	heroInst[0] = LOCPLINT->cb->getHero(hero1);
	heroInst[1] = LOCPLINT->cb->getHero(hero2);

	auto genTitle = [](const CGHeroInstance * h)
	{
		boost::format fmt(CGI->generaltexth->allTexts[138]);
		fmt % h->name % h->level % h->type->heroClass->name;
		return boost::str(fmt);
	};

	titles[0] = std::make_shared<CLabel>(147, 25, FONT_SMALL, CENTER, Colors::WHITE, genTitle(heroInst[0]));
	titles[1] = std::make_shared<CLabel>(653, 25, FONT_SMALL, CENTER, Colors::WHITE, genTitle(heroInst[1]));

	auto PSKIL32 = std::make_shared<CAnimation>("PSKIL32");
	PSKIL32->preload();

	auto SECSK32 = std::make_shared<CAnimation>("SECSK32");

	for(int g = 0; g < 4; ++g)
	{
		if(hdLayout)
			primSkillImages.push_back(std::make_shared<CAnimImage>(PSKIL32, g, Rect(389, 12 + 26 * g, 22, 22)));
		else
			primSkillImages.push_back(std::make_shared<CAnimImage>(PSKIL32, g, 0, 385, 19 + 36 * g));
	}

	for(int leftRight : {0, 1})
	{
		const CGHeroInstance * hero = heroInst.at(leftRight);

		herosWArt[leftRight] = std::make_shared<CHeroWithMaybePickedArtifact>(this, hero);

		for(int m = 0; m < GameConstants::PRIMARY_SKILLS; ++m)
			primSkillValues[leftRight].push_back(std::make_shared<CLabel>(352 + (hdLayout ? 96 : 93) * leftRight, (hdLayout ? 22 : 35) + (hdLayout ? 26 : 36) * m, FONT_SMALL, CENTER, Colors::WHITE));


		for(int m = 0; m < hero->secSkills.size(); ++m)
			secSkillIcons[leftRight].push_back(std::make_shared<CAnimImage>(SECSK32, 0, 0, 32 + 36 * m + 454 * leftRight, hdLayout ? 83 : 88));

		specImages[leftRight] = std::make_shared<CAnimImage>("UN32", hero->type->imageIndex, 0, 67 + 490 * leftRight, hdLayout ? 41 : 45);

		expImages[leftRight] = std::make_shared<CAnimImage>(PSKIL32, 4, 0, 103 + 490 * leftRight, hdLayout ? 41 : 45);
		expValues[leftRight] = std::make_shared<CLabel>(119 + 490 * leftRight, hdLayout ? 66 : 71, FONT_SMALL, CENTER, Colors::WHITE);

		manaImages[leftRight] = std::make_shared<CAnimImage>(PSKIL32, 5, 0, 139 + 490 * leftRight, hdLayout ? 41 : 45);
		manaValues[leftRight] = std::make_shared<CLabel>(155 + 490 * leftRight, hdLayout ? 66 : 71, FONT_SMALL, CENTER, Colors::WHITE);
	}

	portraits[0] = std::make_shared<CAnimImage>("PortraitsLarge", heroInst[0]->portrait, 0, 257, 13);
	portraits[1] = std::make_shared<CAnimImage>("PortraitsLarge", heroInst[1]->portrait, 0, 485, 13);

	artifs[0] = std::make_shared<CArtifactsOfHero>(Point(-334, 150));
	artifs[0]->commonInfo = std::make_shared<CArtifactsOfHero::SCommonPart>();
	artifs[0]->commonInfo->participants.insert(artifs[0].get());
	artifs[0]->setHero(heroInst[0]);
	artifs[1] = std::make_shared<CArtifactsOfHero>(Point(96, 150));
	artifs[1]->commonInfo = artifs[0]->commonInfo;
	artifs[1]->commonInfo->participants.insert(artifs[1].get());
	artifs[1]->setHero(heroInst[1]);

	addSet(artifs[0]);
	addSet(artifs[1]);

	for(int g = 0; g < 4; ++g)
	{
		primSkillAreas.push_back(std::make_shared<LRClickableAreaWTextComp>());
		if(hdLayout)
			primSkillAreas[g]->pos = genRect(22, 152, pos.x + 324, pos.y + 12 + 26 * g);
		else
			primSkillAreas[g]->pos = genRect(32, 140, pos.x + 329, pos.y + 19 + 36 * g);
		primSkillAreas[g]->text = CGI->generaltexth->arraytxt[2 + g];
		primSkillAreas[g]->type = g;
		primSkillAreas[g]->bonusValue = -1;
		primSkillAreas[g]->baseType = 0;
		primSkillAreas[g]->hoverText = CGI->generaltexth->heroscrn[1];
		boost::replace_first(primSkillAreas[g]->hoverText, "%s", CGI->generaltexth->primarySkillNames[g]);
	}

	//heroes related thing
	for(int b = 0; b < heroInst.size(); b++)
	{
		const CGHeroInstance * hero = heroInst.at(b);

		//secondary skill's clickable areas
		for(int g = 0; g < hero->secSkills.size(); ++g)
		{
			int skill = hero->secSkills[g].first,
				level = hero->secSkills[g].second; // <1, 3>
			secSkillAreas[b].push_back(std::make_shared<LRClickableAreaWTextComp>());
			secSkillAreas[b][g]->pos = genRect(32, 32, pos.x + 32 + g * 36 + b * 454, pos.y + hdLayout ? 83 : 88);
			secSkillAreas[b][g]->baseType = 1;

			secSkillAreas[b][g]->type = skill;
			secSkillAreas[b][g]->bonusValue = level;
			secSkillAreas[b][g]->text = CGI->skillh->skillInfo(skill, level);

			secSkillAreas[b][g]->hoverText = CGI->generaltexth->heroscrn[21];
			boost::algorithm::replace_first(secSkillAreas[b][g]->hoverText, "%s", CGI->generaltexth->levels[level - 1]);
			boost::algorithm::replace_first(secSkillAreas[b][g]->hoverText, "%s", CGI->skillh->skillName(skill));
		}

		heroAreas[b] = std::make_shared<CHeroArea>(257 + 228 * b, 13, hero);

		specialtyAreas[b] = std::make_shared<LRClickableAreaWText>();
		specialtyAreas[b]->pos = genRect(32, 32, pos.x + 69 + 490 * b, pos.y + hdLayout ? 41 : 45);
		specialtyAreas[b]->hoverText = CGI->generaltexth->heroscrn[27];
		specialtyAreas[b]->text = hero->type->specDescr;

		experienceAreas[b] = std::make_shared<LRClickableAreaWText>();
		experienceAreas[b]->pos = genRect(32, 32, pos.x + 105 + 490 * b, pos.y + hdLayout ? 41 : 45);
		experienceAreas[b]->hoverText = CGI->generaltexth->heroscrn[9];
		experienceAreas[b]->text = CGI->generaltexth->allTexts[2];
		boost::algorithm::replace_first(experienceAreas[b]->text, "%d", boost::lexical_cast<std::string>(hero->level));
		boost::algorithm::replace_first(experienceAreas[b]->text, "%d", boost::lexical_cast<std::string>(CGI->heroh->reqExp(hero->level + 1)));
		boost::algorithm::replace_first(experienceAreas[b]->text, "%d", boost::lexical_cast<std::string>(hero->exp));

		spellPointsAreas[b] = std::make_shared<LRClickableAreaWText>();
		spellPointsAreas[b]->pos = genRect(32, 32, pos.x + 141 + 490 * b, pos.y + hdLayout ? 41 : 45);
		spellPointsAreas[b]->hoverText = CGI->generaltexth->heroscrn[22];
		spellPointsAreas[b]->text = CGI->generaltexth->allTexts[205];
		boost::algorithm::replace_first(spellPointsAreas[b]->text, "%s", hero->name);
		boost::algorithm::replace_first(spellPointsAreas[b]->text, "%d", boost::lexical_cast<std::string>(hero->mana));
		boost::algorithm::replace_first(spellPointsAreas[b]->text, "%d", boost::lexical_cast<std::string>(hero->manaLimit()));

		morale[b] = std::make_shared<MoraleLuckBox>(true, genRect(32, 32, 176 + 490 * b, 39), true);
		luck[b] = std::make_shared<MoraleLuckBox>(false, genRect(32, 32, 212 + 490 * b, 39), true);
	}

	quit = std::make_shared<CButton>(Point(732, 567), "IOKAY.DEF", CGI->generaltexth->zelp[600], std::bind(&CExchangeWindow::close, this), SDLK_RETURN);
	if(queryID.getNum() > 0)
		quit->addCallback([=]() { LOCPLINT->cb->selectionMade(0, queryID); });

	questlogButton[0] = std::make_shared<CButton>(Point(10, hdLayout ? 39 : 44), "hsbtns4.def", CButton::tooltip(CGI->generaltexth->heroscrn[0]), std::bind(&CExchangeWindow::questlog, this, 0));
	questlogButton[1] = std::make_shared<CButton>(Point(740, hdLayout ? 39 : 44), "hsbtns4.def", CButton::tooltip(CGI->generaltexth->heroscrn[0]), std::bind(&CExchangeWindow::questlog, this, 1));

	Rect barRect(5, 578, 725, 18);
	statusbar = CGStatusBar::create(std::make_shared<CPicture>(*background, barRect, 5, 578, false));

	//garrison interface

	garr = std::make_shared<CGarrisonInt>(69, hdLayout ? 122 : 131, 4, Point(418, 0), heroInst[0], heroInst[1], true, true);
	auto splitButtonCallback = [&]() { garr->splitClick(); };
	garr->addSplitBtn(std::make_shared<CButton>(Point(10, hdLayout ? 122 : 132), "TSBTNS.DEF", CButton::tooltip(CGI->generaltexth->tcommands[3]), splitButtonCallback));
	garr->addSplitBtn(std::make_shared<CButton>(Point(744, hdLayout ? 122 : 132), "TSBTNS.DEF", CButton::tooltip(CGI->generaltexth->tcommands[3]), splitButtonCallback));

	if(hdLayout && (CGI->generaltexth->hdModCommands.size() >= 5))
	{
		GuiJsonLoader uiLoader;

		root = uiLoader.loadJson(pos, ResourceID("data/exchangeWindow.json"), controller);
	}

	updateWidgets();
}

CExchangeWindow::~CExchangeWindow()
{
	artifs[0]->commonInfo = nullptr;
	artifs[1]->commonInfo = nullptr;
}

void CExchangeWindow::updateGarrisons()
{
	garr->recreateSlots();

	updateWidgets();
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

		for(int m = 0; m < GameConstants::PRIMARY_SKILLS; ++m)
		{
			auto value = herosWArt[leftRight]->getPrimSkillLevel(static_cast<PrimarySkill::PrimarySkill>(m));
			primSkillValues[leftRight][m]->setText(boost::lexical_cast<std::string>(value));
		}

		for(int m = 0; m < hero->secSkills.size(); ++m)
		{
			int id = hero->secSkills[m].first;
			int level = hero->secSkills[m].second;

			secSkillIcons[leftRight][m]->setFrame(2 + id * 3 + level);
		}

		expValues[leftRight]->setText(makeNumberShort(hero->exp));
		manaValues[leftRight]->setText(makeNumberShort(hero->mana));

		morale[leftRight]->set(hero);
		luck[leftRight]->set(hero);
	}
}