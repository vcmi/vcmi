/*
 * CBonusSelection.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "../mainmenu/CMainMenu.h"
#include "../mainmenu/CPrologEpilogVideo.h"
#include "CBonusSelection.h"
#include "CSelectionBase.h"

#include "../CGameInfo.h"
#include "../CMessage.h"
#include "../CBitmapHandler.h"
#include "../CMusicHandler.h"
#include "../CVideoHandler.h"
#include "../CPlayerInterface.h"
#include "../CServerHandler.h"
#include "../gui/CAnimation.h"
#include "../gui/CGuiHandler.h"
#include "../widgets/CComponent.h"
#include "../widgets/Buttons.h"
#include "../widgets/MiscWidgets.h"
#include "../widgets/ObjectLists.h"
#include "../widgets/TextControls.h"
#include "../windows/GUIClasses.h"
#include "../windows/InfoWindows.h"

#include "../../lib/filesystem/Filesystem.h"
#include "../../lib/CGeneralTextHandler.h"

#include "../../lib/CArtHandler.h"
#include "../../lib/CBuildingHandler.h"
#include "../../lib/spells/CSpellHandler.h"

#include "../../lib/CSkillHandler.h"
#include "../../lib/CTownHandler.h"
#include "../../lib/CHeroHandler.h"
#include "../../lib/CCreatureHandler.h"
#include "../../lib/StartInfo.h"

#include "../../lib/mapping/CCampaignHandler.h"
#include "../../lib/mapping/CMapService.h"
#include "../../lib/mapping/CMapInfo.h"

#include "../../lib/mapObjects/CGHeroInstance.h"

std::shared_ptr<CCampaignState> CBonusSelection::getCampaign()
{
	return CSH->si->campState;
}

CBonusSelection::CBonusSelection()
{
	highlightedRegion = nullptr;
	buttonDifficultyLeft = nullptr;
	buttonDifficultyRight = nullptr;
	bonuses = nullptr;

	OBJ_CONSTRUCTION_CAPTURING_ALL;
	static const std::string bgNames[] =
	{
		"E1_BG.BMP", "G2_BG.BMP", "E2_BG.BMP", "G1_BG.BMP", "G3_BG.BMP", "N1_BG.BMP",
		"S1_BG.BMP", "BR_BG.BMP", "IS_BG.BMP", "KR_BG.BMP", "NI_BG.BMP", "TA_BG.BMP", "AR_BG.BMP", "HS_BG.BMP",
		"BB_BG.BMP", "NB_BG.BMP", "EL_BG.BMP", "RN_BG.BMP", "UA_BG.BMP", "SP_BG.BMP"
	};

	loadPositionsOfGraphics();

	background = BitmapHandler::loadBitmap(bgNames[getCampaign()->camp->header.mapVersion]);
	pos.h = background->h;
	pos.w = background->w;
	center();

	SDL_Surface * panel = BitmapHandler::loadBitmap("CAMPBRF.BMP");

	blitAt(panel, 456, 6, background);

	buttonStart = new CButton(Point(475, 536), "CBBEGIB.DEF", CButton::tooltip(), std::bind(&CBonusSelection::startMap, this), SDLK_RETURN);
	buttonRestart = new CButton(Point(475, 536), "CBRESTB.DEF", CButton::tooltip(), std::bind(&CBonusSelection::restartMap, this), SDLK_RETURN);
	buttonBack = new CButton(Point(624, 536), "CBCANCB.DEF", CButton::tooltip(), std::bind(&CBonusSelection::goBack, this), SDLK_ESCAPE);

	//campaign name
	if(getCampaign()->camp->header.name.length())
		graphics->fonts[FONT_BIG]->renderTextLeft(background, getCampaign()->camp->header.name, Colors::YELLOW, Point(481, 28));
	else
		graphics->fonts[FONT_BIG]->renderTextLeft(background, CGI->generaltexth->allTexts[508], Colors::YELLOW, Point(481, 28));

	//map size icon
	mapSizeIcons = new CAnimImage("SCNRMPSZ", 4, 0, 735, 26);
	mapSizeIcons->recActions &= ~(SHOWALL | UPDATE); //explicit draw

	//campaign description
	graphics->fonts[FONT_SMALL]->renderTextLeft(background, CGI->generaltexth->allTexts[38], Colors::YELLOW, Point(481, 63));

	campaignDescription = new CTextBox(getCampaign()->camp->header.description, Rect(480, 86, 286, 117), 1);
	//campaignDescription->showAll(background);

	//map description
	mapDescription = new CTextBox("", Rect(480, 280, 286, 117), 1);

	//bonus choosing
	graphics->fonts[FONT_MEDIUM]->renderTextLeft(background, CGI->generaltexth->allTexts[71], Colors::WHITE, Point(511, 432));
	bonuses = new CToggleGroup(std::bind(&CBonusSelection::selectBonus, this, _1));

	//set left part of window
	bool isCurrentMapConquerable = getCampaign()->currentMap && getCampaign()->camp->conquerable(*getCampaign()->currentMap);
	for(int g = 0; g < getCampaign()->camp->scenarios.size(); ++g)
	{
		if(getCampaign()->camp->conquerable(g))
		{
			regions.push_back(new CRegion(this, true, true, g));
			regions[regions.size() - 1]->rclickText = getCampaign()->camp->scenarios[g].regionText;
			if(highlightedRegion == nullptr)
			{
				if(!isCurrentMapConquerable || (isCurrentMapConquerable && g == *getCampaign()->currentMap))
				{
					highlightedRegion = regions.back();
					selectMap(g);
				}
			}
		}
		else if(getCampaign()->camp->scenarios[g].conquered) //display as striped
		{
			regions.push_back(new CRegion(this, false, false, g));
			regions[regions.size() - 1]->rclickText = getCampaign()->camp->scenarios[g].regionText;
		}
	}

	//allies / enemies
	graphics->fonts[FONT_SMALL]->renderTextLeft(background, CGI->generaltexth->allTexts[390] + ":", Colors::WHITE, Point(486, 407));
	graphics->fonts[FONT_SMALL]->renderTextLeft(background, CGI->generaltexth->allTexts[391] + ":", Colors::WHITE, Point(619, 407));

	SDL_FreeSurface(panel);

	//difficulty
	std::vector<std::string> difficulty;
	boost::split(difficulty, CGI->generaltexth->allTexts[492], boost::is_any_of(" "));
	graphics->fonts[FONT_MEDIUM]->renderTextLeft(background, difficulty.back(), Colors::WHITE, Point(689, 432));

	//difficulty pics
	for(size_t b = 0; b < difficultyIcons.size(); ++b)
	{
		difficultyIcons[b] = new CAnimImage("GSPBUT" + boost::lexical_cast<std::string>(b + 3) + ".DEF", 0, 0, 709, 455);
		difficultyIcons[b]->recActions &= ~(SHOWALL | UPDATE); //explicit draw
	}

	//difficulty selection buttons
	if(getCampaign()->camp->header.difficultyChoosenByPlayer)
	{
		buttonDifficultyLeft = new CButton(Point(694, 508), "SCNRBLF.DEF", CButton::tooltip(), std::bind(&CBonusSelection::decreaseDifficulty, this));
		buttonDifficultyRight = new CButton(Point(738, 508), "SCNRBRT.DEF", CButton::tooltip(), std::bind(&CBonusSelection::increaseDifficulty, this));
	}

	//load miniflags
	sFlags = std::make_shared<CAnimation>("ITGFLAGS.DEF");
	sFlags->load();

	updateAfterStateChange();
}

CBonusSelection::~CBonusSelection()
{
	SDL_FreeSurface(background);
	sFlags->unload();
}

void CBonusSelection::showAll(SDL_Surface * to)
{
	blitAt(background, pos.x, pos.y, to);
	CIntObject::showAll(to);

	show(to);
	if(pos.h != to->h || pos.w != to->w)
		CMessage::drawBorder(PlayerColor(1), to, pos.w + 28, pos.h + 30, pos.x - 14, pos.y - 15);
}

void CBonusSelection::show(SDL_Surface * to)
{
	printAtLoc(CSH->mi->getName(), 481, 219, FONT_BIG, Colors::YELLOW, to);

	//map description
	printAtLoc(CGI->generaltexth->allTexts[496], 481, 253, FONT_SMALL, Colors::YELLOW, to);

	mapDescription->showAll(to); //showAll because CTextBox has no show()

	//map size icon
	switch(CSH->mi->mapHeader->width)
	{
	case 36:
		mapSizeIcons->setFrame(0);
		break;
	case 72:
		mapSizeIcons->setFrame(1);
		break;
	case 108:
		mapSizeIcons->setFrame(2);
		break;
	case 144:
		mapSizeIcons->setFrame(3);
		break;
	default:
		mapSizeIcons->setFrame(4);
		break;
	}
	mapSizeIcons->showAll(to);

	//MPTODO: reuse lobby flags code
	int fx = 496 + graphics->fonts[FONT_SMALL]->getStringWidth(CGI->generaltexth->allTexts[390]);
	int ex = 629 + graphics->fonts[FONT_SMALL]->getStringWidth(CGI->generaltexth->allTexts[391]);
	TeamID myT;
	if(CSH->myFirstColor() < PlayerColor::PLAYER_LIMIT)
		myT = CSH->getPlayerInfo(CSH->myFirstColor().getNum()).team;
	else
		myT = TeamID::NO_TEAM;

	for(auto i = CSH->si->playerInfos.cbegin(); i != CSH->si->playerInfos.cend(); i++)
	{
		int * myx = ((i->first == CSH->myFirstColor() || CSH->getPlayerInfo(i->first.getNum()).team == myT) ? &fx : &ex);
		IImage * flag = sFlags->getImage(i->first.getNum(), 0);
		flag->draw(to, pos.x + *myx, pos.y + 405);
		*myx += flag->width();
		flag->decreaseRef();
	}

	//difficulty
	difficultyIcons[CSH->si->difficulty]->showAll(to);

	CIntObject::show(to);
}

void CBonusSelection::loadPositionsOfGraphics()
{
	const JsonNode config(ResourceID("config/campaign_regions.json"));
	int idx = 0;

	for(const JsonNode & campaign : config["campaign_regions"].Vector())
	{
		SCampPositions sc;

		sc.campPrefix = campaign["prefix"].String();
		sc.colorSuffixLength = campaign["color_suffix_length"].Float();

		for(const JsonNode & desc : campaign["desc"].Vector())
		{
			SCampPositions::SRegionDesc rd;

			rd.infix = desc["infix"].String();
			rd.xpos = desc["x"].Float();
			rd.ypos = desc["y"].Float();
			sc.regions.push_back(rd);
		}

		campDescriptions.push_back(sc);

		idx++;
	}

	assert(idx == CGI->generaltexth->campaignMapNames.size());
}

void CBonusSelection::updateStartButtonState(int selected)
{
	if(selected == -1)
	{
		buttonStart->block(getCampaign()->camp->scenarios[CSH->campaignMap].travelOptions.bonusesToChoose.size());
	}
	else if(buttonStart->isBlocked())
	{
		buttonStart->block(false);
	}
}

void CBonusSelection::updateBonusSelection()
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;
	//graphics:
	//spell - SPELLBON.DEF
	//monster - TWCRPORT.DEF
	//building - BO*.BMP graphics
	//artifact - ARTIFBON.DEF
	//spell scroll - SPELLBON.DEF
	//prim skill - PSKILBON.DEF
	//sec skill - SSKILBON.DEF
	//resource - BORES.DEF
	//player - CREST58.DEF
	//hero - PORTRAITSLARGE (HPL###.BMPs)
	const CCampaignScenario & scenario = getCampaign()->camp->scenarios[CSH->campaignMap];
	const std::vector<CScenarioTravel::STravelBonus> & bonDescs = scenario.travelOptions.bonusesToChoose;

	updateStartButtonState(-1);

	delete bonuses;
	bonuses = new CToggleGroup(std::bind(&CBonusSelection::selectBonus, this, _1));

	static const char * bonusPics[] =
	{
		"SPELLBON.DEF", "TWCRPORT.DEF", "", "ARTIFBON.DEF", "SPELLBON.DEF",
		"PSKILBON.DEF", "SSKILBON.DEF", "BORES.DEF", "PORTRAITSLARGE", "PORTRAITSLARGE"
	};

	for(int i = 0; i < bonDescs.size(); i++)
	{
		std::string picName = bonusPics[bonDescs[i].type];
		size_t picNumber = bonDescs[i].info2;

		std::string desc;
		switch(bonDescs[i].type)
		{
		case CScenarioTravel::STravelBonus::SPELL:
			desc = CGI->generaltexth->allTexts[715];
			boost::algorithm::replace_first(desc, "%s", CGI->spellh->objects[bonDescs[i].info2]->name);
			break;
		case CScenarioTravel::STravelBonus::MONSTER:
			picNumber = bonDescs[i].info2 + 2;
			desc = CGI->generaltexth->allTexts[717];
			boost::algorithm::replace_first(desc, "%d", boost::lexical_cast<std::string>(bonDescs[i].info3));
			boost::algorithm::replace_first(desc, "%s", CGI->creh->creatures[bonDescs[i].info2]->namePl);
			break;
		case CScenarioTravel::STravelBonus::BUILDING:
		{
			int faction = -1;
			for(auto & elem : CSH->si->playerInfos)
			{
				if(elem.second.isControlledByHuman())
				{
					faction = elem.second.castle;
					break;
				}

			}
			assert(faction != -1);

			BuildingID buildID = CBuildingHandler::campToERMU(bonDescs[i].info1, faction, std::set<BuildingID>());
			picName = graphics->ERMUtoPicture[faction][buildID];
			picNumber = -1;

			if(vstd::contains(CGI->townh->factions[faction]->town->buildings, buildID))
				desc = CGI->townh->factions[faction]->town->buildings.find(buildID)->second->Name();

			break;
		}
		case CScenarioTravel::STravelBonus::ARTIFACT:
			desc = CGI->generaltexth->allTexts[715];
			boost::algorithm::replace_first(desc, "%s", CGI->arth->artifacts[bonDescs[i].info2]->Name());
			break;
		case CScenarioTravel::STravelBonus::SPELL_SCROLL:
			desc = CGI->generaltexth->allTexts[716];
			boost::algorithm::replace_first(desc, "%s", CGI->spellh->objects[bonDescs[i].info2]->name);
			break;
		case CScenarioTravel::STravelBonus::PRIMARY_SKILL:
		{
			int leadingSkill = -1;
			std::vector<std::pair<int, int>> toPrint; //primary skills to be listed <num, val>
			const ui8 * ptr = reinterpret_cast<const ui8 *>(&bonDescs[i].info2);
			for(int g = 0; g < GameConstants::PRIMARY_SKILLS; ++g)
			{
				if(leadingSkill == -1 || ptr[g] > ptr[leadingSkill])
				{
					leadingSkill = g;
				}
				if(ptr[g] != 0)
				{
					toPrint.push_back(std::make_pair(g, ptr[g]));
				}
			}
			picNumber = leadingSkill;
			desc = CGI->generaltexth->allTexts[715];

			std::string substitute; //text to be printed instead of %s
			for(int v = 0; v < toPrint.size(); ++v)
			{
				substitute += boost::lexical_cast<std::string>(toPrint[v].second);
				substitute += " " + CGI->generaltexth->primarySkillNames[toPrint[v].first];
				if(v != toPrint.size() - 1)
				{
					substitute += ", ";
				}
			}

			boost::algorithm::replace_first(desc, "%s", substitute);
			break;
		}
		case CScenarioTravel::STravelBonus::SECONDARY_SKILL:
			desc = CGI->generaltexth->allTexts[718];

			boost::algorithm::replace_first(desc, "%s", CGI->generaltexth->levels[bonDescs[i].info3 - 1]); //skill level
			boost::algorithm::replace_first(desc, "%s", CGI->skillh->skillName(bonDescs[i].info2)); //skill name
			picNumber = bonDescs[i].info2 * 3 + bonDescs[i].info3 - 1;

			break;
		case CScenarioTravel::STravelBonus::RESOURCE:
		{
			int serialResID = 0;
			switch(bonDescs[i].info1)
			{
			case 0:
			case 1:
			case 2:
			case 3:
			case 4:
			case 5:
			case 6:
				serialResID = bonDescs[i].info1;
				break;
			case 0xFD: //wood + ore
				serialResID = 7;
				break;
			case 0xFE: //rare resources
				serialResID = 8;
				break;
			}
			picNumber = serialResID;

			desc = CGI->generaltexth->allTexts[717];
			boost::algorithm::replace_first(desc, "%d", boost::lexical_cast<std::string>(bonDescs[i].info2));
			std::string replacement;
			if(serialResID <= 6)
			{
				replacement = CGI->generaltexth->restypes[serialResID];
			}
			else
			{
				replacement = CGI->generaltexth->allTexts[714 + serialResID];
			}
			boost::algorithm::replace_first(desc, "%s", replacement);
			break;
		}
		case CScenarioTravel::STravelBonus::HEROES_FROM_PREVIOUS_SCENARIO:
		{
			auto superhero = getCampaign()->camp->scenarios[bonDescs[i].info2].strongestHero(PlayerColor(bonDescs[i].info1));
			if(!superhero)
				logGlobal->warn("No superhero! How could it be transferred?");
			picNumber = superhero ? superhero->portrait : 0;
			desc = CGI->generaltexth->allTexts[719];

			boost::algorithm::replace_first(desc, "%s", getCampaign()->camp->scenarios[bonDescs[i].info2].scenarioName); //scenario
			break;
		}

		case CScenarioTravel::STravelBonus::HERO:

			desc = CGI->generaltexth->allTexts[718];
			boost::algorithm::replace_first(desc, "%s", CGI->generaltexth->capColors[bonDescs[i].info1]); //hero's color

			if(bonDescs[i].info2 == 0xFFFF)
			{
				boost::algorithm::replace_first(desc, "%s", CGI->generaltexth->allTexts[101]); //hero's name
				picNumber = -1;
				picName = "CBONN1A3.BMP";
			}
			else
			{
				boost::algorithm::replace_first(desc, "%s", CGI->heroh->heroes[bonDescs[i].info2]->name); //hero's name
			}
			break;
		}

		CToggleButton * bonusButton = new CToggleButton(Point(475 + i * 68, 455), "", CButton::tooltip(desc, desc));

		if(picNumber != -1)
			picName += ":" + boost::lexical_cast<std::string>(picNumber);

		auto anim = std::make_shared<CAnimation>();
		anim->setCustom(picName, 0);
		bonusButton->setImage(anim);
		const SDL_Color brightYellow = { 242, 226, 110, 0 };
		bonusButton->borderColor = boost::make_optional(brightYellow);
		bonuses->addToggle(i, bonusButton);
	}

	// set bonus if already chosen
	if(vstd::contains(getCampaign()->chosenCampaignBonuses, CSH->campaignMap))
	{
		bonuses->setSelected(getCampaign()->chosenCampaignBonuses[CSH->campaignMap]);
	}
}

void CBonusSelection::updateAfterStateChange()
{
	// initialize restart / start button
//	if(!getCampaign()->currentMap || *getCampaign()->currentMap != CSH->campaignMap)
	if(CSH->state != EClientState::GAMEPLAY)
	{
		// draw start button
		buttonRestart->disable();
		buttonStart->enable();
		if(!getCampaign()->mapsConquered.empty())
			buttonBack->block(true);
		else
			buttonBack->block(false);
	}
	else
	{
		// draw restart button
		buttonStart->disable();
		buttonRestart->enable();
		buttonBack->block(false);
	}

	mapDescription->setText(CSH->mi->getDescription());
	updateBonusSelection();

	updateStartButtonState(CSH->campaignBonus);
}

void CBonusSelection::goBack()
{
	if(CSH->state != EClientState::GAMEPLAY)
	{
		GH.popInts(2);
	}
	else
	{
		GH.popIntTotally(this);
	}
	// TODO: we can actually only pop bonus selection interface for custom campaigns
	// Though this would require clearing CLobbyScreen::bonusSel pointer when poping this interface
/*
	else
	{
		GH.popIntTotally(this);
		CSH->state = EClientState::LOBBY;
	}
*/
}

