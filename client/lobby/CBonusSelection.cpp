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

#include "../CPlayerInterface.h"
#include "../CServerHandler.h"
#include "../mainmenu/CMainMenu.h"
#include "../mainmenu/CPrologEpilogVideo.h"
#include "../media/IMusicPlayer.h"
#include "../widgets/CComponent.h"
#include "../widgets/Buttons.h"
#include "../widgets/MiscWidgets.h"
#include "../widgets/ObjectLists.h"
#include "../widgets/TextControls.h"
#include "../widgets/VideoWidget.h"
#include "../windows/GUIClasses.h"
#include "../windows/InfoWindows.h"
#include "../render/IImage.h"
#include "../render/IRenderHandler.h"
#include "../render/CAnimation.h"
#include "../render/Graphics.h"
#include "../GameEngine.h"
#include "../GameInstance.h"
#include "../gui/Shortcut.h"
#include "../gui/WindowHandler.h"
#include "../adventureMap/AdventureMapInterface.h"

#include "../../lib/CConfigHandler.h"
#include "../../lib/CCreatureHandler.h"
#include "../../lib/CSkillHandler.h"
#include "../../lib/GameLibrary.h"
#include "../../lib/StartInfo.h"
#include "../../lib/campaign/CampaignState.h"
#include "../../lib/entities/artifact/CArtifact.h"
#include "../../lib/entities/building/CBuilding.h"
#include "../../lib/entities/faction/CFaction.h"
#include "../../lib/entities/faction/CTown.h"
#include "../../lib/entities/faction/CTownHandler.h"
#include "../../lib/entities/hero/CHeroHandler.h"
#include "../../lib/spells/CSpellHandler.h"
#include "../../lib/filesystem/Filesystem.h"
#include "../../lib/mapObjects/CGHeroInstance.h"
#include "../../lib/mapping/CMapHeader.h"
#include "../../lib/mapping/CMapInfo.h"
#include "../../lib/mapping/CMapService.h"
#include "../../lib/texts/CGeneralTextHandler.h"
#include "mapping/MapFormatSettings.h"

std::shared_ptr<CampaignState> CBonusSelection::getCampaign()
{
	return GAME->server().si->campState;
}

