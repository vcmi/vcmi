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
	, config(std::make_unique<JsonNode>(config))
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

	return getConfig()["language"].isString() ? getConfig()["language"].String() : defaultLanguage;
}

const std::string & ModDescription::getName() const
{
	return getConfig()["name"].String();
}

const JsonNode & ModDescription::getFilesystemConfig() const
{
	return getConfig()["filesystem"];
}

const JsonNode & ModDescription::getConfig() const
{
	return *config;
}

CModVersion ModDescription::getVersion() const
{
	return CModVersion::fromString(getConfig()["version"].String());
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
	return getConfig()["modType"].String() == "Compatibility";
}

bool ModDescription::isTranslation() const
{
	return getConfig()["modType"].String() == "Translation";
}

bool ModDescription::keepDisabled() const
{
	return getConfig()["keepDisabled"].Bool();
}

bool ModDescription::affectsGameplay() const
{
	return true; // TODO
}

VCMI_LIB_NAMESPACE_END