void CBonusSelection::startMap()
{
	auto showPrologVideo = [=]()
	{
		auto exitCb = [=]()
		{
			logGlobal->info("Starting scenario %d", CSH->campaignMap);
			CSH->sendStartGame();
		};

		const CCampaignScenario & scenario = getCampaign()->camp->scenarios[CSH->campaignMap];
		if(scenario.prolog.hasPrologEpilog)
		{
			GH.pushInt(new CPrologEpilogVideo(scenario.prolog, exitCb));
		}
		else
		{
			exitCb();
		}
	};

	if(LOCPLINT) // we're currently ingame, so ask for starting new map and end game
	{
		GH.popInt(this);
		LOCPLINT->showYesNoDialog(CGI->generaltexth->allTexts[67], [=]()
		{
			GH.curInt = CMainMenu::create();
			showPrologVideo();
		}, 0);
	}
	else
	{
		showPrologVideo();
	}
}

void CBonusSelection::restartMap()
{
	GH.popInt(this);
	LOCPLINT->showYesNoDialog(CGI->generaltexth->allTexts[67], [=]()
	{
		CSH->startCampaignScenario();
	}, 0);
}

void CBonusSelection::selectMap(int mapNr)
{
	CSH->setCampaignMap(mapNr);
}

void CBonusSelection::selectBonus(int id)
{
	CSH->setCampaignBonus(id);
}