CBonusSelection::CBonusSelection()
	: CWindowObject(BORDERED)
{
	OBJECT_CONSTRUCTION;

	setBackground(getCampaign()->getRegions().getBackgroundName());

	panelBackground = std::make_shared<CPicture>(ImagePath::builtin("CAMPBRF.BMP"), 456, 6);

	const auto & playVideo = [this]()
	{
		ENGINE->windows().createAndPushWindow<CPrologEpilogVideo>(
			getCampaign()->scenario(GAME->server().campaignMap).prolog,
			[this]() { redraw(); } );
	};

	buttonStart = std::make_shared<CButton>(
		Point(475, 536), AnimationPath::builtin("CBBEGIB.DEF"), CButton::tooltip(), std::bind(&CBonusSelection::startMap, this), EShortcut::GLOBAL_ACCEPT
		);
	buttonRestart = std::make_shared<CButton>(Point(475, 536), AnimationPath::builtin("CBRESTB.DEF"), CButton::tooltip(), std::bind(&CBonusSelection::restartMap, this), EShortcut::GLOBAL_ACCEPT);
	buttonVideo = std::make_shared<CButton>(Point(705, 214), AnimationPath::builtin("CBVIDEB.DEF"), CButton::tooltip(), playVideo, EShortcut::LOBBY_REPLAY_VIDEO);
	buttonBack = std::make_shared<CButton>(Point(624, 536), AnimationPath::builtin("CBCANCB.DEF"), CButton::tooltip(), std::bind(&CBonusSelection::goBack, this), EShortcut::GLOBAL_CANCEL);

	campaignName = std::make_shared<CLabel>(481, 28, FONT_BIG, ETextAlignment::TOPLEFT, Colors::YELLOW, GAME->server().si->getCampaignName(), 250);

	iconsMapSizes = std::make_shared<CAnimImage>(AnimationPath::builtin("SCNRMPSZ"), 4, 0, 735, 26);

	labelCampaignDescription = std::make_shared<CLabel>(481, 63, FONT_SMALL, ETextAlignment::TOPLEFT, Colors::YELLOW, LIBRARY->generaltexth->allTexts[38]);
	campaignDescription = std::make_shared<CTextBox>(getCampaign()->getDescriptionTranslated(), Rect(480, 86, 286, 117), 1);

	bool videoButtonActive = GAME->server().getState() == EClientState::GAMEPLAY;
	int availableSpace = videoButtonActive ? 225 : 285;
	mapName = std::make_shared<CLabel>(481, 219, FONT_BIG, ETextAlignment::TOPLEFT, Colors::YELLOW, GAME->server().mi->getNameTranslated(), availableSpace );
	labelMapDescription = std::make_shared<CLabel>(481, 253, FONT_SMALL, ETextAlignment::TOPLEFT, Colors::YELLOW, LIBRARY->generaltexth->allTexts[496]);
	mapDescription = std::make_shared<CTextBox>("", Rect(480, 278, 286, 108), 1);

	labelChooseBonus = std::make_shared<CLabel>(475, 432, FONT_SMALL, ETextAlignment::TOPLEFT, Colors::WHITE, LIBRARY->generaltexth->allTexts[71]);
	groupBonuses = std::make_shared<CToggleGroup>(std::bind(&IServerAPI::setCampaignBonus, &GAME->server(), _1));

	flagbox = std::make_shared<CFlagBox>(Rect(486, 407, 335, 23));

	std::vector<std::string> difficulty;
	std::string difficultyString = LIBRARY->generaltexth->allTexts[492];
	boost::split(difficulty, difficultyString, boost::is_any_of(" "));
	labelDifficulty = std::make_shared<CLabel>(724, settings["general"]["enableUiEnhancements"].Bool() ? 457 : 432, FONT_MEDIUM, ETextAlignment::TOPCENTER, Colors::WHITE, difficulty.back());

	for(size_t b = 0; b < difficultyIcons.size(); ++b)
	{
		difficultyIcons[b] = std::make_shared<CAnimImage>(AnimationPath::builtinTODO("GSPBUT" + std::to_string(b + 3) + ".DEF"), 0, 0, 709, settings["general"]["enableUiEnhancements"].Bool() ? 480 : 455);
		difficultyIconAreas[b] = std::make_shared<LRClickableArea>(difficultyIcons[b]->pos - pos.topLeft(), nullptr, [b]() { CRClickPopup::createAndPush(LIBRARY->generaltexth->zelp[24 + b].second); });
	}

	if(getCampaign()->playerSelectedDifficulty())
	{
		Point posLeft = settings["general"]["enableUiEnhancements"].Bool() ? Point(693, 495) : Point(694, 508);
		Point posRight = settings["general"]["enableUiEnhancements"].Bool() ? Point(739, 495) : Point(738, 508);

		buttonDifficultyLeft = std::make_shared<CButton>(posLeft, AnimationPath::builtin("SCNRBLF.DEF"), CButton::tooltip(), std::bind(&CBonusSelection::decreaseDifficulty, this), EShortcut::MOVE_LEFT);
		buttonDifficultyRight = std::make_shared<CButton>(posRight, AnimationPath::builtin("SCNRBRT.DEF"), CButton::tooltip(), std::bind(&CBonusSelection::increaseDifficulty, this), EShortcut::MOVE_RIGHT);
	}

	for(auto scenarioID : getCampaign()->allScenarios())
	{
		if(getCampaign()->isAvailable(scenarioID))
			regions.push_back(std::make_shared<CRegion>(scenarioID, true, true, false, getCampaign()->getRegions()));
		else if(getCampaign()->isConquered(scenarioID)) //display as striped
			regions.push_back(std::make_shared<CRegion>(scenarioID, false, false, false, getCampaign()->getRegions()));
		else
			regions.push_back(std::make_shared<CRegion>(scenarioID, false, false, true, getCampaign()->getRegions()));
	}

	if (!getCampaign()->getMusic().empty())
		ENGINE->music().playMusic( getCampaign()->getMusic(), true, false);

	if(GAME->server().getState() != EClientState::GAMEPLAY && settings["general"]["enableUiEnhancements"].Bool())
	{
		tabExtraOptions = std::make_shared<ExtraOptionsTab>();
		tabExtraOptions->recActions = UPDATE | SHOWALL | LCLICK | RCLICK_POPUP;
		tabExtraOptions->recreate(true);
		tabExtraOptions->setEnabled(false);
		buttonExtraOptions = std::make_shared<CButton>(Point(643, 431), AnimationPath::builtin("GSPBUT2.DEF"), LIBRARY->generaltexth->zelp[46], [this]{ tabExtraOptions->setEnabled(!tabExtraOptions->isActive()); ENGINE->windows().totalRedraw(); }, EShortcut::LOBBY_EXTRA_OPTIONS);
		buttonExtraOptions->setTextOverlay(LIBRARY->generaltexth->translate("vcmi.optionsTab.extraOptions.hover"), FONT_SMALL, Colors::WHITE);
	}
}

