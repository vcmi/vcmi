
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
#include "../lib/filesystem/CResourceLoader.h"
#include "../lib/filesystem/CFilesystemLoader.h"
#include "../lib/JsonNode.h"
#include "../lib/mapping/CMapEditManager.h"
#include "../lib/int3.h"
#include "../lib/CRandomGenerator.h"

BOOST_AUTO_TEST_CASE(CMapEditManager_DrawTerrain)
{
	try
	{
		// Load maps and json config
		auto loader = make_shared<CFilesystemLoader>(".");
		CResourceHandler::get()->addLoader("test/", loader, false);
		const auto originalMap = CMapService::loadMap("test/TerrainViewTest");
		auto map = CMapService::loadMap("test/TerrainViewTest");
		logGlobal->infoStream() << "Loaded test map successfully.";

		// Validate edit manager
		auto editManager = map->getEditManager();
		CRandomGenerator gen;
		const JsonNode viewNode(ResourceID("test/terrainViewMappings", EResType::TEXT));
		const auto & mappingsNode = viewNode["mappings"].Vector();
		BOOST_FOREACH(const auto & node, mappingsNode)
		{
			// Get terrain group and id
			const auto & patternStr = node["pattern"].String();
			std::vector<std::string> patternParts;
			boost::split(patternParts, patternStr, boost::is_any_of("."));
			if(patternParts.size() != 2) throw std::runtime_error("A pattern should consist of two parts, the group and the id. Continue with next pattern.");
			const auto & groupStr = patternParts[0];
			const auto & id = patternParts[1];
			auto terGroup = CTerrainViewPatternConfig::get().getTerrainGroup(groupStr);

			// Get mapping range
			const auto & pattern = CTerrainViewPatternConfig::get().getPatternById(terGroup, id);
			const auto & mapping = (*pattern).mapping;

			const auto & positionsNode = node["pos"].Vector();
			BOOST_FOREACH(const auto & posNode, positionsNode)
			{
				const auto & posVector = posNode.Vector();
				if(posVector.size() != 3) throw std::runtime_error("A position should consist of three values x,y,z. Continue with next position.");
				int3 pos(posVector[0].Float(), posVector[1].Float(), posVector[2].Float());
				logGlobal->infoStream() << boost::format("Test pattern '%s' on position x '%d', y '%d', z '%d'.") % patternStr % pos.x % pos.y % pos.z;
				const auto & originalTile = originalMap->getTile(pos);
				editManager->drawTerrain(MapRect(pos, 1, 1), originalTile.terType, &gen);
				const auto & tile = map->getTile(pos);
				bool isInRange = false;
				BOOST_FOREACH(const auto & range, mapping)
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
		logGlobal-> errorStream() << e.what();
	}
}
