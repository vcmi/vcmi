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

VCMI_LIB_NAMESPACE_BEGIN

class CModHandler;
class CModIndentifier;
class CModInfo;
struct CModVersion;
class JsonNode;
class IHandlerBase;
class CIdentifierStorage;
class CContentHandler;
struct ModVerificationInfo;
class ResourcePath;
class MetaString;

using TModID = std::string;

class DLL_LINKAGE CModHandler final : boost::noncopyable
{
	std::map <TModID, CModInfo> allMods;
	std::vector <TModID> activeMods;//active mods, in order in which they were loaded
	std::unique_ptr<CModInfo> coreMod;
	mutable std::unique_ptr<MetaString> modLoadErrors;

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

public:
	std::shared_ptr<CContentHandler> content; //(!)Do not serialize FIXME: make private

	/// receives list of available mods and trying to load mod.json from all of them
	void initializeConfig();
	void loadMods();
	void loadModFilesystems();

	/// returns ID of mod that provides selected file resource
	TModID findResourceOrigin(const ResourcePath & name) const;

	std::string getModLanguage(const TModID & modId) const;

	std::set<TModID> getModDependencies(const TModID & modId, bool & isModFound) const;

	/// returns list of all (active) mods
	std::vector<std::string> getAllMods() const;
	std::vector<std::string> getActiveMods() const;

	/// Returns human-readable string that describes erros encounter during mod loading, such as missing dependencies
	std::string getModLoadErrors() const;
	
	const CModInfo & getModInfo(const TModID & modId) const;

	/// load content from all available mods
	void load();
	void afterLoad(bool onlyEssential);

	CModHandler();
	~CModHandler();
};

VCMI_LIB_NAMESPACE_END
