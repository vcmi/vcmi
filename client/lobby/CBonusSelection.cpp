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

#include "CBonusSelection.h"

#include <vcmi/spells/Spell.h>
#include <vcmi/spells/Service.h>

#include "CSelectionBase.h"

#include "../CGameInfo.h"
#include "../CMessage.h"
#include "../CMusicHandler.h"
#include "../CVideoHandler.h"
#include "../CPlayerInterface.h"
#include "../CServerHandler.h"
#include "../gui/CAnimation.h"
#include "../gui/CGuiHandler.h"
#include "../mainmenu/CMainMenu.h"
#include "../mainmenu/CPrologEpilogVideo.h"
#include "../widgets/CComponent.h"
#include "../widgets/Buttons.h"
#include "../widgets/MiscWidgets.h"
#include "../widgets/ObjectLists.h"
#include "../widgets/TextControls.h"
#include "../windows/GUIClasses.h"
#include "../windows/InfoWindows.h"

#include "../../lib/filesystem/Filesystem.h"
#include "../../lib/CGeneralTextHandler.h"

#include "../../lib/CBuildingHandler.h"

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
	: CWindowObject(BORDERED)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL_NO_DISPOSE;
	static const std::string bgNames[] =
	{
		"E1_BG.BMP", "G2_BG.BMP", "E2_BG.BMP", "G1_BG.BMP", "G3_BG.BMP", "N1_BG.BMP",
		"S1_BG.BMP", "BR_BG.BMP", "IS_BG.BMP", "KR_BG.BMP", "NI_BG.BMP", "TA_BG.BMP", "AR_BG.BMP", "HS_BG.BMP",
		"BB_BG.BMP", "NB_BG.BMP", "EL_BG.BMP", "RN_BG.BMP", "UA_BG.BMP", "SP_BG.BMP"
	};
	loadPositionsOfGraphics();
	setBackground(bgNames[getCampaign()->camp->header.mapVersion]);

	panelBackground = std::make_shared<CPicture>("CAMPBRF.BMP", 456, 6);

	buttonStart = std::make_shared<CButton>(Point(475, 536), "CBBEGIB.DEF", CButton::tooltip(), std::bind(&CBonusSelection::startMap, this), SDLK_RETURN);
	buttonRestart = std::make_shared<CButton>(Point(475, 536), "CBRESTB.DEF", CButton::tooltip(), std::bind(&CBonusSelection::restartMap, this), SDLK_RETURN);
	buttonBack = std::make_shared<CButton>(Point(624, 536), "CBCANCB.DEF", CButton::tooltip(), std::bind(&CBonusSelection::goBack, this), SDLK_ESCAPE);

	campaignName = std::make_shared<CLabel>(481, 28, FONT_BIG, ETextAlignment::TOPLEFT, Colors::YELLOW, CSH->si->getCampaignName());

	iconsMapSizes = std::make_shared<CAnimImage>("SCNRMPSZ", 4, 0, 735, 26);

	labelCampaignDescription = std::make_shared<CLabel>(481, 63, FONT_SMALL, ETextAlignment::TOPLEFT, Colors::YELLOW, CGI->generaltexth->allTexts[38]);
	campaignDescription = std::make_shared<CTextBox>(getCampaign()->camp->header.description, Rect(480, 86, 286, 117), 1);

	mapName = std::make_shared<CLabel>(481, 219, FONT_BIG, ETextAlignment::TOPLEFT, Colors::YELLOW, CSH->mi->getName());
	labelMapDescription = std::make_shared<CLabel>(481, 253, FONT_SMALL, ETextAlignment::TOPLEFT, Colors::YELLOW, CGI->generaltexth->allTexts[496]);
	mapDescription = std::make_shared<CTextBox>("", Rect(480, 280, 286, 117), 1);

	labelChooseBonus = std::make_shared<CLabel>(511, 432, FONT_SMALL, ETextAlignment::TOPLEFT, Colors::WHITE, CGI->generaltexth->allTexts[71]);
	groupBonuses = std::make_shared<CToggleGroup>(std::bind(&IServerAPI::setCampaignBonus, CSH, _1));

	flagbox = std::make_shared<CFlagBox>(Rect(486, 407, 335, 23));

	std::vector<std::string> difficulty;
	std::string difficultyString = CGI->generaltexth->allTexts[492];
	boost::split(difficulty, difficultyString, boost::is_any_of(" "));
	labelDifficulty = std::make_shared<CLabel>(689, 432, FONT_MEDIUM, ETextAlignment::TOPLEFT, Colors::WHITE, difficulty.back());

	for(size_t b = 0; b < difficultyIcons.size(); ++b)
	{
		difficultyIcons[b] = std::make_shared<CAnimImage>("GSPBUT" + boost::lexical_cast<std::string>(b + 3) + ".DEF", 0, 0, 709, 455);
	}

	if(getCampaign()->camp->header.difficultyChoosenByPlayer)
	{
		buttonDifficultyLeft = std::make_shared<CButton>(Point(694, 508), "SCNRBLF.DEF", CButton::tooltip(), std::bind(&CBonusSelection::decreaseDifficulty, this));
		buttonDifficultyRight = std::make_shared<CButton>(Point(738, 508), "SCNRBRT.DEF", CButton::tooltip(), std::bind(&CBonusSelection::increaseDifficulty, this));
	}

	for(int g = 0; g < getCampaign()->camp->scenarios.size(); ++g)
	{
		if(getCampaign()->camp->conquerable(g))
			regions.push_back(std::make_shared<CRegion>(g, true, true, campDescriptions[getCampaign()->camp->header.mapVersion]));
		else if(getCampaign()->camp->scenarios[g].conquered) //display as striped
			regions.push_back(std::make_shared<CRegion>(g, false, false, campDescriptions[getCampaign()->camp->header.mapVersion]));
	}
}

