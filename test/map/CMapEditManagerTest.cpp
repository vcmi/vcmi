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

#include "../lib/filesystem/ResourcePath.h"
#include "../lib/mapping/CMapService.h"
#include "../lib/mapping/CMap.h"
#include "../lib/TerrainHandler.h"
#include "../lib/mapping/CMapEditManager.h"
#include "../lib/int3.h"
#include "../lib/CRandomGenerator.h"
#include "../lib/GameLibrary.h"
#include "../lib/callback/EditorCallback.h"


TEST(MapManager, DrawTerrain_Type)
{
	try
	{
		auto map = std::make_unique<CMap>(nullptr);
		CRandomGenerator rand;
		map->width = 52;
		map->height = 52;
		map->initTerrain();
		auto editManager = map->getEditManager();
		editManager->clearTerrain(&rand);

		// 1x1 Blow up
		editManager->getTerrainSelection().select(int3(5, 5, 0));
		editManager->drawTerrain(ETerrainId::GRASS, 10, &rand);
		static const int3 squareCheck[] = { int3(5,5,0), int3(5,4,0), int3(4,4,0), int3(4,5,0) };
		for(const auto & tile : squareCheck)
		{
			EXPECT_EQ(map->getTile(tile).getTerrainID(), ETerrainId::GRASS);
		}

		// Concat to square
		editManager->getTerrainSelection().select(int3(6, 5, 0));
		editManager->drawTerrain(ETerrainId::GRASS, 10, &rand);
		EXPECT_EQ(map->getTile(int3(6, 4, 0)).getTerrainID(), ETerrainId::GRASS);
		editManager->getTerrainSelection().select(int3(6, 5, 0));
		editManager->drawTerrain(ETerrainId::LAVA, 10, &rand);
		EXPECT_EQ(map->getTile(int3(4, 4, 0)).getTerrainID(), ETerrainId::GRASS);
		EXPECT_EQ(map->getTile(int3(7, 4, 0)).getTerrainID(), ETerrainId::LAVA);

		// Special case water,rock
		editManager->getTerrainSelection().selectRange(MapRect(int3(10, 10, 0), 10, 5));
		editManager->drawTerrain(ETerrainId::GRASS, 10, &rand);
		editManager->getTerrainSelection().selectRange(MapRect(int3(15, 17, 0), 10, 5));
		editManager->drawTerrain(ETerrainId::GRASS, 10, &rand);
		editManager->getTerrainSelection().select(int3(21, 16, 0));
		editManager->drawTerrain(ETerrainId::GRASS, 10, &rand);
		EXPECT_EQ(map->getTile(int3(20, 15, 0)).getTerrainID(), ETerrainId::GRASS);

		// Special case non water,rock
		static const int3 diagonalCheck[] = { int3(31,42,0), int3(32,42,0), int3(32,43,0), int3(33,43,0), int3(33,44,0),
											int3(34,44,0), int3(34,45,0), int3(35,45,0), int3(35,46,0), int3(36,46,0),
											int3(36,47,0), int3(37,47,0)};
		for(const auto & tile : diagonalCheck)
		{
			editManager->getTerrainSelection().select(tile);
		}
		editManager->drawTerrain(ETerrainId::GRASS, 10, &rand);
		EXPECT_EQ(map->getTile(int3(35, 44, 0)).getTerrainID(), ETerrainId::WATER);

		// Rock case
		editManager->getTerrainSelection().selectRange(MapRect(int3(1, 1, 1), 15, 15));
		editManager->drawTerrain(ETerrainId::SUBTERRANEAN, 10, &rand);
		std::vector<int3> vec({ int3(6, 6, 1), int3(7, 6, 1), int3(8, 6, 1), int3(5, 7, 1), int3(6, 7, 1), int3(7, 7, 1),
								int3(8, 7, 1), int3(4, 8, 1), int3(5, 8, 1), int3(6, 8, 1)});
		editManager->getTerrainSelection().setSelection(vec);
		editManager->drawTerrain(ETerrainId::ROCK, 10, &rand);
		EXPECT_TRUE(!map->getTile(int3(5, 6, 1)).getTerrain()->isPassable() || !map->getTile(int3(7, 8, 1)).getTerrain()->isPassable());

		//todo: add checks here and enable, also use smaller size
		#if 0

		// next check
		auto map2 = make_unique<CMap>();
		map2->width = 128;
		map2->height = 128;
		map2->initTerrain();
		auto editManager2 = map2->getEditManager();

		editManager2->getTerrainSelection().selectRange(MapRect(int3(0, 0, 1), 128, 128));
		editManager2->drawTerrain(CTerrainType("subterra"));

		std::vector<int3> selection({ int3(95, 43, 1), int3(95, 44, 1), int3(94, 45, 1), int3(95, 45, 1), int3(96, 45, 1),
									int3(93, 46, 1), int3(94, 46, 1), int3(95, 46, 1), int3(96, 46, 1), int3(97, 46, 1),
									int3(98, 46, 1), int3(99, 46, 1)});
		editManager2->getTerrainSelection().setSelection(selection);
		editManager2->drawTerrain(CTerrainType("rock"));
		#endif // 0

	}
	catch(const std::exception & e)
	{
		FAIL()<<e.what();
		throw;
	}
}

TEST(MapManager, DrawTerrain_View)
{
	try
	{
		const ResourcePath testMap("test/TerrainViewTest", EResType::MAP);
		// Load maps and json config
		CMapService mapService;
		EditorCallback cb(nullptr);
		const auto originalMap = mapService.loadMap(testMap, &cb);
		auto map = mapService.loadMap(testMap, &cb);

		// Validate edit manager
		auto editManager = map->getEditManager();
		CRandomGenerator gen;
		const JsonNode viewNode(JsonPath::builtin("test/terrainViewMappings"));
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

			// Get mapping range
			const auto & pattern = LIBRARY->terviewh->getTerrainViewPatternById(groupStr, id); 
			const auto & mapping = pattern->get().mapping;

			const auto & positionsNode = node["pos"].Vector();
			for (const auto & posNode : positionsNode)
			{
				const auto & posVector = posNode.Vector();
				if(posVector.size() != 3) throw std::runtime_error("A position should consist of three values x,y,z. Continue with next position.");
				int3 pos((si32)posVector[0].Float(), (si32)posVector[1].Float(), (si32)posVector[2].Float());
				const auto & originalTile = originalMap->getTile(pos);
				editManager->getTerrainSelection().selectRange(MapRect(pos, 1, 1));
				editManager->drawTerrain(originalTile.getTerrainID(), 10, &gen);
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
				EXPECT_TRUE(isInRange);
				if(!isInRange)
					FAIL()<<("No or invalid pattern found for current position.");
			}
		}
	}
	catch(const std::exception & e)
	{
		FAIL()<<e.what();
		throw;
	}
}
