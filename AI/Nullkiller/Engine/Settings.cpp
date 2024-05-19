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
#include "../../../lib/json/JsonUtils.h"

namespace NKAI
{
	Settings::Settings()
		: maxRoamingHeroes(8),
		mainHeroTurnDistanceLimit(10),
		scoutHeroTurnDistanceLimit(5),
		maxGoldPressure(0.3f), 
		maxpass(10),
		allowObjectGraph(true),
		useTroopsFromGarrisons(false),
		openMap(true)
	{
		JsonNode node = JsonUtils::assembleFromFiles("config/ai/nkai/nkai-settings");

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

		if(node.Struct()["maxGoldPressure"].isNumber())
		{
			maxGoldPressure = node.Struct()["maxGoldPressure"].Float();
		}

		if(!node.Struct()["allowObjectGraph"].isNull())
		{
			allowObjectGraph = node.Struct()["allowObjectGraph"].Bool();
		}

		if(!node.Struct()["openMap"].isNull())
		{
			openMap = node.Struct()["openMap"].Bool();
		}

		if(!node.Struct()["useTroopsFromGarrisons"].isNull())
		{
			useTroopsFromGarrisons = node.Struct()["useTroopsFromGarrisons"].Bool();
		}
	}
}