void CBonusSelection::loadPositionsOfGraphics()
{
	const JsonNode config(ResourceID("config/campaign_regions.json"));
	int idx = 0;

	for(const JsonNode & campaign : config["campaign_regions"].Vector())
	{
		SCampPositions sc;

		sc.campPrefix = campaign["prefix"].String();
		sc.colorSuffixLength = static_cast<int>(campaign["color_suffix_length"].Float());

		for(const JsonNode & desc : campaign["desc"].Vector())
		{
			SCampPositions::SRegionDesc rd;

			rd.infix = desc["infix"].String();
			rd.xpos = static_cast<int>(desc["x"].Float());
			rd.ypos = static_cast<int>(desc["y"].Float());
			sc.regions.push_back(rd);
		}

		campDescriptions.push_back(sc);

		idx++;
	}
}

void CBonusSelection::createBonusesIcons()
{
	OBJ_CONSTRUCTION_CAPTURING_ALL_NO_DISPOSE;
	const CCampaignScenario & scenario = getCampaign()->camp->scenarios[CSH->campaignMap];
	const std::vector<CScenarioTravel::STravelBonus> & bonDescs = scenario.travelOptions.bonusesToChoose;
	groupBonuses = std::make_shared<CToggleGroup>(std::bind(&IServerAPI::setCampaignBonus, CSH, _1));

	static const char * bonusPics[] =
	{
		"SPELLBON.DEF",	// Spell
		"TWCRPORT.DEF", // Monster
		"", // Building - BO*.BMP
		"ARTIFBON.DEF", // Artifact
		"SPELLBON.DEF", // Spell scroll
		"PSKILBON.DEF", // Primary skill
		"SSKILBON.DEF", // Secondary skill
		"BORES.DEF", // Resource
		"PORTRAITSLARGE", // Hero HPL*.BMP
		"PORTRAITSLARGE"
		// Player - CREST58.DEF
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
			boost::algorithm::replace_first(desc, "%s", CGI->spells()->getByIndex(bonDescs[i].info2)->getName());
			break;
		case CScenarioTravel::STravelBonus::MONSTER:
			picNumber = bonDescs[i].info2 + 2;
			desc = CGI->generaltexth->allTexts[717];
			boost::algorithm::replace_first(desc, "%d", boost::lexical_cast<std::string>(bonDescs[i].info3));
			boost::algorithm::replace_first(desc, "%s", CGI->creatures()->getByIndex(bonDescs[i].info2)->getPluralName());
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

			if(vstd::contains((*CGI->townh)[faction]->town->buildings, buildID))
				desc = (*CGI->townh)[faction]->town->buildings.find(buildID)->second->Name();

			break;
		}
		case CScenarioTravel::STravelBonus::ARTIFACT:
			desc = CGI->generaltexth->allTexts[715];
			boost::algorithm::replace_first(desc, "%s", CGI->artifacts()->getByIndex(bonDescs[i].info2)->getName());
			break;
		case CScenarioTravel::STravelBonus::SPELL_SCROLL:
			desc = CGI->generaltexth->allTexts[716];
			boost::algorithm::replace_first(desc, "%s", CGI->spells()->getByIndex(bonDescs[i].info2)->getName());
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

			boost::algorithm::replace_first(desc, "%s", getCampaign()->camp->scenarios[bonDescs[i].info2].scenarioName);
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
				boost::algorithm::replace_first(desc, "%s", CGI->heroh->objects[bonDescs[i].info2]->name);
			}
			break;
		}

		std::shared_ptr<CToggleButton> bonusButton = std::make_shared<CToggleButton>(Point(475 + i * 68, 455), "", CButton::tooltip(desc, desc));

		if(picNumber != -1)
			picName += ":" + boost::lexical_cast<std::string>(picNumber);

		auto anim = std::make_shared<CAnimation>();
		anim->setCustom(picName, 0);
		bonusButton->setImage(anim);
		if(CSH->campaignBonus == i)
			bonusButton->setBorderColor(Colors::BRIGHT_YELLOW);
		groupBonuses->addToggle(i, bonusButton);
	}

	if(vstd::contains(getCampaign()->chosenCampaignBonuses, CSH->campaignMap))
	{
		groupBonuses->setSelected(getCampaign()->chosenCampaignBonuses[CSH->campaignMap]);
	}
}

