/*
 * CModHandler.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "CModVersion.h"

VCMI_LIB_NAMESPACE_BEGIN

class CModHandler;
class CModIndentifier;
class CModInfo;
class JsonNode;
class IHandlerBase;
class CIdentifierStorage;
class CContentHandler;
class ResourceID;

using TModID = std::string;

class DLL_LINKAGE CModHandler : boost::noncopyable
{
	std::map <TModID, CModInfo> allMods;
	std::vector <TModID> activeMods;//active mods, in order in which they were loaded
	std::unique_ptr<CModInfo> coreMod;

	bool hasCircularDependency(const TModID & mod, std::set<TModID> currentList = std::set<TModID>()) const;

	/**
	* 1. Set apart mods with resolved dependencies from mods which have unresolved dependencies
	* 2. Sort resolved mods using topological algorithm
	* 3. Log all problem mods and their unresolved dependencies
	*
	* @param modsToResolve list of valid mod IDs (checkDependencies returned true - TODO: Clarify it.)
	* @return a vector of the topologically sorted resolved mods: child nodes (dependent mods) have greater index than parents
	*/
	std::vector <TModID> validateAndSortDependencies(std::vector <TModID> modsToResolve) const;

	std::vector<std::string> getModList(const std::string & path) const;
	void loadMods(const std::string & path, const std::string & parent, const JsonNode & modSettings, bool enableMods);
	void loadOneMod(std::string modName, const std::string & parent, const JsonNode & modSettings, bool enableMods);
	void loadTranslation(const TModID & modName);

	bool validateTranslations(TModID modName) const;

	CModVersion getModVersion(TModID modName) const;

	/// Attempt to set active mods according to provided list of mods from save, throws on failure
	void trySetActiveMods(std::vector<TModID> saveActiveMods, const std::map<TModID, CModVersion> & modList);

public:
	std::shared_ptr<CContentHandler> content; //(!)Do not serialize FIXME: make private

	/// receives list of available mods and trying to load mod.json from all of them
	void initializeConfig();
	void loadMods(bool onlyEssential = false);
	void loadModFilesystems();

	/// returns ID of mod that provides selected file resource
	TModID findResourceOrigin(const ResourceID & name);

	std::string getModLanguage(const TModID & modId) const;

	std::set<TModID> getModDependencies(const TModID & modId, bool & isModFound) const;

	/// returns list of all (active) mods
	std::vector<std::string> getAllMods();
	std::vector<std::string> getActiveMods();
	
	const CModInfo & getModInfo(const TModID & modId) const;

	/// load content from all available mods
	void load();
	void afterLoad(bool onlyEssential);

	CModHandler();
	virtual ~CModHandler();

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		if(h.saving)
		{
			h & activeMods;
			for(const auto & m : activeMods)
			{
				CModVersion version = getModVersion(m);
				h & version;
			}
		}
		else
		{
			loadMods();
			std::vector<TModID> saveActiveMods;
			std::map<TModID, CModVersion> modVersions;
			h & saveActiveMods;

			for(const auto & m : saveActiveMods)
				h & modVersions[m];

			trySetActiveMods(saveActiveMods, modVersions);
		}
	}
};

VCMI_LIB_NAMESPACE_END
