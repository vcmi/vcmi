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
#include "../../lib/texts/TextOperations.h"
#include "../../lib/filesystem/Filesystem.h"

void BattleOnlyMode::openBattleWindow()
{
	ENGINE->windows().createAndPushWindow<BattleOnlyModeWindow>();
}

BattleOnlyModeWindow::BattleOnlyModeWindow()
	: CWindowObject(BORDERED)
	, selectedTerrain(TerrainId::DIRT)
	, selectedTown(FactionID::NONE)
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
	buttonAbort = std::make_shared<CButton>(Point(265, 203), AnimationPath::builtin("MuBcanc"), CButton::tooltip(), [this](){ close(); }, EShortcut::GLOBAL_CANCEL);
	title = std::make_shared<CLabel>(260, 20, FONT_BIG, ETextAlignment::CENTER, Colors::YELLOW, LIBRARY->generaltexth->translate("vcmi.lobby.battleOnlyMode"));

	battlegroundSelector = std::make_shared<CButton>(Point(29, 174), AnimationPath::builtin("GSPButtonClear"), CButton::tooltip(), [this](){
		std::vector<std::string> texts;
		std::vector<std::shared_ptr<IImage>> images;

		auto terrains = LIBRARY->terrainTypeHandler->objects;
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

		auto factions = LIBRARY->townh->objects;
		factions.erase(std::remove_if(factions.begin(), factions.end(), [](const std::shared_ptr<CFaction>& n) {
			return !n->town;
		}), factions.end());
		for (const auto & faction : factions)
		{
			texts.push_back(faction->getNameTranslated());

			auto image = ENGINE->renderHandler().loadImage(AnimationPath::builtin("ITPA"), faction->town->clientInfo.icons[true][false] + 2, 0, EImageBlitMode::OPAQUE);
			image->scaleTo(Point(35, 23), EScalingAlgorithm::NEAREST);
			images.push_back(image);
		}

		ENGINE->windows().createAndPushWindow<CObjectListWindow>(texts, nullptr, LIBRARY->generaltexth->translate("vcmi.lobby.battleOnlyModeBattleground"), LIBRARY->generaltexth->translate("vcmi.lobby.battleOnlyModeBattlegroundSelect"), [this, terrains, factions](int index){
			if(terrains.size() > index)
			{
				selectedTerrain = terrains[index]->getId();
				selectedTown = FactionID::NONE;
			}
			else
			{
				selectedTerrain = TerrainId::NONE;
				selectedTown = factions[index - terrains.size()]->getId();
			}
			setTerrainButtonText();
			setOkButtonEnabled();
		}, (selectedTerrain != TerrainId::NONE ? static_cast<int>(selectedTerrain) : static_cast<int>(selectedTown + terrains.size())), images, true, true);
	});
	buttonReset = std::make_shared<CButton>(Point(289, 174), AnimationPath::builtin("GSPButtonClear"), CButton::tooltip(), [this](){
		selectedTerrain = TerrainId::DIRT;
		selectedTown = FactionID::NONE;
		setTerrainButtonText();
		setOkButtonEnabled();
		heroSelector1->selectedHero.reset();
		heroSelector2->selectedHero.reset();
		heroSelector1->selectedArmy->clearSlots();
		heroSelector2->selectedArmy->clearSlots();
		for(size_t i=0; i<GameConstants::ARMY_SIZE; i++)
		{
			heroSelector1->selectedArmyInput.at(i)->setText("0");
			heroSelector2->selectedArmyInput.at(i)->setText("0");
		}
		heroSelector1->setHeroIcon();
	    heroSelector1->setCreatureIcons();
		heroSelector2->setHeroIcon();
	    heroSelector2->setCreatureIcons();
		redraw();
	});
	buttonReset->setTextOverlay(LIBRARY->generaltexth->translate("vcmi.lobby.battleOnlyModeReset"), EFonts::FONT_SMALL, Colors::WHITE);

	setTerrainButtonText();
	setOkButtonEnabled();

	heroSelector1 = std::make_shared<BattleOnlyModeHeroSelector>(*this, Point(0, 40));
	heroSelector2 = std::make_shared<BattleOnlyModeHeroSelector>(*this, Point(260, 40));
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

	cb = std::make_unique<EditorCallback>(map.get());
}