void CBonusSelection::updateAfterStateChange()
{
	if(CSH->state != EClientState::GAMEPLAY)
	{
		buttonRestart->disable();
		buttonStart->enable();
		if(!getCampaign()->mapsConquered.empty())
			buttonBack->block(true);
		else
			buttonBack->block(false);
	}
	else
	{
		buttonStart->disable();
		buttonRestart->enable();
		buttonBack->block(false);
		if(buttonDifficultyLeft)
			buttonDifficultyLeft->disable();
		if(buttonDifficultyRight)
			buttonDifficultyRight->disable();
	}
	if(CSH->campaignBonus == -1)
	{
		buttonStart->block(getCampaign()->camp->scenarios[CSH->campaignMap].travelOptions.bonusesToChoose.size());
	}
	else if(buttonStart->isBlocked())
	{
		buttonStart->block(false);
	}

	for(auto region : regions)
		region->updateState();

	if(!CSH->mi)
		return;
	iconsMapSizes->setFrame(CSH->mi->getMapSizeIconId());
	mapDescription->setText(CSH->mi->getDescription());
	for(size_t i = 0; i < difficultyIcons.size(); i++)
	{
		if(i == CSH->si->difficulty)
			difficultyIcons[i]->enable();
		else
			difficultyIcons[i]->disable();
	}
	flagbox->recreate();
	createBonusesIcons();
}

