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

BattleOnlyModeTab::BattleOnlyModeTab()
	: startInfo(std::make_shared<BattleOnlyModeStartInfo>())
	, disabledColor(GAME->server().isHost() ? Colors::WHITE : Colors::ORANGE)
{
	OBJECT_CONSTRUCTION;

	init();

	backgroundImage = std::make_shared<CPicture>(ImagePath::builtin("AdventureOptionsBackgroundClear"), 0, 6);
	buttonOk = std::make_shared<CButton>(Point(148, 430), AnimationPath::builtin("CBBEGIB"), CButton::tooltip(), [this](){ startBattle(); }, EShortcut::GLOBAL_ACCEPT);
	buttonOk->block(true);
	title = std::make_shared<CLabel>(220, 35, FONT_BIG, ETextAlignment::CENTER, Colors::YELLOW, LIBRARY->generaltexth->translate("vcmi.lobby.battleOnlyMode"));
	subTitle = std::make_shared<CMultiLineLabel>(Rect(55, 40, 333, 40), FONT_SMALL, ETextAlignment::BOTTOMCENTER, Colors::WHITE, LIBRARY->generaltexth->translate("vcmi.lobby.battleOnlyModeSubTitle"));

	battlefieldSelector = std::make_shared<CButton>(Point(120, 370), AnimationPath::builtin("GSPButtonClear"), CButton::tooltip(), [this](){
		std::vector<std::string> texts;
		std::vector<std::shared_ptr<IImage>> images;

		auto & terrains = LIBRARY->terrainTypeHandler->objects;
		for (const auto & terrain : terrains)
		{
			if(!terrain->isPassable())
				continue;

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
	buttonReset = std::make_shared<CButton>(Point(120, 400), AnimationPath::builtin("GSPButtonClear"), CButton::tooltip(), [this](){
		if(GAME->server().isHost())
		{
			startInfo->selectedTerrain = TerrainId::DIRT;
			startInfo->selectedTown = std::nullopt;
			startInfo->selectedHero[0] = std::nullopt;
			startInfo->selectedArmy[0].fill(CStackBasicDescriptor(CreatureID::NONE, 1));
			for(size_t i=0; i<GameConstants::ARMY_SIZE; i++)
				heroSelector1->selectedArmyInput.at(i)->disable();
		}
		startInfo->selectedHero[1] = std::nullopt;
		startInfo->selectedArmy[1].fill(CStackBasicDescriptor(CreatureID::NONE, 1));
		for(size_t i=0; i<GameConstants::ARMY_SIZE; i++)
			heroSelector2->selectedArmyInput.at(i)->disable();
		onChange();
	});
	buttonReset->setTextOverlay(LIBRARY->generaltexth->translate("vcmi.lobby.battleOnlyModeReset"), EFonts::FONT_SMALL, Colors::WHITE);

	heroSelector1 = std::make_shared<BattleOnlyModeHeroSelector>(0, *this, Point(91, 90));
	heroSelector2 = std::make_shared<BattleOnlyModeHeroSelector>(1, *this, Point(91, 225));

	heroSelector1->setInputEnabled(GAME->server().isHost());

	onChange();
}

void BattleOnlyModeTab::init()
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

void BattleOnlyModeTab::onChange()
{
	GAME->server().setBattleOnlyModeStartInfo(startInfo);
}

void BattleOnlyModeTab::update()
{
	setTerrainButtonText();
	setOkButtonEnabled();
	
	heroSelector1->setHeroIcon();
	heroSelector1->setCreatureIcons();
	heroSelector2->setHeroIcon();
	heroSelector2->setCreatureIcons();
	redraw();
}

void BattleOnlyModeTab::applyStartInfo(std::shared_ptr<BattleOnlyModeStartInfo> si)
{
	startInfo = si;
	update();
}

void BattleOnlyModeTab::setTerrainButtonText()
{
	battlefieldSelector->setTextOverlay(LIBRARY->generaltexth->translate("vcmi.lobby.battleOnlyModeBattlefield") + ":   " + (startInfo->selectedTerrain ? (*startInfo->selectedTerrain).toEntity(LIBRARY)->getNameTranslated() : (*startInfo->selectedTown).toEntity(LIBRARY)->getNameTranslated()), EFonts::FONT_SMALL, disabledColor);
}

void BattleOnlyModeTab::setOkButtonEnabled()
{
	bool army2Empty = std::all_of(startInfo->selectedArmy[1].begin(), startInfo->selectedArmy[1].end(), [](const auto x) { return x.getId() == CreatureID::NONE; });

	bool canStart = (startInfo->selectedTerrain || startInfo->selectedTown);
	canStart &= (startInfo->selectedHero[0] && ((startInfo->selectedHero[1]) || (startInfo->selectedTown && !army2Empty)));
	buttonOk->block(!canStart || GAME->server().isGuest());
}

std::shared_ptr<IImage> drawBlackBox(Point size, std::string text, ColorRGBA color)
{
	auto image = ENGINE->renderHandler().createImage(size, CanvasScalingPolicy::AUTO);
	Canvas canvas = image->getCanvas();
	canvas.drawColor(Rect(0, 0, size.x, size.y), Colors::BLACK);
	canvas.drawText(Point(size.x / 2, size.y / 2), FONT_TINY, color, ETextAlignment::CENTER, text);
	return image;
}

BattleOnlyModeHeroSelector::BattleOnlyModeHeroSelector(int id, BattleOnlyModeTab& p, Point position)
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
			if(parent.startInfo->selectedArmy[id][i].getId() != CreatureID::NONE)
			{
				parent.startInfo->selectedArmy[id][i].setCount(TextOperations::parseMetric<int>(text));
				parent.onChange();
				selectedArmyInput[i]->enable();
			}
			else
				selectedArmyInput[i]->disable();
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
			if(heroA->heroClass->faction != heroB->heroClass->faction)
				return heroA->heroClass->faction < heroB->heroClass->faction;
			if(heroA->heroClass->getId() != heroB->heroClass->getId())
				return heroA->heroClass->getId() < heroB->heroClass->getId();
			return heroA->getNameTranslated() < heroB->getNameTranslated();
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
		auto window = std::make_shared<CObjectListWindow>(texts, nullptr, LIBRARY->generaltexth->translate("vcmi.lobby.battleOnlyModeHeroSelect"), LIBRARY->generaltexth->translate("vcmi.lobby.battleOnlyModeHeroSelect"), [this, heroes](int index){
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
		if(parent.startInfo->selectedArmy[id][i].getId() == CreatureID::NONE)
		{
			creatureImage[i] = std::make_shared<CPicture>(drawBlackBox(Point(32, 32), LIBRARY->generaltexth->translate("vcmi.lobby.battleOnlyModeSelect"), id == 1 ? Colors::WHITE : parent.disabledColor), Point(6 + i * 36, 78));
			selectedArmyInput[i]->disable();
		}
		else
		{
			auto unit = parent.startInfo->selectedArmy[id][i];
			auto creatureID = unit.getId();
			creatureImage[i] = std::make_shared<CPicture>(ENGINE->renderHandler().loadAnimation(AnimationPath::builtin("CPRSMALL"), EImageBlitMode::COLORKEY)->getImage(LIBRARY->creh->objects.at(creatureID)->getIconIndex()), Point(6 + i * 36, 78));
			selectedArmyInput[i]->setText(TextOperations::formatMetric(unit.getCount(), 3));
			selectedArmyInput[i]->enable();
		}

		creatureImage[i]->addLClickCallback([this, i](){
			auto allowedSet = LIBRARY->creh->getDefaultAllowed();
			std::vector<CreatureID> creatures(allowedSet.begin(), allowedSet.end());
			std::sort(creatures.begin(), creatures.end(), [](auto a, auto b) {
				auto creatureA = a.toCreature();
				auto creatureB = b.toCreature();
				if(creatureA->getFactionID() != creatureB->getFactionID())
					return creatureA->getFactionID() < creatureB->getFactionID();
				if(creatureA->getLevel() != creatureB->getLevel())
					return creatureA->getLevel() < creatureB->getLevel();
				if(creatureA->upgrades.size() != creatureB->upgrades.size())
					return creatureA->upgrades.size() > creatureB->upgrades.size();
				return creatureA->getNameSingularTranslated() < creatureB->getNameSingularTranslated();
			});

			int selectedIndex = parent.startInfo->selectedArmy[id][i].getId() == CreatureID::NONE ? 0 : (1 + std::distance(creatures.begin(), std::find_if(creatures.begin(), creatures.end(), [this, i](auto creatureID) {
				return creatureID == parent.startInfo->selectedArmy[id][i].getId();
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
			auto window = std::make_shared<CObjectListWindow>(texts, nullptr, LIBRARY->generaltexth->translate("vcmi.lobby.battleOnlyModeCreatureSelect"), LIBRARY->generaltexth->translate("vcmi.lobby.battleOnlyModeCreatureSelect"), [this, creatures, i](int index){
				if(index == 0)
				{
					parent.startInfo->selectedArmy[id][i] = CStackBasicDescriptor(CreatureID::NONE, 1);
					parent.onChange();
					return;
				}
				index--;

				auto creature = creatures.at(index).toCreature();
				parent.startInfo->selectedArmy[id][i] = CStackBasicDescriptor(creature->getId(), 100);
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
			if(parent.startInfo->selectedArmy[id][i].getId() == CreatureID::NONE)
				return;
			
			ENGINE->windows().createAndPushWindow<CStackWindow>(LIBRARY->creh->objects.at(parent.startInfo->selectedArmy[id][i].getId()).get(), true);
		});
	}
}

void BattleOnlyModeTab::startBattle()
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
			if(startInfo->selectedArmy[sel][slot].getId() != CreatureID::NONE)
				obj->setCreature(SlotID(slot), startInfo->selectedArmy[sel][slot].getId(), startInfo->selectedArmy[sel][slot].getCount());
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
				if(startInfo->selectedArmy[1][slot].getId() != CreatureID::NONE)
					townObj->getArmy()->setCreature(SlotID(slot), startInfo->selectedArmy[1][slot].getId(), startInfo->selectedArmy[1][slot].getCount());
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
