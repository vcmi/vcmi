/*
 * ModDescription.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "ModDescription.h"

#include "CModVersion.h"
#include "ModVerificationInfo.h"

#include "../json/JsonNode.h"
#include "../texts/CGeneralTextHandler.h"
#include "../texts/Languages.h"

VCMI_LIB_NAMESPACE_BEGIN

void ModDescription::mergeModDescriptions(JsonNode & modConfig, const std::string & fullDescription)
{
	std::vector<std::string> sections;
	boost::algorithm::iter_split(sections, fullDescription, boost::algorithm::first_finder("\n# "));

	for (const auto & section : sections)
	{
		size_t endOfFirstLine = section.find('\n', 1);
		if (endOfFirstLine == std::string::npos)
			continue;

		std::string firstLine = section.substr(0, endOfFirstLine);

		for (const auto & language : Languages::getLanguageList())
		{
			if (firstLine.find(language.identifier) == std::string::npos)
			   continue;

			bool baseLanguageDescription = language.identifier == "english";
			if (modConfig.Struct().count("language"))
				baseLanguageDescription = language.identifier == modConfig["language"].String();

			if (baseLanguageDescription)
				modConfig["description"].String() = section.substr(endOfFirstLine+1);
			else
				modConfig[language.identifier]["description"].String() = section.substr(endOfFirstLine+1);

			break;
		}
	}
}

ModDescription::ModDescription(const TModID & fullID, const JsonNode & localConfig, const JsonNode & repositoryConfig)
	: identifier(fullID)
	, localConfig(std::make_unique<JsonNode>(localConfig))
	, repositoryConfig(std::make_unique<JsonNode>(repositoryConfig))
	, dependencies(loadModList(getValue("depends")))
	, softDependencies(loadModList(getValue("softDepends")))
	, conflicts(loadModList(getValue("conflicts")))
{
	if(getID() != "core")
		dependencies.emplace("core");

	if (!getParentID().empty())
		dependencies.emplace(getParentID());
}

ModDescription::~ModDescription() = default;

TModSet ModDescription::loadModList(const JsonNode & configNode) const
{
	TModSet result;
	for(const auto & entry : configNode.Vector())
		result.insert(boost::algorithm::to_lower_copy(entry.String()));
	return result;
}

const TModID & ModDescription::getID() const
{
	return identifier;
}

TModID ModDescription::getParentID() const
{
	size_t dotPos = identifier.find_last_of('.');

	if(dotPos == std::string::npos)
		return {};

	return identifier.substr(0, dotPos);
}

TModID ModDescription::getTopParentID() const
{
	size_t dotPos = identifier.find('.');

	if(dotPos == std::string::npos)
		return {};

	return identifier.substr(0, dotPos);
}

const TModSet & ModDescription::getDependencies() const
{
	return dependencies;
}

const TModSet & ModDescription::getSoftDependencies() const
{
	return softDependencies;
}

const TModSet & ModDescription::getConflicts() const
{
	return conflicts;
}

const std::string & ModDescription::getBaseLanguage() const
{
	static const std::string defaultLanguage = "english";

	return getValue("language").isString() ? getValue("language").String() : defaultLanguage;
}

const std::string & ModDescription::getName() const
{
	return getLocalizedValue("name").String();
}

const JsonNode & ModDescription::getFilesystemConfig() const
{
	return getLocalValue("filesystem");
}

const JsonNode & ModDescription::getLocalConfig() const
{
	return *localConfig;
}

const JsonNode & ModDescription::getLocalizedValue(const std::string & keyName) const
{
	const std::string language = CGeneralTextHandler::getPreferredLanguage();
	const JsonNode & languageNode = getValue(language);
	const JsonNode & baseValue = getValue(keyName);
	const JsonNode & localizedValue = languageNode[keyName];

	if (localizedValue.isNull())
		return baseValue;
	else
		return localizedValue;
}

const JsonNode & ModDescription::getValue(const std::string & keyName) const
{
	if (!isInstalled() || isUpdateAvailable())
		return getRepositoryValue(keyName);
	else
		return getLocalValue(keyName);
}

const JsonNode & ModDescription::getLocalValue(const std::string & keyName) const
{
	return getLocalConfig()[keyName];
}

const JsonNode & ModDescription::getRepositoryValue(const std::string & keyName) const
{
	return (*repositoryConfig)[keyName];
}

CModVersion ModDescription::getVersion() const
{
	return CModVersion::fromString(getValue("version").String());
}

ModVerificationInfo ModDescription::getVerificationInfo() const
{
	ModVerificationInfo result;
	result.name = getName();
	result.version = getVersion();
	result.impactsGameplay = affectsGameplay();
	result.parent = getParentID();

	return result;
}

bool ModDescription::isCompatible() const
{
	const JsonNode & compatibility = getValue("compatibility");

	if (compatibility.isNull())
		return true;

	auto vcmiCompatibleMin = CModVersion::fromString(compatibility["min"].String());
	auto vcmiCompatibleMax = CModVersion::fromString(compatibility["max"].String());

	bool compatible = true;
	compatible &= (vcmiCompatibleMin.isNull() || CModVersion::GameVersion().compatible(vcmiCompatibleMin, true, true));
	compatible &= (vcmiCompatibleMax.isNull() || vcmiCompatibleMax.compatible(CModVersion::GameVersion(), true, true));

	return compatible;
}

bool ModDescription::isCompatibility() const
{
	return getValue("modType").String() == "Compatibility";
}

bool ModDescription::isTranslation() const
{
	return getValue("modType").String() == "Translation";
}

bool ModDescription::keepDisabled() const
{
	return getValue("keepDisabled").Bool();
}

bool ModDescription::isInstalled() const
{
	return !localConfig->isNull();
}

bool ModDescription::affectsGameplay() const
{
	static const std::array keysToTest = {
		"artifacts",
		"battlefields",
		"creatures",
		"factions",
		"heroClasses",
		"heroes",
		"objects",
		"obstacles",
		"mapLayers",
		"rivers",
		"roads",
		"settings",
		"skills",
		"spells",
		"terrains",
	};

	for(const auto & key : keysToTest)
		if (!getLocalValue(key).isNull())
			return true;

	return false;
}

bool ModDescription::isUpdateAvailable() const
{
	if (getRepositoryValue("version").isNull())
		return false;

	if (getLocalValue("version").isNull())
		return false;

	auto localVersion = CModVersion::fromString(getLocalValue("version").String());
	auto repositoryVersion = CModVersion::fromString(getRepositoryValue("version").String());

	return localVersion < repositoryVersion;
}

VCMI_LIB_NAMESPACE_END