void CBonusSelection::createBonusesIcons()
{
	OBJECT_CONSTRUCTION;
	const CampaignScenario & scenario = getCampaign()->scenario(GAME->server().campaignMap);
	const std::vector<CampaignBonus> & bonDescs = scenario.travelOptions.bonusesToChoose;
	groupBonuses = std::make_shared<CToggleGroup>(std::bind(&IServerAPI::setCampaignBonus, &GAME->server(), _1));

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
		const CampaignBonus & bonus	= bonDescs[i];
		CampaignBonusType bonusType = bonus.getType();
		std::string picName = bonusPics[static_cast<int>(bonusType)];
		size_t picNumber = -1;

		MetaString desc;
		switch(bonusType)
		{
			case CampaignBonusType::SPELL:
			{
				const auto & bonusValue = bonus.getValue<CampaignBonusSpell>();
				const auto * spell = bonusValue.spell.toSpell();
				if (!spell->getIconScenarioBonus().empty())
					picName = spell->getIconScenarioBonus();
				else
					picNumber = bonusValue.spell.getNum();

				desc.appendLocalString(EMetaText::GENERAL_TXT, 715);
				desc.replaceName(bonusValue.spell);
				break;
			}
			case CampaignBonusType::MONSTER:
			{
				const auto & bonusValue = bonus.getValue<CampaignBonusCreatures>();
				picNumber = bonusValue.creature.getNum() + 2;
				desc.appendLocalString(EMetaText::GENERAL_TXT, 717);
				desc.replaceNumber(bonusValue.amount);
				desc.replaceNamePlural(bonusValue.creature);
				break;
			}
			case CampaignBonusType::BUILDING:
			{
				const auto & bonusValue = bonus.getValue<CampaignBonusBuilding>();
				FactionID faction;
				for(auto & elem : GAME->server().si->playerInfos)
				{
					if(elem.second.isControlledByHuman())
					{
						faction = elem.second.castle;
						break;
					}
				}
				assert(faction.hasValue());

				BuildingID buildID = bonusValue.buildingDecoded;
				if (bonusValue.buildingH3M.hasValue())
				{
					auto mapping = LIBRARY->mapFormat->getMapping(getCampaign()->getFormat());
					buildID = mapping.remapBuilding(faction, bonusValue.buildingH3M);
				}

				for (const auto & townStructure : faction.toFaction()->town->clientInfo.structures)
					if (townStructure->building && townStructure->building->bid == buildID)
						picName = townStructure->campaignBonus.getOriginalName();

				picNumber = -1;

				if(vstd::contains(faction.toFaction()->town->buildings, buildID))
					desc.appendTextID(faction.toFaction()->town->buildings.find(buildID)->second->getNameTextID());
				break;
			}
			case CampaignBonusType::ARTIFACT:
			{
				const auto & bonusValue = bonus.getValue<CampaignBonusArtifact>();
				const auto * artifact = bonusValue.artifact.toArtifact();
				if (!artifact->scenarioBonus.empty())
					picName = artifact->scenarioBonus;
				else
					picNumber = bonusValue.artifact.getNum();
				desc.appendLocalString(EMetaText::GENERAL_TXT, 715);
				desc.replaceName(bonusValue.artifact);
				break;
			}
			case CampaignBonusType::SPELL_SCROLL:
			{
				const auto & bonusValue = bonus.getValue<CampaignBonusSpellScroll>();
				const auto * spell = bonusValue.spell.toSpell();
				if (!spell->getIconScenarioBonus().empty())
					picName = spell->getIconScenarioBonus();
				else
					picNumber = bonusValue.spell.getNum();

				desc.appendLocalString(EMetaText::GENERAL_TXT, 716);
				desc.replaceName(bonusValue.spell);
				break;
			}
			case CampaignBonusType::PRIMARY_SKILL:
			{
				const auto & bonusValue = bonus.getValue<CampaignBonusPrimarySkill>();
				int leadingSkill = -1;
				std::vector<std::pair<int, int>> toPrint; //primary skills to be listed <num, val>
				for(int g = 0; g < bonusValue.amounts.size(); ++g)
				{
					if(leadingSkill == -1 || bonusValue.amounts[g] > bonusValue.amounts[leadingSkill])
					{
						leadingSkill = g;
					}
					if(bonusValue.amounts[g] != 0)
					{
						toPrint.push_back(std::make_pair(g, bonusValue.amounts[g]));
					}
				}
				picNumber = leadingSkill;
				desc.appendLocalString(EMetaText::GENERAL_TXT, 715);

				std::string substitute; //text to be printed instead of %s
				for(int v = 0; v < toPrint.size(); ++v)
				{
					substitute += std::to_string(toPrint[v].second);
					substitute += " " + LIBRARY->generaltexth->primarySkillNames[toPrint[v].first];
					if(v != toPrint.size() - 1)
					{
						substitute += ", ";
					}
				}

				desc.replaceRawString(substitute);
				break;
			}
			case CampaignBonusType::SECONDARY_SKILL:
			{
				const auto & bonusValue = bonus.getValue<CampaignBonusSecondarySkill>();
				const auto * skill = bonusValue.skill.toSkill();
				desc.appendLocalString(EMetaText::GENERAL_TXT, 718);
				desc.replaceTextID(TextIdentifier("core", "skilllev", bonusValue.mastery - 1).get());
				desc.replaceName(bonusValue.skill);
				if (!skill->at(bonusValue.mastery).scenarioBonus.empty())
					picName = skill->at(bonusValue.mastery).scenarioBonus.empty();
				else
					picNumber = bonusValue.skill.getNum() * 3 + bonusValue.mastery - 1;
				break;
			}
			case CampaignBonusType::RESOURCE:
			{
				const auto & bonusValue = bonus.getValue<CampaignBonusStartingResources>();
				desc.appendLocalString(EMetaText::GENERAL_TXT, 717);

				switch(bonusValue.resource)
				{
					case EGameResID::COMMON: //wood + ore
					{
						desc.replaceLocalString(EMetaText::GENERAL_TXT, 721);
						picNumber = 7;
						break;
					}
					case EGameResID::RARE: //mercury + sulfur + crystal + gems
					{
						desc.replaceLocalString(EMetaText::GENERAL_TXT, 722);
						picNumber = 8;
						break;
					}
					default:
					{
						desc.replaceName(bonusValue.resource);
						picNumber = bonusValue.resource.getNum();
					}
				}

				desc.replaceNumber(bonusValue.amount);
				break;
			}
			case CampaignBonusType::HEROES_FROM_PREVIOUS_SCENARIO:
			{
				const auto & bonusValue = bonus.getValue<CampaignBonusHeroesFromScenario>();
				auto superhero = getCampaign()->strongestHero(bonusValue.scenario, bonusValue.startingPlayer);
				if(!superhero)
					logGlobal->warn("No superhero! How could it be transferred?");
				picNumber = superhero ? superhero->getIconIndex() : 0;
				desc.appendLocalString(EMetaText::GENERAL_TXT, 719);
				desc.replaceRawString(getCampaign()->scenario(bonusValue.scenario).scenarioName.toString());
				break;
			}

			case CampaignBonusType::HERO:
			{
				const auto & bonusValue = bonus.getValue<CampaignBonusStartingHero>();
				if(bonusValue.hero == HeroTypeID::CAMP_RANDOM.getNum())
				{
					desc.appendLocalString(EMetaText::GENERAL_TXT, 720); // Start with random hero
					picNumber = -1;
					picName = "CBONN1A3.BMP";
				}
				else
				{
					desc.appendLocalString(EMetaText::GENERAL_TXT, 715); // Start with %s
					desc.replaceTextID(bonusValue.hero.toHeroType()->getNameTextID());
					picNumber = bonusValue.hero.getNum();
				}
				break;
			}
		}

		std::shared_ptr<CToggleButton> bonusButton = std::make_shared<CToggleButton>(Point(475 + i * 68, 455), AnimationPath::builtin("campaignBonusSelection"), CButton::tooltip(desc.toString(), desc.toString()), nullptr, EShortcut::NONE, false, [this](){
			if(buttonStart->isActive() && !buttonStart->isBlocked())	
				CBonusSelection::startMap();
		});

		if(picNumber != -1)
			bonusButton->setOverlay(std::make_shared<CAnimImage>(AnimationPath::builtin(picName), picNumber));
		else
			bonusButton->setOverlay(std::make_shared<CPicture>(ImagePath::builtin(picName)));

		if(GAME->server().campaignBonus == i)
			bonusButton->setBorderColor(Colors::BRIGHT_YELLOW);
		groupBonuses->addToggle(i, bonusButton);
	}

	if(getCampaign()->getBonusID(GAME->server().campaignMap))
		groupBonuses->setSelected(*getCampaign()->getBonusID(GAME->server().campaignMap));
}

