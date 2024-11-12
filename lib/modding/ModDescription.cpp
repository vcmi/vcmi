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

VCMI_LIB_NAMESPACE_BEGIN

ModDescription::ModDescription(const TModID & fullID, const JsonNode & config)
	: identifier(fullID)
	, localConfig(std::make_unique<JsonNode>(config))
	, dependencies(loadModList(config["depends"]))
	, softDependencies(loadModList(config["softDepends"]))
	, conflicts(loadModList(config["conflicts"]))
{
	if(getID() != "core")
		dependencies.insert("core");
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

	return getLocalConfig()["language"].isString() ? getLocalConfig()["language"].String() : defaultLanguage;
}

const std::string & ModDescription::getName() const
{
	return getLocalConfig()["name"].String();
}

const JsonNode & ModDescription::getFilesystemConfig() const
{
	return getLocalConfig()["filesystem"];
}

const JsonNode & ModDescription::getLocalConfig() const
{
	return *localConfig;
}

const JsonNode & ModDescription::getValue(const std::string & keyName) const
{
	return getLocalConfig()[keyName];
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
	return CModVersion::fromString(getLocalConfig()["version"].String());
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

bool ModDescription::isCompatibility() const
{
	return getLocalConfig()["modType"].String() == "Compatibility";
}

bool ModDescription::isTranslation() const
{
	return getLocalConfig()["modType"].String() == "Translation";
}

bool ModDescription::keepDisabled() const
{
	return getLocalConfig()["keepDisabled"].Bool();
}

bool ModDescription::isInstalled() const
{
	return !localConfig->isNull();
}

bool ModDescription::affectsGameplay() const
{
	return true; // TODO
}

VCMI_LIB_NAMESPACE_END
