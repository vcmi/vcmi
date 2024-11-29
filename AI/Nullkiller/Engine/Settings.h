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

namespace NKAI
{
	class Settings
	{
	private:
		int maxRoamingHeroes;
		int mainHeroTurnDistanceLimit;
		int scoutHeroTurnDistanceLimit;
		int maxpass;
		int pathfinderBucketsCount;
		int pathfinderBucketSize;
		float maxGoldPressure;
		float retreatThresholdRelative;
		float retreatThresholdAbsolute;
		float safeAttackRatio;
		bool allowObjectGraph;
		bool useTroopsFromGarrisons;
		bool openMap;
		bool useFuzzy;

	public:
		explicit Settings(int difficultyLevel);

		int getMaxPass() const { return maxpass; }
		float getMaxGoldPressure() const { return maxGoldPressure; }
		float getRetreatThresholdRelative() const { return retreatThresholdRelative; }
		float getRetreatThresholdAbsolute() const { return retreatThresholdAbsolute; }
		float getSafeAttackRatio() const { return safeAttackRatio; }
		int getMaxRoamingHeroes() const { return maxRoamingHeroes; }
		int getMainHeroTurnDistanceLimit() const { return mainHeroTurnDistanceLimit; }
		int getScoutHeroTurnDistanceLimit() const { return scoutHeroTurnDistanceLimit; }
		int getPathfinderBucketsCount() const { return pathfinderBucketsCount; }
		int getPathfinderBucketSize() const { return pathfinderBucketSize; }
		bool isObjectGraphAllowed() const { return allowObjectGraph; }
		bool isGarrisonTroopsUsageAllowed() const { return useTroopsFromGarrisons; }
		bool isOpenMap() const { return openMap; }
		bool isUseFuzzy() const { return useFuzzy; }
	};
}
