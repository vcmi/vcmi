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

#include "filesystem/Filesystem.h"

#include "VCMI_Lib.h"
#include "JsonNode.h"

#ifdef __UCLIBC__
#undef major
#undef minor
#undef patch
#endif

VCMI_LIB_NAMESPACE_BEGIN

class CModHandler;
class CModIndentifier;
class CModInfo;
class JsonNode;
class IHandlerBase;

/// class that stores all object identifiers strings and maps them to numeric ID's
/// if possible, objects ID's should be in format <type>.<name>, camelCase e.g. "creature.grandElf"
class CIdentifierStorage
{
	enum ELoadingState
	{
		LOADING,
		FINALIZING,
		FINISHED
	};

	struct ObjectCallback // entry created on ID request
	{
		std::string localScope;  /// scope from which this ID was requested
		std::string remoteScope; /// scope in which this object must be found
		std::string type;        /// type, e.g. creature, faction, hero, etc
		std::string name;        /// string ID
		std::function<void(si32)> callback;
		bool optional;

		ObjectCallback(std::string localScope, std::string remoteScope,
		               std::string type, std::string name,
		               const std::function<void(si32)> & callback,
		               bool optional);
	};

	struct ObjectData // entry created on ID registration
	{
		si32 id;
		std::string scope; /// scope in which this ID located

		bool operator==(const ObjectData & other) const
		{
			return id == other.id && scope == other.scope;
		}

		template <typename Handler> void serialize(Handler &h, const int version)
		{
			h & id;
			h & scope;
		}
	};

	std::multimap<std::string, ObjectData> registeredObjects;
	std::vector<ObjectCallback> scheduledRequests;

	ELoadingState state;

	/// Check if identifier can be valid (camelCase, point as separator)
	void checkIdentifier(std::string & ID);

	void requestIdentifier(ObjectCallback callback);
	bool resolveIdentifier(const ObjectCallback & callback);
	std::vector<ObjectData> getPossibleIdentifiers(const ObjectCallback & callback);
public:
	CIdentifierStorage();
	virtual ~CIdentifierStorage();
	/// request identifier for specific object name.
	/// Function callback will be called during ID resolution phase of loading
	void requestIdentifier(std::string scope, std::string type, std::string name, const std::function<void(si32)> & callback);
	///fullName = [remoteScope:]type.name
	void requestIdentifier(std::string scope, std::string fullName, const std::function<void(si32)> & callback);
	void requestIdentifier(std::string type, const JsonNode & name, const std::function<void(si32)> & callback);
	void requestIdentifier(const JsonNode & name, const std::function<void(si32)> & callback);

	/// try to request ID. If ID with such name won't be loaded, callback function will not be called
	void tryRequestIdentifier(std::string scope, std::string type, std::string name, const std::function<void(si32)> & callback);
	void tryRequestIdentifier(std::string type, const JsonNode & name, const std::function<void(si32)> & callback);

	/// get identifier immediately. If identifier is not know and not silent call will result in error message
	boost::optional<si32> getIdentifier(std::string scope, std::string type, std::string name, bool silent = false);
	boost::optional<si32> getIdentifier(std::string type, const JsonNode & name, bool silent = false);
	boost::optional<si32> getIdentifier(const JsonNode & name, bool silent = false);
	boost::optional<si32> getIdentifier(std::string scope, std::string fullName, bool silent = false);

	/// registers new object
	void registerObject(std::string scope, std::string type, std::string name, si32 identifier);

	/// called at the very end of loading to check for any missing ID's
	void finalize();

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & registeredObjects;
		h & state;
	}
};

/// internal type to handle loading of one data type (e.g. artifacts, creatures)
class DLL_LINKAGE ContentTypeHandler
{
public:
	struct ModInfo
	{
		/// mod data from this mod and for this mod
		JsonNode modData;
		/// mod data for this mod from other mods (patches)
		JsonNode patches;
	};
	/// handler to which all data will be loaded
	IHandlerBase * handler;
	std::string objectName;

	/// contains all loaded H3 data
	std::vector<JsonNode> originalData;
	std::map<std::string, ModInfo> modData;

	ContentTypeHandler(IHandlerBase * handler, std::string objectName);

	/// local version of methods in ContentHandler
	/// returns true if loading was successful
	bool preloadModData(std::string modName, std::vector<std::string> fileList, bool validate);
	bool loadMod(std::string modName, bool validate);
	void loadCustom();
	void afterLoadFinalization();
};

/// class used to load all game data into handlers. Used only during loading
class DLL_LINKAGE CContentHandler
{
	/// preloads all data from fileList as data from modName.
	bool preloadModData(std::string modName, JsonNode modConfig, bool validate);

