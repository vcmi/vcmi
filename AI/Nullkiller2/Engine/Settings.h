/*
* Settings.h, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/
#pragma once

VCMI_LIB_NAMESPACE_BEGIN

class JsonNode;
class ResourcePath;

VCMI_LIB_NAMESPACE_END

namespace NK2AI
{
	class Settings
	{
		int maxRoamingHeroes;
		int maxRoamingHeroesPerTown; // added on top of @link maxRoamingHeroes
		int mainHeroTurnDistanceLimit;
		int scoutHeroTurnDistanceLimit;
		int threatTurnDistanceLimit;
		int maxPass;
		int maxPriorityPass;
		int pathfinderBucketsCount;
		int pathfinderBucketSize;
		float maxGoldPressure;
		float retreatThresholdRelative;
		float retreatThresholdAbsolute;
		float safeAttackRatio;
		float maxArmyLossTarget;
		bool allowObjectGraph;
		bool useTroopsFromGarrisons;
		bool useOneWayMonoliths;
		bool updateHitmapOnTileReveal;
		bool openMap;
		bool useFuzzy;

	public:
		explicit Settings(int difficultyLevel);

		int getMaxPass() const { return maxPass; }
		int getMaxPriorityPass() const { return maxPriorityPass; }
		float getMaxGoldPressure() const { return maxGoldPressure; }
		float getRetreatThresholdRelative() const { return retreatThresholdRelative; }
		float getRetreatThresholdAbsolute() const { return retreatThresholdAbsolute; }
		float getSafeAttackRatio() const { return safeAttackRatio; }
		float getMaxArmyLossTarget() const { return maxArmyLossTarget; }
		int getMaxRoamingHeroes() const { return maxRoamingHeroes; }
		int getMaxRoamingHeroesPerTown() const { return maxRoamingHeroesPerTown; }
		int getMainHeroTurnDistanceLimit() const { return mainHeroTurnDistanceLimit; }
		int getScoutHeroTurnDistanceLimit() const { return scoutHeroTurnDistanceLimit; }
		int getThreatTurnDistanceLimit() const { return threatTurnDistanceLimit; }
		int getPathfinderBucketsCount() const { return pathfinderBucketsCount; }
		int getPathfinderBucketSize() const { return pathfinderBucketSize; }
		bool isObjectGraphAllowed() const { return allowObjectGraph; }
		bool isGarrisonTroopsUsageAllowed() const { return useTroopsFromGarrisons; }
		bool isOneWayMonolithUsageAllowed() const { return useOneWayMonoliths; }
		bool isUpdateHitmapOnTileReveal() const { return updateHitmapOnTileReveal; }
		bool isOpenMap() const { return openMap; }
		bool isUseFuzzy() const { return useFuzzy; }
	};
}
