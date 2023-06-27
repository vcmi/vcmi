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
#include "../CMusicHandler.h"
#include "../CVideoHandler.h"
#include "../CPlayerInterface.h"
#include "../CServerHandler.h"
#include "../mainmenu/CMainMenu.h"
#include "../mainmenu/CPrologEpilogVideo.h"
#include "../widgets/CComponent.h"
#include "../widgets/Buttons.h"
#include "../widgets/MiscWidgets.h"
#include "../widgets/ObjectLists.h"
#include "../widgets/TextControls.h"
#include "../windows/GUIClasses.h"
#include "../windows/InfoWindows.h"
#include "../render/IImage.h"
#include "../render/CAnimation.h"
#include "../gui/CGuiHandler.h"
#include "../gui/Shortcut.h"
#include "../gui/WindowHandler.h"

#include "../../lib/filesystem/Filesystem.h"
#include "../../lib/CGeneralTextHandler.h"

#include "../../lib/CBuildingHandler.h"

#include "../../lib/CSkillHandler.h"
#include "../../lib/CTownHandler.h"
#include "../../lib/CHeroHandler.h"
#include "../../lib/CCreatureHandler.h"
#include "../../lib/StartInfo.h"

#include "../../lib/campaign/CampaignState.h"
#include "../../lib/mapping/CMapService.h"
#include "../../lib/mapping/CMapInfo.h"

#include "../../lib/mapObjects/CGHeroInstance.h"


std::shared_ptr<CampaignState> CBonusSelection::getCampaign()
{
	return CSH->si->campState;
}

CBonusSelection::CBonusSelection()
	: CWindowObject(BORDERED)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL_NO_DISPOSE;

	std::string bgName = getCampaign()->getRegions().getBackgroundName();
	setBackground(bgName);

	panelBackground = std::make_shared<CPicture>("CAMPBRF.BMP", 456, 6);

	buttonStart = std::make_shared<CButton>(Point(475, 536), "CBBEGIB.DEF", CButton::tooltip(), std::bind(&CBonusSelection::startMap, this), EShortcut::GLOBAL_ACCEPT);
	buttonRestart = std::make_shared<CButton>(Point(475, 536), "CBRESTB.DEF", CButton::tooltip(), std::bind(&CBonusSelection::restartMap, this), EShortcut::GLOBAL_ACCEPT);
	buttonBack = std::make_shared<CButton>(Point(624, 536), "CBCANCB.DEF", CButton::tooltip(), std::bind(&CBonusSelection::goBack, this), EShortcut::GLOBAL_CANCEL);

	campaignName = std::make_shared<CLabel>(481, 28, FONT_BIG, ETextAlignment::TOPLEFT, Colors::YELLOW, CSH->si->getCampaignName());

	iconsMapSizes = std::make_shared<CAnimImage>("SCNRMPSZ", 4, 0, 735, 26);

	labelCampaignDescription = std::make_shared<CLabel>(481, 63, FONT_SMALL, ETextAlignment::TOPLEFT, Colors::YELLOW, CGI->generaltexth->allTexts[38]);
	campaignDescription = std::make_shared<CTextBox>(getCampaign()->getDescription(), Rect(480, 86, 286, 117), 1);

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
		difficultyIcons[b] = std::make_shared<CAnimImage>("GSPBUT" + std::to_string(b + 3) + ".DEF", 0, 0, 709, 455);
	}

	if(getCampaign()->playerSelectedDifficulty())
	{
		buttonDifficultyLeft = std::make_shared<CButton>(Point(694, 508), "SCNRBLF.DEF", CButton::tooltip(), std::bind(&CBonusSelection::decreaseDifficulty, this));
		buttonDifficultyRight = std::make_shared<CButton>(Point(738, 508), "SCNRBRT.DEF", CButton::tooltip(), std::bind(&CBonusSelection::increaseDifficulty, this));
	}

	for(auto scenarioID : getCampaign()->allScenarios())
	{
		if(getCampaign()->isAvailable(scenarioID))
			regions.push_back(std::make_shared<CRegion>(scenarioID, true, true, getCampaign()->getRegions()));
		else if(getCampaign()->isConquered(scenarioID)) //display as striped
			regions.push_back(std::make_shared<CRegion>(scenarioID, false, false, getCampaign()->getRegions()));
	}
}

