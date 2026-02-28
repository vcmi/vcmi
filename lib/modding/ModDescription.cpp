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


static std::string normalizeDescriptionMarkup(std::string text)
{
	// Keep this normalization in lib layer: launcher/Qt conversion is not available here.
	// Handle both <br> and self-closing variants (<br/>, <br />).
	text = std::regex_replace(text, std::regex(R"(<\s*br\s*/?\s*>)", std::regex_constants::icase), "\n");

	// Convert only the most useful inline tags before stripping the remaining HTML.
	text = std::regex_replace(text, std::regex(R"(<\s*(b|strong)(\s+[^>]*)?>)", std::regex_constants::icase), "**");
	text = std::regex_replace(text, std::regex(R"(<\s*/\s*(b|strong)\s*>)", std::regex_constants::icase), "**");
	text = std::regex_replace(text, std::regex(R"(<\s*(i|em)(\s+[^>]*)?>)", std::regex_constants::icase), "*");
	text = std::regex_replace(text, std::regex(R"(<\s*/\s*(i|em)\s*>)", std::regex_constants::icase), "*");

	// Strip any remaining tags to avoid leaking raw HTML into UI markdown renderer.
	static const std::regex htmlTagPattern("<[^>]+>");
	text = std::regex_replace(text, htmlTagPattern, "");

	return text;
}


void ModDescription::mergeModDescriptions(JsonNode & modConfig, const std::string & fullDescription)
{
	if (modConfig["description"].isString())
		modConfig["description"].String() = normalizeDescriptionMarkup(modConfig["description"].String());

	for (const auto & language : Languages::getLanguageList())
	{
		if (modConfig[language.identifier]["description"].isString())
			modConfig[language.identifier]["description"].String() = normalizeDescriptionMarkup(modConfig[language.identifier]["description"].String());
	}

	if (fullDescription.empty())
		return;

	// description.md is already markdown/plain text; keep it verbatim.
	if (fullDescription.find('#') == std::string::npos)
	{
		modConfig["description"].String() = fullDescription;
		return;
	}

	static const std::set<std::string> knownLanguages = []()
	{
		std::set<std::string> result;
		for (const auto & language : Languages::getLanguageList())
			result.insert(boost::algorithm::to_lower_copy(language.identifier));
		return result;
	}();
	
	std::map<std::string, std::string> sections;
	std::string currentLanguage;
	std::string currentContent;
	bool hasKnownLanguageHeader = false;
	const std::string baseLanguage = boost::algorithm::to_lower_copy(modConfig.Struct().count("language") ? modConfig["language"].String() : "english");

	auto flushCurrentSection = [&]()
	{
		if (!currentLanguage.empty())
			sections[currentLanguage] = currentContent;
		currentLanguage.clear();
		currentContent.clear();
	};

	std::string_view fullDescriptionView(fullDescription);
	bool firstLine = true;
	for (size_t lineStart = 0; lineStart <= fullDescriptionView.size();)
	{
		size_t lineEnd = fullDescriptionView.find('\n', lineStart);
		if (lineEnd == std::string_view::npos)
			lineEnd = fullDescriptionView.size();

		std::string_view lineView = fullDescriptionView.substr(lineStart, lineEnd - lineStart);
		if (!lineView.empty() && lineView.back() == '\r')
			lineView.remove_suffix(1);

		if (firstLine)
		{
			// UTF-8 BOM, if present
			if (lineView.size() >= 3 &&
				static_cast<unsigned char>(lineView[0]) == 0xEF &&
				static_cast<unsigned char>(lineView[1]) == 0xBB &&
				static_cast<unsigned char>(lineView[2]) == 0xBF)
			{
				lineView.remove_prefix(3);
			}
			firstLine = false;
		}

		if (!lineView.empty() && lineView.front() == '#')
		{
			std::string languageID(lineView.substr(1));
			boost::trim(languageID);
			boost::to_lower(languageID);

			if (knownLanguages.count(languageID) != 0)
			{
				flushCurrentSection();
				currentLanguage = languageID;
				hasKnownLanguageHeader = true;
				if (lineEnd == fullDescriptionView.size())
					break;
				lineStart = lineEnd + 1;
				continue;
			}
		}

		if (!currentLanguage.empty())
		{
			currentContent.append(lineView);
			currentContent.push_back('\n');
		}

		if (lineEnd == fullDescriptionView.size())
			break;
		lineStart = lineEnd + 1;
	}

	flushCurrentSection();

	if (!hasKnownLanguageHeader)
	{
		modConfig["description"].String() = fullDescription;
		return;
	}

	for (const auto & [languageID, description] : sections)
	{
		if (languageID != baseLanguage)
			modConfig[languageID]["description"].String() = description;
	}

	if (sections.count(baseLanguage) != 0)
		modConfig["description"].String() = sections[baseLanguage];
	else if (sections.count("english") != 0)
		modConfig["description"].String() = sections["english"];
	else
		modConfig["description"].String() = sections.begin()->second;
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
	{
		dependencies.emplace(getParentID());
		if (getTopParentID() != getParentID())
			dependencies.emplace(getTopParentID());
	}
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