	/// actually loads data in mod
	bool loadMod(std::string modName, bool validate);

	std::map<std::string, ContentTypeHandler> handlers;
public:
	CContentHandler();

	void init();

	/// preloads all data from fileList as data from modName.
	void preloadData(CModInfo & mod);

	/// actually loads data in mod
	void load(CModInfo & mod);

	void loadCustom();

	/// all data was loaded, time for final validation / integration
	void afterLoadFinalization();

	const ContentTypeHandler & operator[] (const std::string & name) const;
};

typedef std::string TModID;

class DLL_LINKAGE CModInfo
{
public:
	enum EValidationStatus
	{
		PENDING,
		FAILED,
		PASSED
	};
	
	struct Version
	{
		int major = 0;
		int minor = 0;
		int patch = 0;
		
		Version() = default;
		Version(int mj, int mi, int p): major(mj), minor(mi), patch(p) {}
		
		static Version GameVersion();
		static Version fromString(std::string from);
		std::string toString() const;
		
		bool compatible(const Version & other, bool checkMinor = false, bool checkPatch = false) const;
		bool isNull() const;
		
		template <typename Handler> void serialize(Handler &h, const int version)
		{
			h & major;
			h & minor;
			h & patch;
		}
	};

	/// identifier, identical to name of folder with mod
	std::string identifier;

	/// human-readable strings
	std::string name;
	std::string description;
	
	/// version of the mod
	Version version;
	
	/// vcmi versions compatible with the mod

	Version vcmiCompatibleMin, vcmiCompatibleMax;

	/// list of mods that should be loaded before this one
	std::set <TModID> dependencies;

	/// list of mods that can't be used in the same time as this one
	std::set <TModID> conflicts;

	/// CRC-32 checksum of the mod
	ui32 checksum;

	/// true if mod is enabled
	bool enabled;

	EValidationStatus validation;

	JsonNode config;

	CModInfo();
	CModInfo(std::string identifier, const JsonNode & local, const JsonNode & config);

	JsonNode saveLocalData() const;
	void updateChecksum(ui32 newChecksum);

	static std::string getModDir(std::string name);
	static std::string getModFile(std::string name);

private:
	void loadLocalData(const JsonNode & data);
};

class DLL_LINKAGE CModHandler
{
	std::map <TModID, CModInfo> allMods;
	std::vector <TModID> activeMods;//active mods, in order in which they were loaded
	CModInfo coreMod;

	void loadConfigFromFile(std::string name);

	bool hasCircularDependency(TModID mod, std::set <TModID> currentList = std::set <TModID>()) const;

	//returns false if mod list is incorrect and prints error to console. Possible errors are:
	// - missing dependency mod
	// - conflicting mod in load order
	// - circular dependencies
	bool checkDependencies(const std::vector <TModID> & input) const;

	/**
	* 1. Set apart mods with resolved dependencies from mods which have unresolved dependencies
	* 2. Sort resolved mods using topological algorithm
	* 3. Log all problem mods and their unresolved dependencies
	*
	* @param modsToResolve list of valid mod IDs (checkDependencies returned true - TODO: Clarify it.)
	* @return a vector of the topologically sorted resolved mods: child nodes (dependent mods) have greater index than parents
	*/
	std::vector <TModID> validateAndSortDependencies(std::vector <TModID> modsToResolve) const;

	std::vector<std::string> getModList(std::string path);
	void loadMods(std::string path, std::string parent, const JsonNode & modSettings, bool enableMods);
	void loadOneMod(std::string modName, std::string parent, const JsonNode & modSettings, bool enableMods);
	void loadTranslation(TModID modName);
public:

	/// returns true if scope is reserved for internal use and can not be used by mods
	static bool isScopeReserved(const TModID & scope);

	/// reserved scope name for referencing built-in (e.g. H3) objects
	static const TModID & scopeBuiltin();

	/// reserved scope name for accessing objects from any loaded mod
	static const TModID & scopeGame();

	/// reserved scope name for accessing object for map loading
	static const TModID & scopeMap();

	class DLL_LINKAGE Incompatibility: public std::exception
	{
	public:
		using StringPair = std::pair<const std::string, const std::string>;
		using ModList = std::list<StringPair>;
		
		Incompatibility(ModList && _missingMods):
			missingMods(std::move(_missingMods))
		{
			std::ostringstream _ss;
			for(auto & m : missingMods)
				_ss << m.first << ' ' << m.second << std::endl;
			message = _ss.str();
		}
		
		const char * what() const noexcept override
		{
			return message.c_str();
		}
		
