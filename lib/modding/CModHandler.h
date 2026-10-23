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

#include "../json/JsonNode.h"

class CModHandler;
class ModDescription;
class ResourcePath;
class ModManager;
class ISimpleResourceLoader;
class LibraryLoadContext;

using TModID = std::string;

class DLL_LINKAGE CModHandler final : boost::noncopyable
{
	std::unique_ptr<ModManager> modManager;

	/// Mods from which at least one identifier request has failed to resolve while loading library content
	std::vector<TModID> modsWithFailedRequests;

	/// Reads files of all active mods and computes their checksums
	void loadModContent(LibraryLoadContext & context) const;
	JsonNode loadModContent(const TModID & modName, const std::vector<std::string> & contentTypes, const std::string & preferredLanguage, uint32_t & checksum, bool & isValid) const;

	void loadTranslation(LibraryLoadContext & context, const TModID & modName);
	void checkModFilesystemsConflicts(const std::map<TModID, std::unique_ptr<ISimpleResourceLoader>> & modFilesystems);

	bool isModValidationNeeded(const LibraryLoadContext & context, const ModDescription & mod) const;

public:
	/// receives list of available mods and trying to load mod.json from all of them
	void initializeConfig(LibraryLoadContext & context);
	void loadModFilesystems();

	/// returns ID of mod that provides selected file resource
	TModID findResourceOrigin(const ResourcePath & name) const;

	/// Returns assumed language ID of mod that provides selected file resource
	std::string findResourceLanguage(const ResourcePath & name) const;

	/// Returns assumed encoding of language of mod that provides selected file resource
	std::string findResourceEncoding(const ResourcePath & name) const;

	/// Same as findResourceEncoding, for callers that have already located the providing mod
	std::string getResourceEncoding(const ResourcePath & name, const TModID & modName) const;

	std::string getModLanguage(const TModID & modId) const;

	const std::set<TModID> & getModDependencies(const TModID & modId) const;
	const std::set<TModID> & getModDependencies(const TModID & modId, bool & isModFound) const;
	std::set<TModID> getModSoftDependencies(const TModID & modId) const;
	std::set<TModID> getModEnabledSoftDependencies(const TModID & modId) const;

	/// returns list of all (active) mods
	std::vector<std::string> getAllMods() const;
	const std::vector<std::string> & getActiveMods() const;

	const ModDescription & getModInfo(const TModID & modId) const;

	/// load content from all available mods
	void load(LibraryLoadContext & context);
	void afterLoad(const LibraryLoadContext & context);

	/// Returns list of all mods, from which at least one identifier has failed to resolve
	const std::vector<TModID> & getModsWithFailedRequests() const;

	explicit CModHandler(bool useTestPreset = false);
	~CModHandler();
};