void CBonusSelection::increaseDifficulty()
{
	CSH->setDifficulty(CSH->si->difficulty + 1);
}

void CBonusSelection::decreaseDifficulty()
{
	CSH->setDifficulty(CSH->si->difficulty - 1);
}

CBonusSelection::CRegion::CRegion(CBonusSelection * _owner, bool _accessible, bool _selectable, int _myNumber)
	: owner(_owner), accessible(_accessible), selectable(_selectable), myNumber(_myNumber)
{
	OBJ_CONSTRUCTION;
	addUsedEvents(LCLICK | RCLICK);

	static const std::string colors[2][8] =
	{
		{"R", "B", "N", "G", "O", "V", "T", "P"},
		{"Re", "Bl", "Br", "Gr", "Or", "Vi", "Te", "Pi"}
	};

	const SCampPositions & campDsc = owner->campDescriptions[owner->getCampaign()->camp->header.mapVersion];
	const SCampPositions::SRegionDesc & desc = campDsc.regions[myNumber];
	pos.x += desc.xpos;
	pos.y += desc.ypos;

	//loading of graphics

	std::string prefix = campDsc.campPrefix + desc.infix + "_";
	std::string suffix = colors[campDsc.colorSuffixLength - 1][owner->getCampaign()->camp->scenarios[myNumber].regionColor];

	static const std::string infix[] = {"En", "Se", "Co"};
	for(int g = 0; g < ARRAY_COUNT(infix); g++)
	{
		graphics[g] = BitmapHandler::loadBitmap(prefix + infix[g] + suffix + ".BMP");
	}
	pos.w = graphics[0]->w;
	pos.h = graphics[0]->h;

}

