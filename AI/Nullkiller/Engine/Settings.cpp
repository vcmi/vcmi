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

#include "../../../lib/constants/StringConstants.h"
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
	Settings::Settings(int difficultyLevel)
		: maxRoamingHeroes(8),
		mainHeroTurnDistanceLimit(10),
		scoutHeroTurnDistanceLimit(5),
		maxGoldPressure(0.3f),
		retreatThresholdRelative(0.3),
		retreatThresholdAbsolute(10000),
		safeAttackRatio(1.1),
		maxpass(10),
		pathfinderBucketsCount(1),
		pathfinderBucketSize(32),
		allowObjectGraph(true),
		useTroopsFromGarrisons(false),
		openMap(true),
		useFuzzy(false)
	{
		const std::string & difficultyName = GameConstants::DIFFICULTY_NAMES[difficultyLevel];
		const JsonNode & rootNode = JsonUtils::assembleFromFiles("config/ai/nkai/nkai-settings");
		const JsonNode & node = rootNode[difficultyName];

		maxRoamingHeroes = node["maxRoamingHeroes"].Integer();
		mainHeroTurnDistanceLimit = node["mainHeroTurnDistanceLimit"].Integer();
		scoutHeroTurnDistanceLimit = node["scoutHeroTurnDistanceLimit"].Integer();
		maxpass = node["maxpass"].Integer();
		pathfinderBucketsCount = node["pathfinderBucketsCount"].Integer();
		pathfinderBucketSize = node["pathfinderBucketSize"].Integer();
		maxGoldPressure = node["maxGoldPressure"].Float();
		retreatThresholdRelative = node["retreatThresholdRelative"].Float();
		retreatThresholdAbsolute = node["retreatThresholdAbsolute"].Float();
		safeAttackRatio = node["safeAttackRatio"].Float();
		allowObjectGraph = node["allowObjectGraph"].Bool();
		openMap = node["openMap"].Bool();
		useFuzzy = node["useFuzzy"].Bool();
		useTroopsFromGarrisons = node["useTroopsFromGarrisons"].Bool();
	}
}