void CBonusSelection::updateAfterStateChange()
{
	if(GAME->server().getState() != EClientState::GAMEPLAY)
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
	if(GAME->server().campaignBonus == -1)
	{
		buttonStart->block(getCampaign()->scenario(GAME->server().campaignMap).travelOptions.bonusesToChoose.size());
	}
	else
	{
		buttonStart->block(false);
	}

	for(auto region : regions)
		region->updateState();

	if(!GAME->server().mi)
		return;
	iconsMapSizes->setFrame(GAME->server().mi->getMapSizeIconId());
	mapName->setText(GAME->server().mi->getNameTranslated());
	mapDescription->setText(GAME->server().mi->getDescriptionTranslated());
	for(size_t i = 0; i < difficultyIcons.size(); i++)
	{
		if(i == GAME->server().si->difficulty)
		{
			difficultyIcons[i]->enable();
			difficultyIconAreas[i]->enable();

		}
		else
		{
			difficultyIcons[i]->disable();
			difficultyIconAreas[i]->disable();
		}
	}
	flagbox->recreate();
	createBonusesIcons();
}

void CBonusSelection::goBack()
{
	if(GAME->server().getState() != EClientState::GAMEPLAY)
	{
		ENGINE->windows().popWindows(2);
		GAME->mainmenu()->playMusic();
	}
	else
	{
		close();
		if(adventureInt)
			adventureInt->onAudioResumed();
	}
	// TODO: we can actually only pop bonus selection interface for custom campaigns
	// Though this would require clearing CLobbyScreen::bonusSel pointer when poping this interface
/*
	else
	{
		close();
		GAME->server().state = EClientState::LOBBY;
	}
*/
}