void BattleOnlyModeWindow::setTerrainButtonText()
{
	battlegroundSelector->setTextOverlay(LIBRARY->generaltexth->translate("vcmi.lobby.battleOnlyModeBattleground") + ":   " + (selectedTerrain != TerrainId::NONE ? selectedTerrain.toEntity(LIBRARY)->getNameTranslated() : selectedTown.toEntity(LIBRARY)->getNameTranslated()), EFonts::FONT_SMALL, Colors::WHITE);
}

void BattleOnlyModeWindow::setOkButtonEnabled()
{
	bool canStart = (selectedTerrain != TerrainId::NONE || selectedTown != FactionID::NONE);
	canStart &= (heroSelector1 && heroSelector1->selectedHero && ((heroSelector2 && heroSelector2->selectedHero) || (selectedTown != FactionID::NONE && heroSelector2->selectedArmy->stacksCount())));
	buttonOk->block(!canStart);
}

std::shared_ptr<IImage> drawBlackBox(Point size, std::string text)
{
	auto image = ENGINE->renderHandler().createImage(size, CanvasScalingPolicy::AUTO);
	Canvas canvas = image->getCanvas();
	canvas.drawColor(Rect(0, 0, size.x, size.y), Colors::BLACK);
	canvas.drawText(Point(size.x / 2, size.y / 2), FONT_TINY, Colors::WHITE, ETextAlignment::CENTER, text);
	return image;
}

BattleOnlyModeHeroSelector::BattleOnlyModeHeroSelector(BattleOnlyModeWindow& parent, Point position)
: parent(parent)
, selectedArmy(std::make_shared<CCreatureSet>())
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
		primSkillsInput.back()->setFilterNumber(0, 100);
		primSkillsInput.back()->setText("0");
	}

	creatureImage.resize(GameConstants::ARMY_SIZE);
	for(size_t i=0; i<GameConstants::ARMY_SIZE; i++)
	{
		selectedArmyInput.push_back(std::make_shared<CTextInput>(Rect(5 + i * 36, 113, 32, 16), EFonts::FONT_SMALL, ETextAlignment::CENTER, false));
		selectedArmyInput.back()->setFilterNumber(0, 10000000, 3);
		selectedArmyInput.back()->setText("0");
	}

	setHeroIcon();
	setCreatureIcons();
}

void BattleOnlyModeHeroSelector::setHeroIcon()
{
	OBJECT_CONSTRUCTION;

	if(!selectedHero)
	{
		heroImage = std::make_shared<CPicture>(drawBlackBox(Point(58, 64), LIBRARY->generaltexth->translate("vcmi.lobby.battleOnlyModeSelect")), Point(6, 7));
		heroLabel = std::make_shared<CLabel>(160, 16, FONT_SMALL, ETextAlignment::CENTER, Colors::WHITE, LIBRARY->generaltexth->translate("core.genrltxt.507"));
	}
	else
	{
		heroImage = std::make_shared<CPicture>(ENGINE->renderHandler().loadAnimation(AnimationPath::builtin("PortraitsLarge"), EImageBlitMode::COLORKEY)->getImage(selectedHero->getHeroType()->imageIndex), Point(6, 7));
		heroLabel = std::make_shared<CLabel>(160, 16, FONT_SMALL, ETextAlignment::CENTER, Colors::WHITE, selectedHero->getNameTranslated());
	}

	heroImage->addLClickCallback([this](){
		auto heroes = LIBRARY->heroh->objects;
		std::sort(heroes.begin(), heroes.end(), [](const std::shared_ptr<CHero>& a, const std::shared_ptr<CHero>& b) {
			if (a->heroClass->getId() == b->heroClass->getId())
				return a->getNameTranslated() < b->getNameTranslated();
			if (a->heroClass->faction == b->heroClass->faction)
				return a->heroClass->getId() < b->heroClass->getId();
			return a->heroClass->faction < b->heroClass->faction;
		});
		heroes.erase(std::remove_if(heroes.begin(), heroes.end(), [](const std::shared_ptr<CHero>& n) {
			return n->special;
		}), heroes.end());

		int selectedIndex = !selectedHero ? 0 : (1 + std::distance(heroes.begin(), std::find_if(heroes.begin(), heroes.end(), [this](const std::shared_ptr<CHero>& heroPtr) {
			return heroPtr.get() == selectedHero->getHeroType();
    	})));
		
		std::vector<std::string> texts;
		std::vector<std::shared_ptr<IImage>> images;
		// Add "no hero" option
		texts.push_back(LIBRARY->generaltexth->translate("core.genrltxt.507"));
		images.push_back(nullptr);
		for (const auto & h : heroes)
		{
			texts.push_back(h->getNameTranslated());

			auto image = ENGINE->renderHandler().loadImage(AnimationPath::builtin("PortraitsSmall"), h->imageIndex, 0, EImageBlitMode::OPAQUE);
			image->scaleTo(Point(35, 23), EScalingAlgorithm::NEAREST);
			images.push_back(image);
		}
		auto window = std::make_shared<CObjectListWindow>(texts, nullptr, LIBRARY->generaltexth->translate("object.core.hero.name"), LIBRARY->generaltexth->translate("vcmi.lobby.battleOnlyModeHeroSelect"), [this, heroes](int index){
			if(index == 0)
			{
				selectedHero.reset();
				setHeroIcon();
				return;
			}
			index--;
			auto hero = heroes[index];

			auto factory = LIBRARY->objtypeh->getHandlerFor(Obj::HERO, hero->heroClass->getId());
			auto templates = factory->getTemplates();
			auto obj = std::dynamic_pointer_cast<CGHeroInstance>(factory->create(parent.cb.get(), templates.front()));
			obj->setHeroType(hero->getId());

			selectedHero = obj;
			setHeroIcon();
			parent.setOkButtonEnabled();
		}, selectedIndex, images, true, true);
		window->onPopup = [heroes](int index) {
			if(index == 0)
				return;
			index--;

			ENGINE->windows().createAndPushWindow<CHeroOverview>(heroes[index]->getId());
		};
		ENGINE->windows().pushWindow(window);
	});

	heroImage->addRClickCallback([this](){
		if(!selectedHero)
			return;
		
		ENGINE->windows().createAndPushWindow<CHeroOverview>(selectedHero->getHeroType()->getId());
	});
}

