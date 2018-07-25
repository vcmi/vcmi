/*
 * CHeroWindow.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "CHeroWindow.h"

#include "CAdvmapInterface.h"
#include "CCreatureWindow.h"
#include "CKingdomInterface.h"
#include "GUIClasses.h"

#include "../CBitmapHandler.h"
#include "../CGameInfo.h"
#include "../CMessage.h"
#include "../CMT.h"
#include "../CPlayerInterface.h"
#include "../Graphics.h"

#include "../gui/SDL_Extensions.h"
#include "../gui/CGuiHandler.h"
#include "../gui/CAnimation.h"
#include "../widgets/MiscWidgets.h"
#include "../widgets/CComponent.h"

#include "../../CCallback.h"

#include "../lib/CArtHandler.h"
#include "../lib/CConfigHandler.h"
#include "../lib/CGeneralTextHandler.h"
#include "../lib/CHeroHandler.h"
#include "../lib/CSkillHandler.h"
#include "../lib/mapObjects/CGHeroInstance.h"
#include "../lib/NetPacksBase.h"
#include "../mapHandler.h"


const TBonusListPtr CHeroWithMaybePickedArtifact::getAllBonuses(const CSelector & selector, const CSelector & limit, const CBonusSystemNode * root, const std::string & cachingStr) const
{
	TBonusListPtr out(new BonusList());
	TBonusListPtr heroBonuses = hero->getAllBonuses(selector, limit, hero, cachingStr);
	TBonusListPtr bonusesFromPickedUpArtifact;

	std::shared_ptr<CArtifactsOfHero::SCommonPart> cp = cww->getCommonPart();
	if(cp && cp->src.art && cp->src.valid() && cp->src.AOH && cp->src.AOH->getHero() == hero)
		bonusesFromPickedUpArtifact = cp->src.art->getAllBonuses(selector, limit, hero);
	else
		bonusesFromPickedUpArtifact = TBonusListPtr(new BonusList());

	for(auto b : *bonusesFromPickedUpArtifact)
		*heroBonuses -= b;
	for(auto b : *heroBonuses)
		out->push_back(b);
	return out;
}

int64_t CHeroWithMaybePickedArtifact::getTreeVersion() const
{
	return hero->getTreeVersion();  //this assumes that hero and artifact belongs to main bonus tree
}

CHeroWithMaybePickedArtifact::CHeroWithMaybePickedArtifact(CWindowWithArtifacts * Cww, const CGHeroInstance * Hero)
	: hero(Hero), cww(Cww)
{
}

void CHeroSwitcher::clickLeft(tribool down, bool previousState)
{
	if(!down)
	{
		//TODO: do not recreate window
		#if 0
		owner->update(hero, true);
		#else
		const CGHeroInstance * buf = hero;
		GH.popInts(1);
		GH.pushIntT<CHeroWindow>(buf);
		#endif // 0
	}
}

CHeroSwitcher::CHeroSwitcher(CHeroWindow * owner_, Point pos_, const CGHeroInstance * hero_)
	: CIntObject(LCLICK),
	owner(owner_),
	hero(hero_)
{
	OBJECT_CONSTRUCTION_CAPTURING(255-DISPOSE);
	pos += pos_;

	image = std::make_shared<CAnimImage>("PortraitsSmall", hero->portrait);
	pos.w = image->pos.w;
	pos.h = image->pos.h;
}

CHeroWindow::CHeroWindow(const CGHeroInstance * hero)
	: CWindowObject(PLAYER_COLORED, "HeroScr4"),
	heroWArt(this, hero)
{
	auto & heroscrn = CGI->generaltexth->heroscrn;

	OBJECT_CONSTRUCTION_CAPTURING(255-DISPOSE);
	curHero = hero;

	banner = std::make_shared<CAnimImage>("CREST58", LOCPLINT->playerID.getNum(), 0, 606, 8);
	name = std::make_shared<CLabel>(190, 38, EFonts::FONT_BIG, EAlignment::CENTER, Colors::YELLOW);
	title = std::make_shared<CLabel>(190, 65, EFonts::FONT_MEDIUM, EAlignment::CENTER, Colors::WHITE);

	statusBar = std::make_shared<CGStatusBar>(7, 559, "ADROLLVR.bmp", 660);

	quitButton = std::make_shared<CButton>(Point(609, 516), "hsbtns.def", CButton::tooltip(heroscrn[17]), [=](){ close(); }, SDLK_RETURN);
	quitButton->assignedKeys.insert(SDLK_ESCAPE);

	dismissLabel = std::make_shared<CTextBox>(CGI->generaltexth->jktexts[8], Rect(370, 430, 65, 35), 0, FONT_SMALL, TOPLEFT, Colors::WHITE);
	dismissButton = std::make_shared<CButton>(Point(454, 429), "hsbtns2.def", CButton::tooltip(heroscrn[28]), [=](){ dismissCurrent(); }, SDLK_d);

	questlogLabel = std::make_shared<CTextBox>(CGI->generaltexth->jktexts[9], Rect(510, 430, 65, 35), 0, FONT_SMALL, TOPLEFT, Colors::WHITE);
	questlogButton = std::make_shared<CButton>(Point(314, 429), "hsbtns4.def", CButton::tooltip(heroscrn[0]), [=](){ LOCPLINT->showQuestLog(); }, SDLK_q);

	formations = std::make_shared<CToggleGroup>(0);
	formations->addToggle(0, std::make_shared<CToggleButton>(Point(481, 483), "hsbtns6.def", std::make_pair(heroscrn[23], heroscrn[29]), 0, SDLK_t));
	formations->addToggle(1, std::make_shared<CToggleButton>(Point(481, 519), "hsbtns7.def", std::make_pair(heroscrn[24], heroscrn[30]), 0, SDLK_l));

	if(hero->commander)
	{
		auto texts = CGI->generaltexth->localizedTexts["heroWindow"]["openCommander"];
		commanderButton = std::make_shared<CButton>(Point(317, 18), "buttons/commander", CButton::tooltip(texts), [&](){ commanderWindow(); }, SDLK_c);
	}

	//right list of heroes
	for(int i=0; i < std::min(LOCPLINT->cb->howManyHeroes(false), 8); i++)
		heroList.push_back(std::make_shared<CHeroSwitcher>(this, Point(612, 87 + i * 54), LOCPLINT->cb->getHeroBySerial(i, false)));

	//areas
	portraitArea = std::make_shared<LRClickableAreaWText>(Rect(18, 18, 58, 64));
	portraitImage = std::make_shared<CAnimImage>("PortraitsLarge", 0, 0, 19, 19);

	for(int v = 0; v < GameConstants::PRIMARY_SKILLS; ++v)
	{
		auto area = std::make_shared<LRClickableAreaWTextComp>(Rect(30 + 70 * v, 109, 42, 64), CComponent::primskill);
		area->text = CGI->generaltexth->arraytxt[2+v];
		area->type = v;
		area->hoverText = boost::str(boost::format(CGI->generaltexth->heroscrn[1]) % CGI->generaltexth->primarySkillNames[v]);
		primSkillAreas.push_back(area);

		auto value = std::make_shared<CLabel>(53 + 70 * v, 166, FONT_SMALL, CENTER);
		primSkillValues.push_back(value);
	}

	auto primSkills = std::make_shared<CAnimation>("PSKIL42");
	primSkills->preload();
	primSkillImages.push_back(std::make_shared<CAnimImage>(primSkills, 0, 0, 32, 111));
	primSkillImages.push_back(std::make_shared<CAnimImage>(primSkills, 1, 0, 102, 111));
	primSkillImages.push_back(std::make_shared<CAnimImage>(primSkills, 2, 0, 172, 111));
	primSkillImages.push_back(std::make_shared<CAnimImage>(primSkills, 3, 0, 162, 230));
	primSkillImages.push_back(std::make_shared<CAnimImage>(primSkills, 4, 0, 20, 230));
	primSkillImages.push_back(std::make_shared<CAnimImage>(primSkills, 5, 0, 242, 111));

	specImage = std::make_shared<CAnimImage>("UN44", 0, 0, 18, 180);
	specArea = std::make_shared<LRClickableAreaWText>(Rect(18, 180, 136, 42), CGI->generaltexth->heroscrn[27]);
	specName = std::make_shared<CLabel>(69, 205);

	expArea = std::make_shared<LRClickableAreaWText>(Rect(18, 228, 136, 42), CGI->generaltexth->heroscrn[9]);
	morale = std::make_shared<MoraleLuckBox>(true, Rect(175, 179, 53, 45));
	luck = std::make_shared<MoraleLuckBox>(false, Rect(233, 179, 53, 45));
	spellPointsArea = std::make_shared<LRClickableAreaWText>(Rect(162,228, 136, 42), CGI->generaltexth->heroscrn[22]);

	expValue = std::make_shared<CLabel>(68, 252);
	manaValue = std::make_shared<CLabel>(211, 252);

	auto secSkills = std::make_shared<CAnimation>("SECSKILL");
	for(int i = 0; i < std::min<size_t>(hero->secSkills.size(), 8u); ++i)
	{
		Rect r = Rect(i%2 == 0  ?  18  :  162,  276 + 48 * (i/2),  136,  42);
		secSkillAreas.push_back(std::make_shared<LRClickableAreaWTextComp>(r, CComponent::secskill));
		secSkillImages.push_back(std::make_shared<CAnimImage>(secSkills, 0, 0, r.x, r.y));

		int x = (i % 2) ? 212 : 68;
		int y = 280 + 48 * (i/2);

		secSkillValues.push_back(std::make_shared<CLabel>(x, y, FONT_SMALL, TOPLEFT));
		secSkillNames.push_back(std::make_shared<CLabel>(x, y+20, FONT_SMALL, TOPLEFT));
	}

	// various texts
	labels.push_back(std::make_shared<CLabel>(52, 99, FONT_SMALL, CENTER, Colors::YELLOW, CGI->generaltexth->jktexts[1]));
	labels.push_back(std::make_shared<CLabel>(123, 99, FONT_SMALL, CENTER, Colors::YELLOW, CGI->generaltexth->jktexts[2]));
	labels.push_back(std::make_shared<CLabel>(193, 99, FONT_SMALL, CENTER, Colors::YELLOW, CGI->generaltexth->jktexts[3]));
	labels.push_back(std::make_shared<CLabel>(262, 99, FONT_SMALL, CENTER, Colors::YELLOW, CGI->generaltexth->jktexts[4]));

	labels.push_back(std::make_shared<CLabel>(69, 183, FONT_SMALL, TOPLEFT, Colors::YELLOW, CGI->generaltexth->jktexts[5]));
	labels.push_back(std::make_shared<CLabel>(69, 232, FONT_SMALL, TOPLEFT, Colors::YELLOW, CGI->generaltexth->jktexts[6]));
	labels.push_back(std::make_shared<CLabel>(213, 232, FONT_SMALL, TOPLEFT, Colors::YELLOW, CGI->generaltexth->jktexts[7]));

	update(hero);
}

void CHeroWindow::update(const CGHeroInstance * hero, bool redrawNeeded)
{
	auto & heroscrn = CGI->generaltexth->heroscrn;

	if(!hero) //something strange... no hero? it shouldn't happen
	{
		logGlobal->error("Set nullptr hero? no way...");
		return;
	}

	assert(hero == curHero);

	name->setText(curHero->name);
	title->setText((boost::format(CGI->generaltexth->allTexts[342]) % curHero->level % curHero->type->heroClass->name).str());

	specArea->text = curHero->type->specDescr;
	specImage->setFrame(curHero->type->imageIndex);
	specName->setText(curHero->type->specName);

	tacticsButton = std::make_shared<CToggleButton>(Point(539, 483), "hsbtns8.def", std::make_pair(heroscrn[26], heroscrn[31]), 0, SDLK_b);
	tacticsButton->addHoverText(CButton::HIGHLIGHTED, CGI->generaltexth->heroscrn[25]);

	dismissButton->addHoverText(CButton::NORMAL, boost::str(boost::format(CGI->generaltexth->heroscrn[16]) % curHero->name % curHero->type->heroClass->name));
	portraitArea->hoverText = boost::str(boost::format(CGI->generaltexth->allTexts[15]) % curHero->name % curHero->type->heroClass->name);
	portraitArea->text = curHero->getBiography();
	portraitImage->setFrame(curHero->portrait);

	{
		OBJECT_CONSTRUCTION_CUSTOM_CAPTURING(255-DISPOSE);
		if(!garr)
		{
			std::string helpBox = heroscrn[32];
			boost::algorithm::replace_first(helpBox, "%s", CGI->generaltexth->allTexts[43]);

			garr = std::make_shared<CGarrisonInt>(15, 485, 8, Point(), curHero);
			auto split = std::make_shared<CButton>(Point(539, 519), "hsbtns9.def", CButton::tooltip(CGI->generaltexth->allTexts[256], helpBox), [&](){ garr->splitClick(); });
			garr->addSplitBtn(split);
		}
		if(!arts)
		{
			arts = std::make_shared<CArtifactsOfHero>(Point(-65, -8), true);
			arts->setHero(curHero);
			addSet(arts);
		}

		int serial = LOCPLINT->cb->getHeroSerial(curHero, false);

		listSelection.reset();
		if(serial >= 0)
			listSelection = std::make_shared<CPicture>("HPSYYY", 612, 33 + serial * 54);
	}

	//primary skills support
	for(size_t g=0; g<primSkillAreas.size(); ++g)
	{
		primSkillAreas[g]->bonusValue = heroWArt.getPrimSkillLevel(static_cast<PrimarySkill::PrimarySkill>(g));
		primSkillValues[g]->setText(boost::lexical_cast<std::string>(primSkillAreas[g]->bonusValue));
	}

	//secondary skills support
	for(size_t g=0; g< secSkillAreas.size(); ++g)
	{
		int skill = curHero->secSkills[g].first;
		int	level = curHero->getSecSkillLevel(SecondarySkill(curHero->secSkills[g].first));
		std::string skillName = CGI->skillh->skillName(skill);
		std::string skillValue = CGI->generaltexth->levels[level-1];

		secSkillAreas[g]->type = skill;
		secSkillAreas[g]->bonusValue = level;
		secSkillAreas[g]->text = CGI->skillh->skillInfo(skill, level);
		secSkillAreas[g]->hoverText = boost::str(boost::format(heroscrn[21]) % skillValue % skillName);
		secSkillImages[g]->setFrame(skill*3 + level + 2);
		secSkillNames[g]->setText(skillName);
		secSkillValues[g]->setText(skillValue);
	}

	std::ostringstream expstr;
	expstr << curHero->exp;
	expValue->setText(expstr.str());

	std::ostringstream manastr;
	manastr << curHero->mana << '/' << heroWArt.manaLimit();
	manaValue->setText(manastr.str());

	//printing experience - original format does not support ui64
	expArea->text = CGI->generaltexth->allTexts[2];
	boost::replace_first(expArea->text, "%d", boost::lexical_cast<std::string>(curHero->level));
	boost::replace_first(expArea->text, "%d", boost::lexical_cast<std::string>(CGI->heroh->reqExp(curHero->level+1)));
	boost::replace_first(expArea->text, "%d", boost::lexical_cast<std::string>(curHero->exp));

	//printing spell points, boost::format can't be used due to locale issues
	spellPointsArea->text = CGI->generaltexth->allTexts[205];
	boost::replace_first(spellPointsArea->text, "%s", boost::lexical_cast<std::string>(curHero->name));
	boost::replace_first(spellPointsArea->text, "%d", boost::lexical_cast<std::string>(curHero->mana));
	boost::replace_first(spellPointsArea->text, "%d", boost::lexical_cast<std::string>(heroWArt.manaLimit()));

	//if we have exchange window with this curHero open
	bool noDismiss=false;
	for(auto isa : GH.listInt)
	{
		if(CExchangeWindow * cew = dynamic_cast<CExchangeWindow*>(isa.get()))
		{
			for(int g=0; g < cew->heroInst.size(); ++g)
				if(cew->heroInst[g] == curHero)
					noDismiss = true;
		}

		if(dynamic_cast<CKingdomInterface*>(isa.get()))
			noDismiss = true;
	}
	//if player only have one hero and no towns
	if(!LOCPLINT->cb->howManyTowns() && LOCPLINT->cb->howManyHeroes() == 1)
		noDismiss = true;

	if(curHero->isMissionCritical())
		noDismiss = true;

	dismissButton->block(!!curHero->visitedTown || noDismiss);

	if(curHero->getSecSkillLevel(SecondarySkill::TACTICS) == 0)
	{
		tacticsButton->block(true);
	}
	else
	{
		tacticsButton->block(false);
		tacticsButton->addCallback([&](bool on){curHero->tacticFormationEnabled = on;});
	}

	formations->resetCallback();
	//setting formations
	formations->setSelected(curHero->formation);
	formations->addCallback([=](int value){ LOCPLINT->cb->setFormation(curHero, value);});

	morale->set(&heroWArt);
	luck->set(&heroWArt);

	if(redrawNeeded)
		redraw();
}

void CHeroWindow::dismissCurrent()
{
	CFunctionList<void()> ony = [=](){ close(); };
	ony += [=](){ LOCPLINT->cb->dismissHero(curHero); };
	LOCPLINT->showYesNoDialog(CGI->generaltexth->allTexts[22], ony, nullptr);
}

void CHeroWindow::commanderWindow()
{
	//bool artSelected = false;
	const std::shared_ptr<CArtifactsOfHero::SCommonPart> commonInfo = getCommonPart();

	if(const CArtifactInstance *art = commonInfo->src.art)
	{
		const CGHeroInstance *srcHero = commonInfo->src.AOH->getHero();
		//artSelected = true;
		ArtifactPosition freeSlot = art->firstAvailableSlot (curHero->commander);
		if(freeSlot < ArtifactPosition::COMMANDER_AFTER_LAST) //we don't want to put it in commander's backpack!
		{
			ArtifactLocation src(srcHero, commonInfo->src.slotID);
			ArtifactLocation dst(curHero->commander.get(), freeSlot);

			if(art->canBePutAt(dst, true))
			{	//equip clicked stack
				if(dst.getArt())
				{
					LOCPLINT->cb->swapArtifacts (dst, ArtifactLocation(srcHero, dst.getArt()->firstBackpackSlot(srcHero)));
				}
				LOCPLINT->cb->swapArtifacts(src, dst);
			}
		}
	}
	else
	{
		GH.pushIntT<CStackWindow>(curHero->commander, false);
	}
}

void CHeroWindow::updateGarrisons()
{
	garr->recreateSlots();
	morale->set(&heroWArt);
}
