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
#include "ExtraOptionsTab.h"

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
#include "../render/IRenderHandler.h"
#include "../render/CAnimation.h"
#include "../render/Graphics.h"
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

	setBackground(getCampaign()->getRegions().getBackgroundName());

	panelBackground = std::make_shared<CPicture>(ImagePath::builtin("CAMPBRF.BMP"), 456, 6);

	buttonStart = std::make_shared<CButton>(Point(475, 536), AnimationPath::builtin("CBBEGIB.DEF"), CButton::tooltip(), std::bind(&CBonusSelection::startMap, this), EShortcut::GLOBAL_ACCEPT);
	buttonRestart = std::make_shared<CButton>(Point(475, 536), AnimationPath::builtin("CBRESTB.DEF"), CButton::tooltip(), std::bind(&CBonusSelection::restartMap, this), EShortcut::GLOBAL_ACCEPT);
	buttonVideo = std::make_shared<CButton>(Point(705, 214), AnimationPath::builtin("CBVIDEB.DEF"), CButton::tooltip(), [this](){ GH.windows().createAndPushWindow<CPrologEpilogVideo>(getCampaign()->scenario(CSH->campaignMap).prolog, [this](){ redraw(); }); });
	buttonBack = std::make_shared<CButton>(Point(624, 536), AnimationPath::builtin("CBCANCB.DEF"), CButton::tooltip(), std::bind(&CBonusSelection::goBack, this), EShortcut::GLOBAL_CANCEL);

	campaignName = std::make_shared<CLabel>(481, 28, FONT_BIG, ETextAlignment::TOPLEFT, Colors::YELLOW, CSH->si->getCampaignName());

	iconsMapSizes = std::make_shared<CAnimImage>(AnimationPath::builtin("SCNRMPSZ"), 4, 0, 735, 26);

	labelCampaignDescription = std::make_shared<CLabel>(481, 63, FONT_SMALL, ETextAlignment::TOPLEFT, Colors::YELLOW, CGI->generaltexth->allTexts[38]);
	campaignDescription = std::make_shared<CTextBox>(getCampaign()->getDescriptionTranslated(), Rect(480, 86, 286, 117), 1);

	mapName = std::make_shared<CLabel>(481, 219, FONT_BIG, ETextAlignment::TOPLEFT, Colors::YELLOW, CSH->mi->getNameTranslated());
	labelMapDescription = std::make_shared<CLabel>(481, 253, FONT_SMALL, ETextAlignment::TOPLEFT, Colors::YELLOW, CGI->generaltexth->allTexts[496]);
	mapDescription = std::make_shared<CTextBox>("", Rect(480, 278, 292, 108), 1);

	labelChooseBonus = std::make_shared<CLabel>(475, 432, FONT_SMALL, ETextAlignment::TOPLEFT, Colors::WHITE, CGI->generaltexth->allTexts[71]);
	groupBonuses = std::make_shared<CToggleGroup>(std::bind(&IServerAPI::setCampaignBonus, CSH, _1));

	flagbox = std::make_shared<CFlagBox>(Rect(486, 407, 335, 23));

	std::vector<std::string> difficulty;
	std::string difficultyString = CGI->generaltexth->allTexts[492];
	boost::split(difficulty, difficultyString, boost::is_any_of(" "));
	labelDifficulty = std::make_shared<CLabel>(724, settings["general"]["enableUiEnhancements"].Bool() ? 457 : 432, FONT_MEDIUM, ETextAlignment::TOPCENTER, Colors::WHITE, difficulty.back());

	for(size_t b = 0; b < difficultyIcons.size(); ++b)
	{
		difficultyIcons[b] = std::make_shared<CAnimImage>(AnimationPath::builtinTODO("GSPBUT" + std::to_string(b + 3) + ".DEF"), 0, 0, 709, settings["general"]["enableUiEnhancements"].Bool() ? 480 : 455);
	}

	if(getCampaign()->playerSelectedDifficulty())
	{
		buttonDifficultyLeft = std::make_shared<CButton>(settings["general"]["enableUiEnhancements"].Bool() ? Point(693, 495) : Point(694, 508), AnimationPath::builtin("SCNRBLF.DEF"), CButton::tooltip(), std::bind(&CBonusSelection::decreaseDifficulty, this));
		buttonDifficultyRight = std::make_shared<CButton>(settings["general"]["enableUiEnhancements"].Bool() ? Point(739, 495) : Point(738, 508), AnimationPath::builtin("SCNRBRT.DEF"), CButton::tooltip(), std::bind(&CBonusSelection::increaseDifficulty, this));
	}

	for(auto scenarioID : getCampaign()->allScenarios())
	{
		if(getCampaign()->isAvailable(scenarioID))
			regions.push_back(std::make_shared<CRegion>(scenarioID, true, true, getCampaign()->getRegions()));
		else if(getCampaign()->isConquered(scenarioID)) //display as striped
			regions.push_back(std::make_shared<CRegion>(scenarioID, false, false, getCampaign()->getRegions()));
	}

	if (!getCampaign()->getMusic().empty())
		CCS->musich->playMusic( getCampaign()->getMusic(), true, false);

	if(settings["general"]["enableUiEnhancements"].Bool())
	{
		tabExtraOptions = std::make_shared<ExtraOptionsTab>();
		tabExtraOptions->recActions = UPDATE | SHOWALL | LCLICK | RCLICK_POPUP;
		tabExtraOptions->recreate(true);
		tabExtraOptions->setEnabled(false);
		buttonExtraOptions = std::make_shared<CButton>(Point(643, 431), AnimationPath::builtin("GSPBUT2.DEF"), CGI->generaltexth->zelp[46], [this]{ tabExtraOptions->setEnabled(!tabExtraOptions->isActive()); GH.windows().totalRedraw(); }, EShortcut::NONE);
		buttonExtraOptions->setTextOverlay(CGI->generaltexth->translate("vcmi.optionsTab.extraOptions.hover"), FONT_SMALL, Colors::WHITE);
	}
}

