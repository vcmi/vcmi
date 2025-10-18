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

#include "../gui/Shortcut.h"
#include "../gui/WindowHandler.h"
#include "../widgets/Buttons.h"
#include "../widgets/Images.h"

#include "../../lib/gameState/CGameState.h"
#include "../../lib/StartInfo.h"
#include "../../lib/VCMIDirs.h"
#include "../../lib/CRandomGenerator.h"
#include "../../lib/callback/EditorCallback.h"
#include "../../lib/mapObjects/CGHeroInstance.h"
#include "../../lib/mapObjectConstructors/AObjectTypeHandler.h"
#include "../../lib/mapObjectConstructors/CObjectClassesHandler.h"
#include "../../lib/mapping/CMap.h"
#include "../../lib/mapping/CMapInfo.h"
#include "../../lib/mapping/CMapEditManager.h"
#include "../../lib/mapping/CMapService.h"
#include "../../lib/mapping/MapFormat.h"
#include "../../lib/filesystem/Filesystem.h"

void BattleOnlyMode::openBattleWindow()
{
	ENGINE->windows().createAndPushWindow<BattleOnlyModeWindow>();
}

BattleOnlyModeWindow::BattleOnlyModeWindow()
	: CWindowObject(BORDERED)
{
	OBJECT_CONSTRUCTION;

	pos.w = 640;
	pos.h = 480;

	updateShadow();
	center();

	backgroundTexture = std::make_shared<FilledTexturePlayerColored>(Rect(0, 0, pos.w, pos.h));
	backgroundTexture->setPlayerColor(PlayerColor(1));
	buttonOk = std::make_shared<CButton>(Point(183, 388), AnimationPath::builtin("MuBchck"), CButton::tooltip(), [this](){ startBattle(); }, EShortcut::GLOBAL_ACCEPT);
	buttonAbort = std::make_shared<CButton>(Point(250, 388), AnimationPath::builtin("MuBcanc"), CButton::tooltip(), [this](){ close(); }, EShortcut::GLOBAL_CANCEL);
}

void BattleOnlyModeWindow::startBattle()
{
	auto map = std::make_unique<CMap>(nullptr);
	map->version = EMapFormat::VCMI;
	map->creationDateTime = std::time(nullptr);
	map->width = 4;
	map->height = 3;
	map->mapLevels = 1;
	map->battleOnly = true;
	
	map->initTerrain();
	map->getEditManager()->clearTerrain(&CRandomGenerator::getDefault());

	//auto terrain = LIBRARY->terrainTypeHandler->objects;
	map->getEditManager()->getTerrainSelection().selectAll();
	map->getEditManager()->drawTerrain(TerrainId::GRASS, 0, &CRandomGenerator::getDefault());

	map->players[0].canComputerPlay = true;
	map->players[0].canHumanPlay = true;
	map->players[1] = map->players[0];

	auto cb = std::make_unique<EditorCallback>(map.get());

	auto knownHeroes = LIBRARY->objtypeh->knownSubObjects(Obj::HERO);

	auto addHero = [&](MapObjectSubID heroType, PlayerColor color, const int3 & position)
	{
		auto it = knownHeroes.find(heroType);
		if (it == knownHeroes.end())
			return; // unknown hero type, nothing to add

		auto firstHeroType = *it;
		auto factory = LIBRARY->objtypeh->getHandlerFor(Obj::HERO, firstHeroType);
		auto templates = factory->getTemplates();

		auto obj = std::dynamic_pointer_cast<CGHeroInstance>(factory->create(cb.get(), templates.front()));
		obj->setOwner(color);
		obj->pos = position;
		/*if(heroType == MapObjectSubID(1))
		{*/
			obj->pushPrimSkill(PrimarySkill::ATTACK, 100);
			obj->getArmy()->setCreature(SlotID(0), CreatureID(1), 10000);
		//}
		map->getEditManager()->insertObject(obj);
	};

	addHero(MapObjectSubID(1), PlayerColor(0), int3(2, 1, 0));
	addHero(MapObjectSubID(2), PlayerColor(1), int3(3, 1, 0));

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
