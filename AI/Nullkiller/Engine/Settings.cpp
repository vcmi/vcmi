/*
* Settings.cpp, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/
#include "StdInc.h"
#include <limits>

#include "Settings.h"
#include "../../../lib/mapObjectConstructors/AObjectTypeHandler.h"
#include "../../../lib/mapObjectConstructors/CObjectClassesHandler.h"
#include "../../../lib/mapObjectConstructors/CBankInstanceConstructor.h"
#include "../../../lib/mapObjects/MapObjects.h"
#include "../../../lib/modding/CModHandler.h"
#include "../../../lib/VCMI_Lib.h"
#include "../../../lib/filesystem/Filesystem.h"
#include "../../../lib/json/JsonNode.h"

namespace NKAI
{
	Settings::Settings()
		: maxRoamingHeroes(8),
		mainHeroTurnDistanceLimit(10),
		scoutHeroTurnDistanceLimit(5),
		maxGoldPreasure(0.3f), 
		maxpass(30),
		allowObjectGraph(false)
	{
		ResourcePath resource("config/ai/nkai/nkai-settings", EResType::JSON);

		loadFromMod("core", resource);

		for(const auto & modName : VLC->modh->getActiveMods())
		{
			if(CResourceHandler::get(modName)->existsResource(resource))
				loadFromMod(modName, resource);
		}
	}

	void Settings::loadFromMod(const std::string & modName, const ResourcePath & resource)
	{
		if(!CResourceHandler::get(modName)->existsResource(resource))
		{
			logGlobal->error("Failed to load font %s from mod %s", resource.getName(), modName);
			return;
		}

	    JsonNode node(JsonPath::fromResource(resource), modName);
		
		if(node.Struct()["maxRoamingHeroes"].isNumber())
		{
			maxRoamingHeroes = node.Struct()["maxRoamingHeroes"].Integer();
		}

		if(node.Struct()["mainHeroTurnDistanceLimit"].isNumber())
		{
			mainHeroTurnDistanceLimit = node.Struct()["mainHeroTurnDistanceLimit"].Integer();
		}

		if(node.Struct()["scoutHeroTurnDistanceLimit"].isNumber())
		{
			scoutHeroTurnDistanceLimit = node.Struct()["scoutHeroTurnDistanceLimit"].Integer();
		}

		if(node.Struct()["maxpass"].isNumber())
		{
			maxpass = node.Struct()["maxpass"].Integer();
		}

		if(node.Struct()["maxGoldPreasure"].isNumber())
		{
			maxGoldPreasure = node.Struct()["maxGoldPreasure"].Float();
		}

		if(!node.Struct()["allowObjectGraph"].isNull())
		{
			allowObjectGraph = node.Struct()["allowObjectGraph"].Bool();
		}
	}
}