void BattleOnlyModeHeroSelector::setCreatureIcons()
{
	OBJECT_CONSTRUCTION;

	for(int i = 0; i < creatureImage.size(); i++)
	{
		if(selectedArmy->slotEmpty(SlotID(i)))
			creatureImage[i] = std::make_shared<CPicture>(drawBlackBox(Point(32, 32), LIBRARY->generaltexth->translate("vcmi.lobby.battleOnlyModeSelect")), Point(6 + i * 36, 78));
		else
		{
			auto creatureID = selectedArmy->Slots().at(SlotID(i))->getCreatureID();
			creatureImage[i] = std::make_shared<CPicture>(ENGINE->renderHandler().loadAnimation(AnimationPath::builtin("CPRSMALL"), EImageBlitMode::COLORKEY)->getImage(LIBRARY->creh->objects.at(creatureID)->getIconIndex()), Point(6 + i * 36, 78));
		}

		creatureImage[i]->addLClickCallback([this, i](){
			auto creatures = LIBRARY->creh->objects;
			std::sort(creatures.begin(), creatures.end(), [](const std::shared_ptr<CCreature>& a, const std::shared_ptr<CCreature>& b) {
				if (a->getLevel() == b->getLevel())
					return a->getNameSingularTranslated() < b->getNameSingularTranslated();
				if (a->getFactionID() == b->getFactionID())
					return a->getLevel() < b->getLevel();
				return a->getFactionID() < b->getFactionID();
			});
			creatures.erase(std::remove_if(creatures.begin(), creatures.end(), [](const std::shared_ptr<CCreature>& n) {
				return n->special;
			}), creatures.end());

			int selectedIndex = selectedArmy->slotEmpty(SlotID(i)) ? 0 : (1 + std::distance(creatures.begin(), std::find_if(creatures.begin(), creatures.end(), [this, i](const std::shared_ptr<CCreature>& creaturePtr) {
				return creaturePtr->getId() == selectedArmy->Slots().at(SlotID(i))->getId();
			})));
			
			std::vector<std::string> texts;
			std::vector<std::shared_ptr<IImage>> images;
			// Add "no creature" option
			texts.push_back(LIBRARY->generaltexth->translate("core.genrltxt.507"));
			images.push_back(nullptr);
			for (const auto & c : creatures)
			{
				texts.push_back(c->getNameSingularTranslated());

				auto image = ENGINE->renderHandler().loadImage(AnimationPath::builtin("CPRSMALL"), c->getIconIndex(), 0, EImageBlitMode::OPAQUE);
				image->scaleTo(Point(23, 23), EScalingAlgorithm::NEAREST);
				images.push_back(image);
			}
			auto window = std::make_shared<CObjectListWindow>(texts, nullptr, LIBRARY->generaltexth->translate("core.genrltxt.42"), LIBRARY->generaltexth->translate("vcmi.lobby.battleOnlyModeCreatureSelect"), [this, creatures, i](int index){
				if(index == 0)
				{
					if(!selectedArmy->slotEmpty(SlotID(i)))
						selectedArmy->eraseStack(SlotID(i));
					setCreatureIcons();
					return;
				}
				index--;
				auto creature = creatures[index];
				selectedArmy->setCreature(SlotID(i), creature->getId(), 100);
				selectedArmyInput[SlotID(i)]->setText("100");
				setCreatureIcons();
			}, selectedIndex, images, true, true);
			window->onPopup = [creatures](int index) {
				if(index == 0)
					return;
				index--;

				ENGINE->windows().createAndPushWindow<CStackWindow>(creatures.at(index).get(), true);
			};
			ENGINE->windows().pushWindow(window);
		});

		creatureImage[i]->addRClickCallback([this, i](){
			if(selectedArmy->slotEmpty(SlotID(i)))
				return;
			
			ENGINE->windows().createAndPushWindow<CStackWindow>(LIBRARY->creh->objects.at(selectedArmy->Slots().at(SlotID(i))->getCreatureID()).get(), true);
		});
	}
}

