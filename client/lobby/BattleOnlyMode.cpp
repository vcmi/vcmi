/*
 * BattleOnlyMode.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "BattleOnlyMode.h"

#include "../CServerHandler.h"
#include "../GameEngine.h"
#include "../GameInstance.h"

#include "../render/IRenderHandler.h"
#include "../render/CAnimation.h"
#include "../render/Canvas.h"
#include "../render/CanvasImage.h"
#include "../gui/Shortcut.h"
#include "../gui/WindowHandler.h"
#include "../widgets/Buttons.h"
#include "../widgets/GraphicalPrimitiveCanvas.h"
#include "../widgets/TextControls.h"
#include "../widgets/CTextInput.h"
#include "../widgets/Images.h"
#include "../windows/GUIClasses.h"
#include "../windows/CHeroOverview.h"
#include "../windows/CCreatureWindow.h"

#include "../../lib/GameLibrary.h"
#include "../../lib/gameState/CGameState.h"
#include "../../lib/networkPacks/PacksForLobby.h"
#include "../../lib/StartInfo.h"
#include "../../lib/VCMIDirs.h"
#include "../../lib/CRandomGenerator.h"
#include "../../lib/callback/EditorCallback.h"
#include "../../lib/entities/hero/CHero.h"
#include "../../lib/entities/hero/CHeroClass.h"
#include "../../lib/entities/hero/CHeroHandler.h"
#include "../../lib/entities/faction/CTown.h"
#include "../../lib/entities/faction/CTownHandler.h"
#include "../../lib/mapObjects/CGHeroInstance.h"
#include "../../lib/mapObjects/CGTownInstance.h"
#include "../../lib/mapObjectConstructors/AObjectTypeHandler.h"
#include "../../lib/mapObjectConstructors/CObjectClassesHandler.h"
#include "../../lib/mapping/CMap.h"
#include "../../lib/mapping/CMapInfo.h"
#include "../../lib/mapping/CMapEditManager.h"
#include "../../lib/mapping/CMapService.h"
#include "../../lib/mapping/MapFormat.h"
#include "../../lib/texts/CGeneralTextHandler.h"
#include "../../lib/texts/MetaString.h"
#include "../../lib/texts/TextOperations.h"
#include "../../lib/filesystem/Filesystem.h"

void BattleOnlyMode::openBattleWindow()
{
	GAME->server().sendGuiAction(LobbyGuiAction::BATTLE_MODE);
	ENGINE->windows().createAndPushWindow<BattleOnlyModeWindow>();
}

BattleOnlyModeWindow::BattleOnlyModeWindow()
	: CWindowObject(BORDERED)
	, startInfo(std::make_shared<BattleOnlyModeStartInfo>())
	, disabledColor(GAME->server().isHost() ? Colors::WHITE : Colors::ORANGE)
{
	OBJECT_CONSTRUCTION;

	pos.w = 519;
	pos.h = 238;

	updateShadow();
	center();

	init();

	backgroundTexture = std::make_shared<FilledTexturePlayerColored>(Rect(0, 0, pos.w, pos.h));
	backgroundTexture->setPlayerColor(PlayerColor(1));
	buttonOk = std::make_shared<CButton>(Point(191, 203), AnimationPath::builtin("MuBchck"), CButton::tooltip(), [this](){ startBattle(); }, EShortcut::GLOBAL_ACCEPT);
	buttonOk->block(true);
	buttonAbort = std::make_shared<CButton>(Point(265, 203), AnimationPath::builtin("MuBcanc"), CButton::tooltip(), [this](){
		GAME->server().sendGuiAction(LobbyGuiAction::NO_TAB);
		close();
	}, EShortcut::GLOBAL_CANCEL);
	buttonAbort->block(true);
	title = std::make_shared<CLabel>(260, 20, FONT_BIG, ETextAlignment::CENTER, Colors::YELLOW, LIBRARY->generaltexth->translate("vcmi.lobby.battleOnlyMode"));

	battlefieldSelector = std::make_shared<CButton>(Point(29, 174), AnimationPath::builtin("GSPButtonClear"), CButton::tooltip(), [this](){
		std::vector<std::string> texts;
		std::vector<std::shared_ptr<IImage>> images;

		auto & terrains = LIBRARY->terrainTypeHandler->objects;
		for (const auto & terrain : terrains)
		{
			texts.push_back(terrain->getNameTranslated());

			const auto & patterns = LIBRARY->terviewh->getTerrainViewPatterns(terrain->getId());
			TerrainViewPattern pattern;
			for(auto & p : patterns)
				if(p[0].id == "n1")
					pattern = p[0];
			auto image = ENGINE->renderHandler().loadImage(terrain->tilesFilename, pattern.mapping[0].first, 0, EImageBlitMode::OPAQUE);
			image->scaleTo(Point(23, 23), EScalingAlgorithm::NEAREST);
			images.push_back(image);
		}

		auto factions = LIBRARY->townh->getDefaultAllowed();
		for (const auto & faction : factions)
		{
			texts.push_back(faction.toFaction()->getNameTranslated());

			auto image = ENGINE->renderHandler().loadImage(AnimationPath::builtin("ITPA"), faction.toFaction()->town->clientInfo.icons[true][false] + 2, 0, EImageBlitMode::OPAQUE);
			image->scaleTo(Point(35, 23), EScalingAlgorithm::NEAREST);
			images.push_back(image);
		}

		ENGINE->windows().createAndPushWindow<CObjectListWindow>(texts, nullptr, LIBRARY->generaltexth->translate("vcmi.lobby.battleOnlyModeBattlefield"), LIBRARY->generaltexth->translate("vcmi.lobby.battleOnlyModeBattlefieldSelect"), [this, terrains, factions](int index){
			if(terrains.size() > index)
			{
				startInfo->selectedTerrain = terrains[index]->getId();
				startInfo->selectedTown = std::nullopt;
			}
			else
			{
				startInfo->selectedTerrain = std::nullopt;
				auto it = std::next(factions.begin(), index - terrains.size());
				if (it != factions.end())
    				startInfo->selectedTown = *it;
			}
			onChange();
		}, (startInfo->selectedTerrain ? static_cast<int>(*startInfo->selectedTerrain) : static_cast<int>(*startInfo->selectedTown + terrains.size())), images, true, true);
	});
	battlefieldSelector->block(GAME->server().isGuest());
	buttonReset = std::make_shared<CButton>(Point(289, 174), AnimationPath::builtin("GSPButtonClear"), CButton::tooltip(), [this](){
		if(GAME->server().isHost())
		{
			startInfo->selectedTerrain = TerrainId::DIRT;
			startInfo->selectedTown = std::nullopt;
			startInfo->selectedHero[0] = std::nullopt;
			startInfo->selectedArmy[0].fill(std::nullopt);
			for(size_t i=0; i<GameConstants::ARMY_SIZE; i++)
				heroSelector1->selectedArmyInput.at(i)->setText("1");
		}
		startInfo->selectedHero[1] = std::nullopt;
		startInfo->selectedArmy[1].fill(std::nullopt);
		for(size_t i=0; i<GameConstants::ARMY_SIZE; i++)
			heroSelector2->selectedArmyInput.at(i)->setText("1");
		onChange();
	});
	buttonReset->setTextOverlay(LIBRARY->generaltexth->translate("vcmi.lobby.battleOnlyModeReset"), EFonts::FONT_SMALL, Colors::WHITE);

	heroSelector1 = std::make_shared<BattleOnlyModeHeroSelector>(0, *this, Point(0, 40));
	heroSelector2 = std::make_shared<BattleOnlyModeHeroSelector>(1, *this, Point(260, 40));

	heroSelector1->setInputEnabled(GAME->server().isHost());

	onChange();
}

void BattleOnlyModeWindow::init()
{
	map = std::make_unique<CMap>(nullptr);
	map->version = EMapFormat::VCMI;
	map->creationDateTime = std::time(nullptr);
	map->width = 10;
	map->height = 10;
	map->mapLevels = 1;
	map->battleOnly = true;
	map->name = MetaString::createFromTextID("vcmi.lobby.battleOnlyMode");

	cb = std::make_unique<EditorCallback>(map.get());
}

void BattleOnlyModeWindow::onChange()
{
	GAME->server().setBattleOnlyModeStartInfo(startInfo);
}

void BattleOnlyModeWindow::update()
{
	setTerrainButtonText();
	setOkButtonEnabled();
	
	heroSelector1->setHeroIcon();
	heroSelector1->setCreatureIcons();
	heroSelector2->setHeroIcon();
	heroSelector2->setCreatureIcons();
	redraw();
}

void BattleOnlyModeWindow::applyStartInfo(std::shared_ptr<BattleOnlyModeStartInfo> si)
{
	startInfo = si;
	update();
}

void BattleOnlyModeWindow::setTerrainButtonText()
{
	battlefieldSelector->setTextOverlay(LIBRARY->generaltexth->translate("vcmi.lobby.battleOnlyModeBattlefield") + ":   " + (startInfo->selectedTerrain ? (*startInfo->selectedTerrain).toEntity(LIBRARY)->getNameTranslated() : (*startInfo->selectedTown).toEntity(LIBRARY)->getNameTranslated()), EFonts::FONT_SMALL, disabledColor);
}

void BattleOnlyModeWindow::setOkButtonEnabled()
{
	bool army2Empty = std::all_of(startInfo->selectedArmy[1].begin(), startInfo->selectedArmy[1].end(), [](const auto x) { return !x; });

	bool canStart = (startInfo->selectedTerrain || startInfo->selectedTown);
	canStart &= (startInfo->selectedHero[0] && ((startInfo->selectedHero[1]) || (startInfo->selectedTown && !army2Empty)));
	buttonOk->block(!canStart || GAME->server().isGuest());
	buttonAbort->block(GAME->server().isGuest());
}

std::shared_ptr<IImage> drawBlackBox(Point size, std::string text, ColorRGBA color)
{
	auto image = ENGINE->renderHandler().createImage(size, CanvasScalingPolicy::AUTO);
	Canvas canvas = image->getCanvas();
	canvas.drawColor(Rect(0, 0, size.x, size.y), Colors::BLACK);
	canvas.drawText(Point(size.x / 2, size.y / 2), FONT_TINY, color, ETextAlignment::CENTER, text);
	return image;
}

BattleOnlyModeHeroSelector::BattleOnlyModeHeroSelector(int id, BattleOnlyModeWindow& p, Point position)
: parent(p)
, id(id)
{
	OBJECT_CONSTRUCTION;

	pos.x += position.x;
	pos.y += position.y;

	backgroundImage = std::make_shared<CPicture>(ImagePath::builtin("heroSlotsBlue"), Point(3, 4));

	for(size_t i=0; i<GameConstants::PRIMARY_SKILLS; i++)
	{
		auto image = std::make_shared<CAnimImage>(AnimationPath::builtin("PSKIL32"), i, 0, 78 + i * 36, 26);
		primSkills.push_back(image);
		primSkillsBorder.push_back(std::make_shared<GraphicalPrimitiveCanvas>(Rect(78 + i * 36, 26, 32, 32)));
		primSkillsBorder.back()->addRectangle(Point(0, 0), Point(32, 32), ColorRGBA(44, 108, 255));
		primSkillsInput.push_back(std::make_shared<CTextInput>(Rect(78 + i * 36, 58, 32, 16), EFonts::FONT_SMALL, ETextAlignment::CENTER, false));
		primSkillsInput.back()->setColor(id == 1 ? Colors::WHITE : parent.disabledColor);
		primSkillsInput.back()->setFilterNumber(0, 100);
		primSkillsInput.back()->setText("0");
		primSkillsInput.back()->setCallback([this, i, id](const std::string & text){
			parent.startInfo->primSkillLevel[id][i] = std::stoi(primSkillsInput[i]->getText());
			parent.onChange();
		});
	}

	creatureImage.resize(GameConstants::ARMY_SIZE);
	for(size_t i=0; i<GameConstants::ARMY_SIZE; i++)
	{
		selectedArmyInput.push_back(std::make_shared<CTextInput>(Rect(5 + i * 36, 113, 32, 16), EFonts::FONT_SMALL, ETextAlignment::CENTER, false));
		selectedArmyInput.back()->setColor(id == 1 ? Colors::WHITE : parent.disabledColor);
		selectedArmyInput.back()->setFilterNumber(1, 10000000, 3);
		selectedArmyInput.back()->setText("1");
		selectedArmyInput.back()->setCallback([this, i, id](const std::string & text){
			if(parent.startInfo->selectedArmy[id][i])
			{
				(*parent.startInfo->selectedArmy[id][i]).second = TextOperations::parseMetric<int>(text);
				parent.onChange();
			}
			else
				selectedArmyInput[i]->setText("1");
		});
	}

	setHeroIcon();
	setCreatureIcons();
}

void BattleOnlyModeHeroSelector::setHeroIcon()
{
	OBJECT_CONSTRUCTION;

	if(!parent.startInfo->selectedHero[id])
	{
		heroImage = std::make_shared<CPicture>(drawBlackBox(Point(58, 64), LIBRARY->generaltexth->translate("vcmi.lobby.battleOnlyModeSelect"), id == 1 ? Colors::WHITE : parent.disabledColor), Point(6, 7));
		heroLabel = std::make_shared<CLabel>(160, 16, FONT_SMALL, ETextAlignment::CENTER, id == 1 ? Colors::WHITE : parent.disabledColor, LIBRARY->generaltexth->translate("core.genrltxt.507"));
		for(size_t i=0; i<GameConstants::PRIMARY_SKILLS; i++)
			primSkillsInput[i]->setText("0");
	}
	else
	{
		heroImage = std::make_shared<CPicture>(ENGINE->renderHandler().loadAnimation(AnimationPath::builtin("PortraitsLarge"), EImageBlitMode::COLORKEY)->getImage((*parent.startInfo->selectedHero[id]).toHeroType()->imageIndex), Point(6, 7));
		heroLabel = std::make_shared<CLabel>(160, 16, FONT_SMALL, ETextAlignment::CENTER, id == 1 ? Colors::WHITE : parent.disabledColor, (*parent.startInfo->selectedHero[id]).toHeroType()->getNameTranslated());
		for(size_t i=0; i<GameConstants::PRIMARY_SKILLS; i++)
			primSkillsInput[i]->setText(std::to_string(parent.startInfo->primSkillLevel[id][i]));
	}

	heroImage->addLClickCallback([this](){
		auto allowedSet = LIBRARY->heroh->getDefaultAllowed();
		std::vector<HeroTypeID> heroes(allowedSet.begin(), allowedSet.end());
		std::sort(heroes.begin(), heroes.end(), [](auto a, auto b) {
			auto heroA = a.toHeroType();
			auto heroB = b.toHeroType();
			if (heroA->heroClass->getId() == heroB->heroClass->getId())
				return heroA->getNameTranslated() < heroB->getNameTranslated();
			if (heroA->heroClass->faction == heroB->heroClass->faction)
				return heroA->heroClass->getId() < heroB->heroClass->getId();
			return heroA->heroClass->faction < heroB->heroClass->faction;
		});

		int selectedIndex = !parent.startInfo->selectedHero[id] ? 0 : (1 + std::distance(heroes.begin(), std::find_if(heroes.begin(), heroes.end(), [this](auto heroID) {
			return heroID == (*parent.startInfo->selectedHero[id]);
    	})));
		
		std::vector<std::string> texts;
		std::vector<std::shared_ptr<IImage>> images;
		// Add "no hero" option
		texts.push_back(LIBRARY->generaltexth->translate("core.genrltxt.507"));
		images.push_back(nullptr);
		for (const auto & h : heroes)
		{
			texts.push_back(h.toHeroType()->getNameTranslated());

			auto image = ENGINE->renderHandler().loadImage(AnimationPath::builtin("PortraitsSmall"), h.toHeroType()->imageIndex, 0, EImageBlitMode::OPAQUE);
			image->scaleTo(Point(35, 23), EScalingAlgorithm::NEAREST);
			images.push_back(image);
		}
		auto window = std::make_shared<CObjectListWindow>(texts, nullptr, LIBRARY->generaltexth->translate("object.core.hero.name"), LIBRARY->generaltexth->translate("vcmi.lobby.battleOnlyModeHeroSelect"), [this, heroes](int index){
			if(index == 0)
			{
				parent.startInfo->selectedHero[id] = std::nullopt;
				parent.onChange();
				return;
			}
			index--;

			parent.startInfo->selectedHero[id] = heroes[index];

			for(size_t i=0; i<GameConstants::PRIMARY_SKILLS; i++)
				parent.startInfo->primSkillLevel[id][i] = 0;
			parent.onChange();
		}, selectedIndex, images, true, true);
		window->onPopup = [heroes](int index) {
			if(index == 0)
				return;
			index--;

			ENGINE->windows().createAndPushWindow<CHeroOverview>(heroes.at(index));
		};
		ENGINE->windows().pushWindow(window);
	});

	heroImage->addRClickCallback([this](){
		if(!parent.startInfo->selectedHero[id])
			return;
		
		ENGINE->windows().createAndPushWindow<CHeroOverview>(parent.startInfo->selectedHero[id]->toHeroType()->getId());
	});
}

void BattleOnlyModeHeroSelector::setCreatureIcons()
{
	OBJECT_CONSTRUCTION;

	for(int i = 0; i < creatureImage.size(); i++)
	{
		if(!parent.startInfo->selectedArmy[id][i])
		{
			creatureImage[i] = std::make_shared<CPicture>(drawBlackBox(Point(32, 32), LIBRARY->generaltexth->translate("vcmi.lobby.battleOnlyModeSelect"), id == 1 ? Colors::WHITE : parent.disabledColor), Point(6 + i * 36, 78));
			selectedArmyInput[i]->setText("1");
		}
		else
		{
			auto unit = (*parent.startInfo->selectedArmy[id][i]);
			auto creatureID = unit.first;
			creatureImage[i] = std::make_shared<CPicture>(ENGINE->renderHandler().loadAnimation(AnimationPath::builtin("CPRSMALL"), EImageBlitMode::COLORKEY)->getImage(LIBRARY->creh->objects.at(creatureID)->getIconIndex()), Point(6 + i * 36, 78));
			selectedArmyInput[i]->setText(TextOperations::formatMetric(unit.second, 3));
		}

		creatureImage[i]->addLClickCallback([this, i](){
			auto allowedSet = LIBRARY->creh->getDefaultAllowed();
			std::vector<CreatureID> creatures(allowedSet.begin(), allowedSet.end());
			std::sort(creatures.begin(), creatures.end(), [](auto a, auto b) {
				auto creatureA = a.toCreature();
				auto creatureB = b.toCreature();
				if (creatureA->getLevel() == creatureB->getLevel())
					return creatureA->getNameSingularTranslated() < creatureB->getNameSingularTranslated();
				if (creatureA->getFactionID() == creatureB->getFactionID())
					return creatureA->getLevel() < creatureB->getLevel();
				return creatureA->getFactionID() < creatureB->getFactionID();
			});

			int selectedIndex = !parent.startInfo->selectedArmy[id][i] ? 0 : (1 + std::distance(creatures.begin(), std::find_if(creatures.begin(), creatures.end(), [this, i](auto creatureID) {
				return creatureID == (*parent.startInfo->selectedArmy[id][i]).first;
			})));
			
			std::vector<std::string> texts;
			std::vector<std::shared_ptr<IImage>> images;
			// Add "no creature" option
			texts.push_back(LIBRARY->generaltexth->translate("core.genrltxt.507"));
			images.push_back(nullptr);
			for (const auto & c : creatures)
			{
				texts.push_back(c.toCreature()->getNameSingularTranslated());

				auto image = ENGINE->renderHandler().loadImage(AnimationPath::builtin("CPRSMALL"), c.toCreature()->getIconIndex(), 0, EImageBlitMode::OPAQUE);
				image->scaleTo(Point(23, 23), EScalingAlgorithm::NEAREST);
				images.push_back(image);
			}
			auto window = std::make_shared<CObjectListWindow>(texts, nullptr, LIBRARY->generaltexth->translate("core.genrltxt.42"), LIBRARY->generaltexth->translate("vcmi.lobby.battleOnlyModeCreatureSelect"), [this, creatures, i](int index){
				if(index == 0)
				{
					if(parent.startInfo->selectedArmy[id][i])
						parent.startInfo->selectedArmy[id][i] = std::nullopt;
					parent.onChange();
					return;
				}
				index--;

				auto creature = creatures.at(index).toCreature();
				parent.startInfo->selectedArmy[id][i] = std::make_pair(creature->getId(), 100);
				parent.onChange();
			}, selectedIndex, images, true, true);
			window->onPopup = [creatures](int index) {
				if(index == 0)
					return;
				index--;

				ENGINE->windows().createAndPushWindow<CStackWindow>(creatures.at(index).toCreature(), true);
			};
			ENGINE->windows().pushWindow(window);
		});

		creatureImage[i]->addRClickCallback([this, i](){
			if(!parent.startInfo->selectedArmy[id][i])
				return;
			
			ENGINE->windows().createAndPushWindow<CStackWindow>(LIBRARY->creh->objects.at((*parent.startInfo->selectedArmy[id][i]).first).get(), true);
		});
	}
}

void BattleOnlyModeWindow::startBattle()
{
	auto rng = &CRandomGenerator::getDefault();
	
	map->initTerrain();
	map->getEditManager()->clearTerrain(rng);

	map->getEditManager()->getTerrainSelection().selectAll();
	map->getEditManager()->drawTerrain(!startInfo->selectedTerrain ? TerrainId::DIRT : *startInfo->selectedTerrain, 0, rng);

	map->players[0].canComputerPlay = true;
	map->players[0].canHumanPlay = true;
	map->players[1] = map->players[0];

	auto knownHeroes = LIBRARY->objtypeh->knownSubObjects(Obj::HERO);

	auto addHero = [&, this](int sel, PlayerColor color, const int3 & position)
	{
		auto factory = LIBRARY->objtypeh->getHandlerFor(Obj::HERO, (*startInfo->selectedHero[sel]).toHeroType()->heroClass->getId());
		auto templates = factory->getTemplates();
		auto obj = std::dynamic_pointer_cast<CGHeroInstance>(factory->create(cb.get(), templates.front()));
		obj->setHeroType(*startInfo->selectedHero[sel]);

		obj->setOwner(color);
		obj->pos = position;
		for(size_t i=0; i<GameConstants::PRIMARY_SKILLS; i++)
			obj->pushPrimSkill(PrimarySkill(i), startInfo->primSkillLevel[sel][i]);
		obj->clearSlots();
		for(int slot = 0; slot < GameConstants::ARMY_SIZE; slot++)
			if(startInfo->selectedArmy[sel][slot])
				obj->setCreature(SlotID(slot), (*startInfo->selectedArmy[sel][slot]).first, (*startInfo->selectedArmy[sel][slot]).second);
		map->getEditManager()->insertObject(obj);
	};

	addHero(0, PlayerColor(0), int3(5, 6, 0));
	if(!startInfo->selectedTown)
		addHero(1, PlayerColor(1), int3(5, 5, 0));
	else
	{
		auto factory = LIBRARY->objtypeh->getHandlerFor(Obj::TOWN, *startInfo->selectedTown);
		auto templates = factory->getTemplates();
		auto obj = factory->create(cb.get(), templates.front());
		auto townObj = std::dynamic_pointer_cast<CGTownInstance>(obj);
		obj->setOwner(PlayerColor(1));
		obj->pos = int3(5, 5, 0);
		for (const auto & building : townObj->getTown()->getAllBuildings())
			townObj->addBuilding(building);
		if(!startInfo->selectedHero[1])
		{
			for(int slot = 0; slot < GameConstants::ARMY_SIZE; slot++)
				if(startInfo->selectedArmy[1][slot])
					townObj->getArmy()->setCreature(SlotID(slot), (*startInfo->selectedArmy[1][slot]).first, (*startInfo->selectedArmy[1][slot]).second);
		}
		else
			addHero(1, PlayerColor(1), int3(5, 5, 0));

		map->getEditManager()->insertObject(townObj);
	}

	auto path = VCMIDirs::get().userDataPath() / "Maps";
	boost::filesystem::create_directories(path);
	const std::string fileName = "BattleOnlyMode.vmap";
	const auto fullPath = path / fileName;
	CMapService mapService;
	mapService.saveMap(map, fullPath);
	CResourceHandler::get()->updateFilteredFiles([&](const std::string & mount) { return true; });

	auto mapInfo = std::make_shared<CMapInfo>();
	mapInfo->mapInit("Maps/BattleOnlyMode");
	GAME->server().setMapInfo(mapInfo);
	ExtraOptionsInfo extraOptions;
	extraOptions.unlimitedReplay = true;
	GAME->server().setExtraOptionsInfo(extraOptions);
	GAME->server().sendStartGame();
}
