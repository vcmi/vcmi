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
class ModDescription;
class CContentHandler;
class ResourcePath;
class ModManager;
class ISimpleResourceLoader;

using TModID = std::string;

class DLL_LINKAGE CModHandler final : boost::noncopyable
{
	std::unique_ptr<ModManager> modManager;
	std::map<std::string, uint32_t> modChecksums;
	std::set<std::string> validationPassed;

	void loadTranslation(const TModID & modName);
	void checkModFilesystemsConflicts(const std::map<TModID, ISimpleResourceLoader *> & modFilesystems);

	bool isModValidationNeeded(const ModDescription & mod) const;

public:
	std::shared_ptr<CContentHandler> content;

	/// receives list of available mods and trying to load mod.json from all of them
	void initializeConfig();
	void loadModFilesystems();

	/// returns ID of mod that provides selected file resource
	TModID findResourceOrigin(const ResourcePath & name) const;

	/// Returns assumed language ID of mod that provides selected file resource
	std::string findResourceLanguage(const ResourcePath & name) const;

	/// Returns assumed encoding of language of mod that provides selected file resource
	std::string findResourceEncoding(const ResourcePath & name) const;

	std::string getModLanguage(const TModID & modId) const;

	std::set<TModID> getModDependencies(const TModID & modId) const;
	std::set<TModID> getModDependencies(const TModID & modId, bool & isModFound) const;
	std::set<TModID> getModSoftDependencies(const TModID & modId) const;
	std::set<TModID> getModEnabledSoftDependencies(const TModID & modId) const;

	/// returns list of all (active) mods
	std::vector<std::string> getAllMods() const;
	const std::vector<std::string> & getActiveMods() const;

	const ModDescription & getModInfo(const TModID & modId) const;

	/// load content from all available mods
	void load();
	void afterLoad();

	CModHandler();
	~CModHandler();
};

VCMI_LIB_NAMESPACE_END