void CBonusSelection::createBonusesIcons()
{
	OBJ_CONSTRUCTION_CAPTURING_ALL_NO_DISPOSE;
	const CampaignScenario & scenario = getCampaign()->scenario(CSH->campaignMap);
	const std::vector<CampaignBonus> & bonDescs = scenario.travelOptions.bonusesToChoose;
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
		int bonusType = static_cast<size_t>(bonDescs[i].type);
		std::string picName = bonusPics[bonusType];
		size_t picNumber = bonDescs[i].info2;

		std::string desc;
		switch(bonDescs[i].type)
		{
		case CampaignBonusType::SPELL:
			desc = CGI->generaltexth->allTexts[715];
			boost::algorithm::replace_first(desc, "%s", CGI->spells()->getByIndex(bonDescs[i].info2)->getNameTranslated());
			break;
		case CampaignBonusType::MONSTER:
			picNumber = bonDescs[i].info2 + 2;
			desc = CGI->generaltexth->allTexts[717];
			boost::algorithm::replace_first(desc, "%d", std::to_string(bonDescs[i].info3));
			boost::algorithm::replace_first(desc, "%s", CGI->creatures()->getByIndex(bonDescs[i].info2)->getNamePluralTranslated());
			break;
		case CampaignBonusType::BUILDING:
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

			BuildingID buildID;
			if(getCampaign()->formatVCMI())
				buildID = BuildingID(bonDescs[i].info1);
			else
				buildID = CBuildingHandler::campToERMU(bonDescs[i].info1, faction, std::set<BuildingID>());
			picName = graphics->ERMUtoPicture[faction][buildID];
			picNumber = -1;

			if(vstd::contains((*CGI->townh)[faction]->town->buildings, buildID))
				desc = (*CGI->townh)[faction]->town->buildings.find(buildID)->second->getNameTranslated();

			break;
		}
		case CampaignBonusType::ARTIFACT:
			desc = CGI->generaltexth->allTexts[715];
			boost::algorithm::replace_first(desc, "%s", CGI->artifacts()->getByIndex(bonDescs[i].info2)->getNameTranslated());
			break;
		case CampaignBonusType::SPELL_SCROLL:
			desc = CGI->generaltexth->allTexts[716];
			boost::algorithm::replace_first(desc, "%s", CGI->spells()->getByIndex(bonDescs[i].info2)->getNameTranslated());
			break;
		case CampaignBonusType::PRIMARY_SKILL:
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
				substitute += std::to_string(toPrint[v].second);
				substitute += " " + CGI->generaltexth->primarySkillNames[toPrint[v].first];
				if(v != toPrint.size() - 1)
				{
					substitute += ", ";
				}
			}

			boost::algorithm::replace_first(desc, "%s", substitute);
			break;
		}
		case CampaignBonusType::SECONDARY_SKILL:
			desc = CGI->generaltexth->allTexts[718];

			boost::algorithm::replace_first(desc, "%s", CGI->generaltexth->levels[bonDescs[i].info3 - 1]); //skill level
			boost::algorithm::replace_first(desc, "%s", CGI->skillh->getByIndex(bonDescs[i].info2)->getNameTranslated()); //skill name
			picNumber = bonDescs[i].info2 * 3 + bonDescs[i].info3 - 1;

			break;
		case CampaignBonusType::RESOURCE:
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
			boost::algorithm::replace_first(desc, "%d", std::to_string(bonDescs[i].info2));
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
		case CampaignBonusType::HEROES_FROM_PREVIOUS_SCENARIO:
		{
			auto superhero = getCampaign()->strongestHero(static_cast<CampaignScenarioID>(bonDescs[i].info2), PlayerColor(bonDescs[i].info1));
			if(!superhero)
				logGlobal->warn("No superhero! How could it be transferred?");
			picNumber = superhero ? superhero->portrait : 0;
			desc = CGI->generaltexth->allTexts[719];

			boost::algorithm::replace_first(desc, "%s", getCampaign()->scenario(static_cast<CampaignScenarioID>(bonDescs[i].info2)).scenarioName);
			break;
		}

		case CampaignBonusType::HERO:

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
				boost::algorithm::replace_first(desc, "%s", CGI->heroh->objects[bonDescs[i].info2]->getNameTranslated());
			}
			break;
		}

		std::shared_ptr<CToggleButton> bonusButton = std::make_shared<CToggleButton>(Point(475 + i * 68, 455), "", CButton::tooltip(desc, desc));

		if(picNumber != -1)
			picName += ":" + std::to_string(picNumber);

		auto anim = std::make_shared<CAnimation>();
		anim->setCustom(picName, 0);
		bonusButton->setImage(anim);
		if(CSH->campaignBonus == i)
			bonusButton->setBorderColor(Colors::BRIGHT_YELLOW);
		groupBonuses->addToggle(i, bonusButton);
	}

	if(getCampaign()->getBonusID(CSH->campaignMap))
		groupBonuses->setSelected(*getCampaign()->getBonusID(CSH->campaignMap));
}