void CBonusSelection::startMap()
{
	if (!GAME->server().validateGameStart())
		return;

	auto showPrologVideo = [this]()
	{
		auto exitCb = []()
		{
			logGlobal->info("Starting scenario %d", GAME->server().campaignMap.getNum());
			GAME->server().sendStartGame();
		};

		const CampaignScenario & scenario = getCampaign()->scenario(GAME->server().campaignMap);
		if(scenario.prolog.hasPrologEpilog)
		{
			ENGINE->windows().createAndPushWindow<CPrologEpilogVideo>(scenario.prolog, exitCb);
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

	if(GAME->interface()) // we're currently ingame, so ask for starting new map and end game
	{
		close();
		GAME->interface()->showYesNoDialog(LIBRARY->generaltexth->allTexts[67], [=]()
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
	GAME->interface()->showYesNoDialog(
		LIBRARY->generaltexth->allTexts[67],
		[=]()
		{
			ENGINE->dispatchMainThread(
				[]()
				{
					GAME->server().sendRestartGame();
				}
			);
		},
		0
	);
}

void CBonusSelection::increaseDifficulty()
{
	GAME->server().setDifficulty(GAME->server().si->difficulty + 1);
}

void CBonusSelection::decreaseDifficulty()
{
	// avoid negative overflow (0 - 1 = 255)
	if (GAME->server().si->difficulty > 0)
		GAME->server().setDifficulty(GAME->server().si->difficulty - 1);
}

CBonusSelection::CRegion::CRegion(CampaignScenarioID id, bool accessible, bool selectable, bool labelOnly, const CampaignRegions & campDsc)
	: CIntObject(LCLICK | SHOW_POPUP | TIME), idOfMapAndRegion(id), accessible(accessible), selectable(selectable), labelOnly(labelOnly), blinkAnim({})
{
	OBJECT_CONSTRUCTION;

	pos += campDsc.getPosition(id);

	auto color = GAME->server().si->campState->scenario(idOfMapAndRegion).regionColor;

	graphicsNotSelected = std::make_shared<CPicture>(campDsc.getAvailableName(id, color));
	graphicsNotSelected->disable();
	graphicsSelected = std::make_shared<CPicture>(campDsc.getSelectedName(id, color));
	graphicsSelected->disable();
	graphicsStriped = std::make_shared<CPicture>(campDsc.getConqueredName(id, color));
	graphicsStriped->disable();
	pos.w = graphicsNotSelected->pos.w;
	pos.h = graphicsNotSelected->pos.h;

	auto labelPos = campDsc.getLabelPosition(id);
	if(labelPos)
	{
		auto mapHeader = GAME->server().si->campState->getMapHeader(idOfMapAndRegion);
		label = std::make_shared<CLabel>((*labelPos).x, (*labelPos).y, FONT_SMALL, ETextAlignment::CENTER, Colors::WHITE, mapHeader->name.toString());
	}
}

void CBonusSelection::CRegion::updateState(bool disableAll)
{
	if(labelOnly)
		return;

	if(disableAll)
	{
		graphicsNotSelected->disable();
		graphicsSelected->disable();
		graphicsStriped->disable();
	}
	else if(!accessible)
	{
		graphicsNotSelected->disable();
		graphicsSelected->disable();
		graphicsStriped->enable();
	}
	else if(GAME->server().campaignMap == idOfMapAndRegion)
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

void CBonusSelection::CRegion::tick(uint32_t msPassed)
{
	if(!accessible)
	{
		removeUsedEvents(TIME);
		return;
	}

	blinkAnim.msPassed += msPassed;
	if(blinkAnim.msPassed >= 150)
	{
		blinkAnim.state = !blinkAnim.state;
		blinkAnim.msPassed -= 150;
		if(blinkAnim.state)
			blinkAnim.count++;
		else if(blinkAnim.count >= 3)
			removeUsedEvents(TIME);
	}
	updateState(blinkAnim.state);
	setRedrawParent(true);
	redraw();
}

void CBonusSelection::CRegion::clickReleased(const Point & cursorPosition)
{
	if(!labelOnly && selectable && !graphicsNotSelected->getSurface()->isTransparent(cursorPosition - pos.topLeft()))
	{
		GAME->server().setCampaignMap(idOfMapAndRegion);
	}
}

void CBonusSelection::CRegion::showPopupWindow(const Point & cursorPosition)
{
	// FIXME: For some reason "down" is only ever contain indeterminate_value
	auto & text = GAME->server().si->campState->scenario(idOfMapAndRegion).regionText;
	if(!labelOnly && !graphicsNotSelected->getSurface()->isTransparent(cursorPosition - pos.topLeft()) && !text.empty())
	{
		CRClickPopup::createAndPush(text.toString());
	}
}