CBonusSelection::CRegion::~CRegion()
{
	for(auto & elem : graphics)
	{
		SDL_FreeSurface(elem);
	}
}

void CBonusSelection::CRegion::show(SDL_Surface * to)
{
	//const SCampPositions::SRegionDesc & desc = owner->campDescriptions[owner->ourCampaign->camp->header.mapVersion].regions[myNumber];
	if(!accessible)
	{
		//show as striped
		blitAtLoc(graphics[2], 0, 0, to);
	}
	else if(this == owner->highlightedRegion)
	{
		//show as selected
		blitAtLoc(graphics[1], 0, 0, to);
	}
	else
	{
		//show as not selected selected
		blitAtLoc(graphics[0], 0, 0, to);
	}
}

void CBonusSelection::CRegion::clickLeft(tribool down, bool previousState)
{
	//select if selectable & clicked inside our graphic
	if(indeterminate(down))
	{
		return;
	}
	if(!down && selectable && !CSDL_Ext::isTransparent(graphics[0], GH.current->motion.x - pos.x, GH.current->motion.y - pos.y))
	{
		owner->selectMap(myNumber);
		owner->highlightedRegion = this;
		parent->showAll(screen);
	}
}

void CBonusSelection::CRegion::clickRight(tribool down, bool previousState)
{
	//show r-click text
	if(down && !CSDL_Ext::isTransparent(graphics[0], GH.current->motion.x - pos.x, GH.current->motion.y - pos.y) && rclickText.size())
	{
		CRClickPopup::createAndPush(rclickText);
	}
}