	private:
		//list of mods required to load the game
		// first: mod name
		// second: mod version
		const ModList missingMods;
		std::string message;
	};

	CIdentifierStorage identifiers;

	std::shared_ptr<CContentHandler> content; //(!)Do not serialize

	/// receives list of available mods and trying to load mod.json from all of them
	void initializeConfig();
	void loadMods(bool onlyEssential = false);
	void loadModFilesystems();

	std::set<TModID> getModDependencies(TModID modId, bool & isModFound);

	/// returns list of all (active) mods
	std::vector<std::string> getAllMods();
	std::vector<std::string> getActiveMods();

	/// load content from all available mods
	void load();
	void afterLoad(bool onlyEssential);

	struct DLL_LINKAGE hardcodedFeatures
	{
		JsonNode data;

		int CREEP_SIZE; // neutral stacks won't grow beyond this number
		int WEEKLY_GROWTH; //percent
		int NEUTRAL_STACK_EXP;
		int MAX_BUILDING_PER_TURN;
		bool DWELLINGS_ACCUMULATE_CREATURES;
		bool ALL_CREATURES_GET_DOUBLE_MONTHS;
		int MAX_HEROES_AVAILABLE_PER_PLAYER;
		int MAX_HEROES_ON_MAP_PER_PLAYER;
		bool WINNING_HERO_WITH_NO_TROOPS_RETREATS;
		bool BLACK_MARKET_MONTHLY_ARTIFACTS_CHANGE;
		bool NO_RANDOM_SPECIAL_WEEKS_AND_MONTHS;
		double ATTACK_POINT_DMG_MULTIPLIER;
		double ATTACK_POINTS_DMG_MULTIPLIER_CAP;
		double DEFENSE_POINT_DMG_MULTIPLIER;
		double DEFENSE_POINTS_DMG_MULTIPLIER_CAP;

		template <typename Handler> void serialize(Handler &h, const int version)
		{
			h & data;
			h & CREEP_SIZE;
			h & WEEKLY_GROWTH;
			h & NEUTRAL_STACK_EXP;
			h & MAX_BUILDING_PER_TURN;
			h & DWELLINGS_ACCUMULATE_CREATURES;
			h & ALL_CREATURES_GET_DOUBLE_MONTHS;
			h & MAX_HEROES_AVAILABLE_PER_PLAYER;
			h & MAX_HEROES_ON_MAP_PER_PLAYER;
			h & WINNING_HERO_WITH_NO_TROOPS_RETREATS;
			h & BLACK_MARKET_MONTHLY_ARTIFACTS_CHANGE;
			h & NO_RANDOM_SPECIAL_WEEKS_AND_MONTHS;
			h & ATTACK_POINT_DMG_MULTIPLIER;
			h & ATTACK_POINTS_DMG_MULTIPLIER_CAP;
			h & DEFENSE_POINT_DMG_MULTIPLIER;
			h & DEFENSE_POINTS_DMG_MULTIPLIER_CAP;
		}
	} settings;

	struct DLL_LINKAGE gameModules
	{
		bool STACK_EXP;
		bool STACK_ARTIFACT;
		bool COMMANDERS;
		bool MITHRIL;

		template <typename Handler> void serialize(Handler &h, const int version)
		{
			h & STACK_EXP;
			h & STACK_ARTIFACT;
			h & COMMANDERS;
			h & MITHRIL;
		}
	} modules;

	CModHandler();
	virtual ~CModHandler();

	static std::string normalizeIdentifier(const std::string & scope, const std::string & remoteScope, const std::string & identifier);

	static void parseIdentifier(const std::string & fullIdentifier, std::string & scope, std::string & type, std::string & identifier);

	static std::string makeFullIdentifier(const std::string & scope, const std::string & type, const std::string & identifier);

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		if(h.saving)
		{
			h & activeMods;
			for(const auto & m : activeMods)
				h & allMods[m].version;
		}
		else
		{
			loadMods();
			std::vector<TModID> newActiveMods;
			h & newActiveMods;
			
			Incompatibility::ModList missingMods;
			for(const auto & m : newActiveMods)

			{
				CModInfo::Version mver;
				h & mver;
				
				if(allMods.count(m) && (allMods[m].version.isNull() || mver.isNull() || allMods[m].version.compatible(mver)))
					allMods[m].enabled = true;
				else
					missingMods.emplace_back(m, mver.toString());
			}
			
			if(!missingMods.empty())
				throw Incompatibility(std::move(missingMods));
			
			std::swap(activeMods, newActiveMods);
		}
				
		h & settings;
		h & modules;
		h & identifiers;
	}
};

VCMI_LIB_NAMESPACE_END