void BattleOnlyModeWindow::startBattle()
{
	auto rng = &CRandomGenerator::getDefault();
	
	map->initTerrain();
	map->getEditManager()->clearTerrain(rng);

	map->getEditManager()->getTerrainSelection().selectAll();
	map->getEditManager()->drawTerrain(selectedTerrain == TerrainId::NONE ? TerrainId::DIRT : selectedTerrain, 0, rng);

	map->players[0].canComputerPlay = true;
	map->players[0].canHumanPlay = true;
	map->players[1] = map->players[0];

	auto knownHeroes = LIBRARY->objtypeh->knownSubObjects(Obj::HERO);

	auto addHero = [&](auto & selector, PlayerColor color, const int3 & position)
	{
		selector->selectedHero->setOwner(color);
		selector->selectedHero->pos = position;
		for(size_t i=0; i<GameConstants::PRIMARY_SKILLS; i++)
			selector->selectedHero->pushPrimSkill(PrimarySkill(i), std::stoi(selector->primSkillsInput[i]->getText()));
		selector->selectedHero->clearSlots();
		for(int slot = 0; slot < GameConstants::ARMY_SIZE; slot++)
			if(!selector->selectedArmy->slotEmpty(SlotID(slot)))
			{
				selector->selectedHero->putStack(SlotID(slot), selector->selectedArmy->detachStack(SlotID(slot)));
				selector->selectedHero->getArmy()->setStackCount(SlotID(slot), TextOperations::parseMetric<int>(selector->selectedArmyInput[slot]->getText()));
			}
		map->getEditManager()->insertObject(selector->selectedHero);
	};

	addHero(heroSelector1, PlayerColor(0), int3(5, 6, 0));
	if(selectedTown == FactionID::NONE)
		addHero(heroSelector2, PlayerColor(1), int3(5, 5, 0));
	else
	{
		auto factory = LIBRARY->objtypeh->getHandlerFor(Obj::TOWN, selectedTown);
		auto templates = factory->getTemplates();
		auto obj = factory->create(cb.get(), templates.front());
		auto townObj = std::dynamic_pointer_cast<CGTownInstance>(obj);
		obj->setOwner(PlayerColor(1));
		obj->pos = int3(5, 5, 0);
		for (const auto & building : townObj->getTown()->getAllBuildings())
			townObj->addBuilding(building);
		if(!heroSelector2->selectedHero)
		{
			for(int slot = 0; slot < GameConstants::ARMY_SIZE; slot++)
				if(!heroSelector2->selectedArmy->slotEmpty(SlotID(slot)))
				{
					townObj->getArmy()->putStack(SlotID(slot), heroSelector2->selectedArmy->detachStack(SlotID(slot)));
					townObj->getArmy()->setStackCount(SlotID(slot), TextOperations::parseMetric<int>(heroSelector2->selectedArmyInput[slot]->getText()));
				}
		}
		else
			addHero(heroSelector2, PlayerColor(1), int3(5, 5, 0));

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