void CBonusSelection::createBonusesIcons()
{
	OBJ_CONSTRUCTION_CAPTURING_ALL_NO_DISPOSE;
	const CampaignScenario & scenario = getCampaign()->scenario(CSH->campaignMap);
	const std::vector<CampaignBonus> & bonDescs = scenario.travelOptions.bonusesToChoose;
	groupBonuses = std::make_shared<CToggleGroup>(std::bind(&IServerAPI::setCampaignBonus, CSH, _1));

	constexpr std::array bonusPics =
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

		MetaString desc;
		switch(bonDescs[i].type)
		{
		case CampaignBonusType::SPELL:
			desc.appendLocalString(EMetaText::GENERAL_TXT, 715);
			desc.replaceName(SpellID(bonDescs[i].info2));
			break;
		case CampaignBonusType::MONSTER:
			picNumber = bonDescs[i].info2 + 2;
			desc.appendLocalString(EMetaText::GENERAL_TXT, 717);
			desc.replaceNumber(bonDescs[i].info3);
			desc.replaceNamePlural(bonDescs[i].info2);
			break;
		case CampaignBonusType::BUILDING:
		{
			FactionID faction;
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
				desc.appendTextID((*CGI->townh)[faction]->town->buildings.find(buildID)->second->getNameTextID());
			break;
		}
		case CampaignBonusType::ARTIFACT:
			desc.appendLocalString(EMetaText::GENERAL_TXT, 715);
			desc.replaceName(ArtifactID(bonDescs[i].info2));
			break;
		case CampaignBonusType::SPELL_SCROLL:
			desc.appendLocalString(EMetaText::GENERAL_TXT, 716);
			desc.replaceName(ArtifactID(bonDescs[i].info2));
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
			desc.appendLocalString(EMetaText::GENERAL_TXT, 715);

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

			desc.replaceRawString(substitute);
			break;
		}
		case CampaignBonusType::SECONDARY_SKILL:
			desc.appendLocalString(EMetaText::GENERAL_TXT, 718);
			desc.replaceTextID(TextIdentifier("core", "genrltxt", "levels", bonDescs[i].info3 - 1).get());
			desc.replaceName(SecondarySkill(bonDescs[i].info2));
			picNumber = bonDescs[i].info2 * 3 + bonDescs[i].info3 - 1;

			break;
		case CampaignBonusType::RESOURCE:
		{
			desc.appendLocalString(EMetaText::GENERAL_TXT, 717);

			switch(bonDescs[i].info1)
			{
				case 0xFD: //wood + ore
				{
					desc.replaceLocalString(EMetaText::GENERAL_TXT, 721);
					picNumber = 7;
					break;
				}
				case 0xFE: //wood + ore
				{
					desc.replaceLocalString(EMetaText::GENERAL_TXT, 722);
					picNumber = 8;
					break;
				}
				default:
				{
					desc.replaceName(GameResID(bonDescs[i].info1));
					picNumber = bonDescs[i].info1;
				}
			}

			desc.replaceNumber(bonDescs[i].info2);
			break;
		}
		case CampaignBonusType::HEROES_FROM_PREVIOUS_SCENARIO:
		{
			auto superhero = getCampaign()->strongestHero(static_cast<CampaignScenarioID>(bonDescs[i].info2), PlayerColor(bonDescs[i].info1));
			if(!superhero)
				logGlobal->warn("No superhero! How could it be transferred?");
			picNumber = superhero ? superhero->getIconIndex() : 0;
			desc.appendLocalString(EMetaText::GENERAL_TXT, 719);
			desc.replaceRawString(getCampaign()->scenario(static_cast<CampaignScenarioID>(bonDescs[i].info2)).scenarioName.toString());
			break;
		}

		case CampaignBonusType::HERO:
			if(bonDescs[i].info2 == 0xFFFF)
			{
				desc.appendLocalString(EMetaText::GENERAL_TXT, 720); // Start with random hero
				picNumber = -1;
				picName = "CBONN1A3.BMP";
			}
			else
			{
				desc.appendLocalString(EMetaText::GENERAL_TXT, 715); // Start with %s
				desc.replaceTextID(CGI->heroh->objects[bonDescs[i].info2]->getNameTextID());
			}
			break;
		}

		std::shared_ptr<CToggleButton> bonusButton = std::make_shared<CToggleButton>(Point(475 + i * 68, 455), AnimationPath::builtin("campaignBonusSelection"), CButton::tooltip(desc.toString(), desc.toString()));

		if(picNumber != -1)
			bonusButton->setOverlay(std::make_shared<CAnimImage>(AnimationPath::builtin(picName), picNumber));
		else
			bonusButton->setOverlay(std::make_shared<CPicture>(ImagePath::builtin(picName)));

		if(CSH->campaignBonus == i)
			bonusButton->setBorderColor(Colors::BRIGHT_YELLOW);
		groupBonuses->addToggle(i, bonusButton);
	}

	if(getCampaign()->getBonusID(CSH->campaignMap))
		groupBonuses->setSelected(*getCampaign()->getBonusID(CSH->campaignMap));
}