void CBonusSelection::goBack()
{
	if(CSH->state != EClientState::GAMEPLAY)
	{
		GH.popInts(2);
	}
	else
	{
		close();
	}
	// TODO: we can actually only pop bonus selection interface for custom campaigns
	// Though this would require clearing CLobbyScreen::bonusSel pointer when poping this interface
/*
	else
	{
		close();
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
			GH.pushIntT<CPrologEpilogVideo>(scenario.prolog, exitCb);
		}
		else
		{
			exitCb();
		}
	};
	
	//block buttons immediately
	buttonStart->block(true);
	buttonRestart->block(true);
	buttonBack->block(true);

	if(LOCPLINT) // we're currently ingame, so ask for starting new map and end game
	{
		close();
		LOCPLINT->showYesNoDialog(CGI->generaltexth->allTexts[67], [=]()
		{
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
	close();
	LOCPLINT->showYesNoDialog(CGI->generaltexth->allTexts[67], [=]()
	{
		LOCPLINT->sendCustomEvent(EUserEvent::RESTART_GAME);
	}, 0);
}

void CBonusSelection::increaseDifficulty()
{
	CSH->setDifficulty(CSH->si->difficulty + 1);
}

void CBonusSelection::decreaseDifficulty()
{
	// avoid negative overflow (0 - 1 = 255)
	if (CSH->si->difficulty > 0)
		CSH->setDifficulty(CSH->si->difficulty - 1);
}

CBonusSelection::CRegion::CRegion(int id, bool accessible, bool selectable, const SCampPositions & campDsc)
	: CIntObject(LCLICK | RCLICK), idOfMapAndRegion(id), accessible(accessible), selectable(selectable)
{
	OBJ_CONSTRUCTION;
	static const std::string colors[2][8] =
	{
		{"R", "B", "N", "G", "O", "V", "T", "P"},
		{"Re", "Bl", "Br", "Gr", "Or", "Vi", "Te", "Pi"}
	};

	const SCampPositions::SRegionDesc & desc = campDsc.regions[idOfMapAndRegion];
	pos.x += desc.xpos;
	pos.y += desc.ypos;

	std::string prefix = campDsc.campPrefix + desc.infix + "_";
	std::string suffix = colors[campDsc.colorSuffixLength - 1][CSH->si->campState->camp->scenarios[idOfMapAndRegion].regionColor];
	graphicsNotSelected = std::make_shared<CPicture>(prefix + "En" + suffix + ".BMP");
	graphicsNotSelected->disable();
	graphicsSelected = std::make_shared<CPicture>(prefix + "Se" + suffix + ".BMP");
	graphicsSelected->disable();
	graphicsStriped = std::make_shared<CPicture>(prefix + "Co" + suffix + ".BMP");
	graphicsStriped->disable();
	pos.w = graphicsNotSelected->bg->w;
	pos.h = graphicsNotSelected->bg->h;

}

void CBonusSelection::CRegion::updateState()
{
	if(!accessible)
	{
		graphicsNotSelected->disable();
		graphicsSelected->disable();
		graphicsStriped->enable();
	}
	else if(CSH->campaignMap == idOfMapAndRegion)
	{
		graphicsNotSelected->disable();
		graphicsSelected->enable();
		graphicsStriped->disable();
	}
	else
	{
		graphicsNotSelected->enable();
		graphicsSelected->disable();
		graphicsStriped->disable();
	}
}

void CBonusSelection::CRegion::clickLeft(tribool down, bool previousState)
{
	//select if selectable & clicked inside our graphic
	if(indeterminate(down))
		return;

	if(!down && selectable && !CSDL_Ext::isTransparent(graphicsNotSelected->getSurface(), GH.current->motion.x - pos.x, GH.current->motion.y - pos.y))
	{
		CSH->setCampaignMap(idOfMapAndRegion);
	}
}

void CBonusSelection::CRegion::clickRight(tribool down, bool previousState)
{
	// FIXME: For some reason "down" is only ever contain indeterminate_value
	auto text = CSH->si->campState->camp->scenarios[idOfMapAndRegion].regionText;
	if(!CSDL_Ext::isTransparent(graphicsNotSelected->getSurface(), GH.current->motion.x - pos.x, GH.current->motion.y - pos.y) && text.size())
	{
		CRClickPopup::createAndPush(text);
	}
}
