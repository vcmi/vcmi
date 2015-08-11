
/*
 * CMapEditManagerTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include <boost/test/unit_test.hpp>

#include "../lib/mapping/CMapService.h"
#include "../lib/mapping/CMap.h"
#include "../lib/filesystem/Filesystem.h"
#include "../lib/filesystem/CFilesystemLoader.h"
#include "../lib/filesystem/AdapterLoaders.h"
#include "../lib/JsonNode.h"
#include "../lib/mapping/CMapEditManager.h"
#include "../lib/int3.h"
#include "../lib/CRandomGenerator.h"
#include "../lib/VCMI_Lib.h"

BOOST_AUTO_TEST_CASE(CMapEditManager_DrawTerrain_Type)
{
	logGlobal->infoStream() << "CMapEditManager_DrawTerrain_Type start";
	try
	{
		auto map = make_unique<CMap>();
		map->width = 52;
		map->height = 52;
		map->initTerrain();
		auto editManager = map->getEditManager();
		editManager->clearTerrain();

		// 1x1 Blow up
		editManager->getTerrainSelection().select(int3(5, 5, 0));
		editManager->drawTerrain(ETerrainType::GRASS);
		static const int3 squareCheck[] = { int3(5,5,0), int3(5,4,0), int3(4,4,0), int3(4,5,0) };
		for(int i = 0; i < ARRAY_COUNT(squareCheck); ++i)
		{
			BOOST_CHECK(map->getTile(squareCheck[i]).terType == ETerrainType::GRASS);
		}

		// Concat to square
		editManager->getTerrainSelection().select(int3(6, 5, 0));
		editManager->drawTerrain(ETerrainType::GRASS);
		BOOST_CHECK(map->getTile(int3(6, 4, 0)).terType == ETerrainType::GRASS);
		editManager->getTerrainSelection().select(int3(6, 5, 0));
		editManager->drawTerrain(ETerrainType::LAVA);
		BOOST_CHECK(map->getTile(int3(4, 4, 0)).terType == ETerrainType::GRASS);
		BOOST_CHECK(map->getTile(int3(7, 4, 0)).terType == ETerrainType::LAVA);

		// Special case water,rock
		editManager->getTerrainSelection().selectRange(MapRect(int3(10, 10, 0), 10, 5));
		editManager->drawTerrain(ETerrainType::GRASS);
		editManager->getTerrainSelection().selectRange(MapRect(int3(15, 17, 0), 10, 5));
		editManager->drawTerrain(ETerrainType::GRASS);
		editManager->getTerrainSelection().select(int3(21, 16, 0));
		editManager->drawTerrain(ETerrainType::GRASS);
		BOOST_CHECK(map->getTile(int3(20, 15, 0)).terType == ETerrainType::GRASS);

		// Special case non water,rock
		static const int3 diagonalCheck[] = { int3(31,42,0), int3(32,42,0), int3(32,43,0), int3(33,43,0), int3(33,44,0),
											  int3(34,44,0), int3(34,45,0), int3(35,45,0), int3(35,46,0), int3(36,46,0),
											  int3(36,47,0), int3(37,47,0)};
		for(int i = 0; i < ARRAY_COUNT(diagonalCheck); ++i)
		{
			editManager->getTerrainSelection().select(diagonalCheck[i]);
		}
		editManager->drawTerrain(ETerrainType::GRASS);
		BOOST_CHECK(map->getTile(int3(35, 44, 0)).terType == ETerrainType::WATER);

		// Rock case
		editManager->getTerrainSelection().selectRange(MapRect(int3(1, 1, 1), 15, 15));
		editManager->drawTerrain(ETerrainType::SUBTERRANEAN);
		std::vector<int3> vec({ int3(6, 6, 1), int3(7, 6, 1), int3(8, 6, 1), int3(5, 7, 1), int3(6, 7, 1), int3(7, 7, 1),
								int3(8, 7, 1), int3(4, 8, 1), int3(5, 8, 1), int3(6, 8, 1)});
		editManager->getTerrainSelection().setSelection(vec);
		editManager->drawTerrain(ETerrainType::ROCK);
		BOOST_CHECK(map->getTile(int3(5, 6, 1)).terType == ETerrainType::ROCK || map->getTile(int3(7, 8, 1)).terType == ETerrainType::ROCK);

		//todo: add checks here and enable, also use smaller size
		#if 0 
		
		// next check
		auto map2 = make_unique<CMap>();
		map2->width = 128;
		map2->height = 128;
		map2->initTerrain();
		auto editManager2 = map2->getEditManager();

		editManager2->getTerrainSelection().selectRange(MapRect(int3(0, 0, 1), 128, 128));
		editManager2->drawTerrain(ETerrainType::SUBTERRANEAN);

		std::vector<int3> selection({ int3(95, 43, 1), int3(95, 44, 1), int3(94, 45, 1), int3(95, 45, 1), int3(96, 45, 1),
									int3(93, 46, 1), int3(94, 46, 1), int3(95, 46, 1), int3(96, 46, 1), int3(97, 46, 1),
									int3(98, 46, 1), int3(99, 46, 1)});
		editManager2->getTerrainSelection().setSelection(selection);
		editManager2->drawTerrain(ETerrainType::ROCK);
		#endif // 0

	}
	catch(const std::exception & e)
	{
		logGlobal->errorStream() << "CMapEditManager_DrawTerrain_Type crash" << "\n" << e.what();	
		throw;
	}	
	logGlobal->infoStream() << "CMapEditManager_DrawTerrain_Type finish";
}

BOOST_AUTO_TEST_CASE(CMapEditManager_DrawTerrain_View)
{
	logGlobal->infoStream() << "CMapEditManager_DrawTerrain_View start";	
	try
	{
		// Load maps and json config
		const std::string TEST_DATA_DIR = "test/";

		auto loader = new CFilesystemLoader("test/", TEST_DATA_DIR);
		dynamic_cast<CFilesystemList*>(CResourceHandler::get())->addLoader(loader, false);
		const auto originalMap = CMapService::loadMap("test/TerrainViewTest");
		auto map = CMapService::loadMap("test/TerrainViewTest");
		logGlobal->infoStream() << "Loaded test map successfully.";

		// Validate edit manager
		auto editManager = map->getEditManager();
		CRandomGenerator gen;
		const JsonNode viewNode(ResourceID("test/terrainViewMappings", EResType::TEXT));
		const auto & mappingsNode = viewNode["mappings"].Vector();
		for (const auto & node : mappingsNode)
		{
			// Get terrain group and id
			const auto & patternStr = node["pattern"].String();
			std::vector<std::string> patternParts;
			boost::split(patternParts, patternStr, boost::is_any_of("."));
			if(patternParts.size() != 2) throw std::runtime_error("A pattern should consist of two parts, the group and the id. Continue with next pattern.");
			const auto & groupStr = patternParts[0];
			const auto & id = patternParts[1];
			auto terGroup = VLC->terviewh->getTerrainGroup(groupStr);

			// Get mapping range
			const auto & pattern = VLC->terviewh->getTerrainViewPatternById(terGroup, id);
			const auto & mapping = (*pattern).mapping;

			const auto & positionsNode = node["pos"].Vector();
			for (const auto & posNode : positionsNode)
			{
				const auto & posVector = posNode.Vector();
				if(posVector.size() != 3) throw std::runtime_error("A position should consist of three values x,y,z. Continue with next position.");
				int3 pos(posVector[0].Float(), posVector[1].Float(), posVector[2].Float());
				logGlobal->infoStream() << boost::format("Test pattern '%s' on position x '%d', y '%d', z '%d'.") % patternStr % pos.x % pos.y % pos.z;
				CTerrainViewPatternUtils::printDebuggingInfoAboutTile(map.get(), pos);
				const auto & originalTile = originalMap->getTile(pos);
				editManager->getTerrainSelection().selectRange(MapRect(pos, 1, 1));
				editManager->drawTerrain(originalTile.terType, &gen);
				const auto & tile = map->getTile(pos);
				bool isInRange = false;
				for(const auto & range : mapping)
				{
					if(tile.terView >= range.first && tile.terView <= range.second)
					{
						isInRange = true;
						break;
					}
				}
				BOOST_CHECK(isInRange);
				if(!isInRange) logGlobal->errorStream() << "No or invalid pattern found for current position.";
			}
		}
	}
	catch(const std::exception & e)
	{
		logGlobal->infoStream() << "CMapEditManager_DrawTerrain_View crash"<< "\n" << e.what();
		throw;
	}
	logGlobal->infoStream() << "CMapEditManager_DrawTerrain_View finish";
}