void CBonusSelection::updateAfterStateChange()
{
	if(CSH->getState() != EClientState::GAMEPLAY)
	{
		buttonRestart->disable();
		buttonVideo->disable();
		buttonStart->enable();
		buttonBack->block(!getCampaign()->conqueredScenarios().empty());
	}
	else
	{
		buttonStart->disable();
		buttonRestart->enable();
		buttonVideo->enable();
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
	else
	{
		buttonStart->block(false);
	}

	for(auto region : regions)
		region->updateState();

	if(!CSH->mi)
		return;
	iconsMapSizes->setFrame(CSH->mi->getMapSizeIconId());
	mapName->setText(CSH->mi->getNameTranslated());
	mapDescription->setText(CSH->mi->getDescriptionTranslated());
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
	if(CSH->getState() != EClientState::GAMEPLAY)
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
	if (!CSH->validateGameStart())
		return;

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
	buttonVideo->block(true);
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

void CBonusSelection::CRegion::clickReleased(const Point & cursorPosition)
{
	if(selectable && !graphicsNotSelected->getSurface()->isTransparent(cursorPosition - pos.topLeft()))
	{
		CSH->setCampaignMap(idOfMapAndRegion);
	}
}

void CBonusSelection::CRegion::showPopupWindow(const Point & cursorPosition)
{
	// FIXME: For some reason "down" is only ever contain indeterminate_value
	auto & text = CSH->si->campState->scenario(idOfMapAndRegion).regionText;
	if(!graphicsNotSelected->getSurface()->isTransparent(cursorPosition - pos.topLeft()) && !text.empty())
	{
		CRClickPopup::createAndPush(text.toString());
	}
}
