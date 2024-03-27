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
		float maxGoldPreasure;
		bool allowObjectGraph;

	public:
		Settings();

		int getMaxPass() const { return maxpass; }
		float getMaxGoldPreasure() const { return maxGoldPreasure; }
		int getMaxRoamingHeroes() const { return maxRoamingHeroes; }
		int getMainHeroTurnDistanceLimit() const { return mainHeroTurnDistanceLimit; }
		int getScoutHeroTurnDistanceLimit() const { return scoutHeroTurnDistanceLimit; }
		bool isObjectGraphAllowed() const { return allowObjectGraph; }

	private:
		void loadFromMod(const std::string & modName, const ResourcePath & resource);
	};
}