void CBonusSelection::updateAfterStateChange()
{
	if(CSH->state != EClientState::GAMEPLAY)
	{
		buttonRestart->disable();
		buttonStart->enable();
		if(!getCampaign()->conqueredScenarios().empty())
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
		buttonStart->block(getCampaign()->scenario(CSH->campaignMap).travelOptions.bonusesToChoose.size());
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
	mapName->setText(CSH->mi->getName());
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
		GH.windows().popWindows(2);
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
			logGlobal->info("Starting scenario %d", static_cast<int>(CSH->campaignMap));
			CSH->sendStartGame();
		};

		const CampaignScenario & scenario = getCampaign()->scenario(CSH->campaignMap);
		if(scenario.prolog.hasPrologEpilog)
		{
			GH.windows().createAndPushWindow<CPrologEpilogVideo>(scenario.prolog, exitCb);
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
	LOCPLINT->showYesNoDialog(
		CGI->generaltexth->allTexts[67],
		[=]()
		{
			GH.dispatchMainThread(
				[]()
				{
					CSH->sendRestartGame();
				}
			);
		},
		0
	);
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

CBonusSelection::CRegion::CRegion(CampaignScenarioID id, bool accessible, bool selectable, const CampaignRegions & campDsc)
	: CIntObject(LCLICK | SHOW_POPUP), idOfMapAndRegion(id), accessible(accessible), selectable(selectable)
{
	OBJ_CONSTRUCTION;

	pos += campDsc.getPosition(id);

	auto color = CSH->si->campState->scenario(idOfMapAndRegion).regionColor;

	graphicsNotSelected = std::make_shared<CPicture>(campDsc.getAvailableName(id, color));
	graphicsNotSelected->disable();
	graphicsSelected = std::make_shared<CPicture>(campDsc.getSelectedName(id, color));
	graphicsSelected->disable();
	graphicsStriped = std::make_shared<CPicture>(campDsc.getConqueredName(id, color));
	graphicsStriped->disable();
	pos.w = graphicsNotSelected->pos.w;
	pos.h = graphicsNotSelected->pos.h;
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

	if(!down && selectable && !graphicsNotSelected->getSurface()->isTransparent(GH.getCursorPosition() - pos.topLeft()))
	{
		CSH->setCampaignMap(idOfMapAndRegion);
	}
}

void CBonusSelection::CRegion::showPopupWindow()
{
	// FIXME: For some reason "down" is only ever contain indeterminate_value
	auto text = CSH->si->campState->scenario(idOfMapAndRegion).regionText;
	if(!graphicsNotSelected->getSurface()->isTransparent(GH.getCursorPosition() - pos.topLeft()) && text.size())
	{
		CRClickPopup::createAndPush(text);
	}
}
