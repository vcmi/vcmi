/*
 * TextLocalizationContainer.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "TextLocalizationContainer.h"

#include "texts/CGeneralTextHandler.h"
#include "Languages.h"
#include "TextOperations.h"

#include "../VCMI_Lib.h"
#include "../json/JsonNode.h"
#include "../modding/CModHandler.h"

VCMI_LIB_NAMESPACE_BEGIN

std::recursive_mutex TextLocalizationContainer::globalTextMutex;

void TextLocalizationContainer::registerStringOverride(const std::string & modContext, const std::string & language, const TextIdentifier & UID, const std::string & localized)
{
	std::lock_guard globalLock(globalTextMutex);

	assert(!modContext.empty());
	assert(!language.empty());

	// NOTE: implicitly creates entry, intended - strings added by maps, campaigns, vcmi and potentially - UI mods are not registered anywhere at the moment
	auto & entry = stringsLocalizations[UID.get()];

	entry.overrideLanguage = language;
	entry.overrideValue = localized;
	if (entry.modContext.empty())
		entry.modContext = modContext;
}

void TextLocalizationContainer::addSubContainer(const TextLocalizationContainer & container)
{
	std::lock_guard globalLock(globalTextMutex);

	assert(!vstd::contains(subContainers, &container));
	subContainers.push_back(&container);
}

void TextLocalizationContainer::removeSubContainer(const TextLocalizationContainer & container)
{
	std::lock_guard globalLock(globalTextMutex);

	assert(vstd::contains(subContainers, &container));

	subContainers.erase(std::remove(subContainers.begin(), subContainers.end(), &container), subContainers.end());
}

const std::string & TextLocalizationContainer::deserialize(const TextIdentifier & identifier) const
{
	std::lock_guard globalLock(globalTextMutex);

	if(stringsLocalizations.count(identifier.get()) == 0)
	{
		for(auto containerIter = subContainers.rbegin(); containerIter != subContainers.rend(); ++containerIter)
			if((*containerIter)->identifierExists(identifier))
				return (*containerIter)->deserialize(identifier);

		logGlobal->error("Unable to find localization for string '%s'", identifier.get());
		return identifier.get();
	}

	const auto & entry = stringsLocalizations.at(identifier.get());

	if (!entry.overrideValue.empty())
		return entry.overrideValue;
	return entry.baseValue;
}

void TextLocalizationContainer::registerString(const std::string & modContext, const TextIdentifier & UID, const std::string & localized, const std::string & language)
{
	std::lock_guard globalLock(globalTextMutex);

	assert(!modContext.empty());
	assert(!Languages::getLanguageOptions(language).identifier.empty());
	assert(UID.get().find("..") == std::string::npos); // invalid identifier - there is section that was evaluated to empty string
	//assert(stringsLocalizations.count(UID.get()) == 0); // registering already registered string?

	if(stringsLocalizations.count(UID.get()) > 0)
	{
		auto & value = stringsLocalizations[UID.get()];
		value.baseLanguage = language;
		value.baseValue = localized;
	}
	else
	{
		StringState value;
		value.baseLanguage = language;
		value.baseValue = localized;
		value.modContext = modContext;

		stringsLocalizations[UID.get()] = value;
	}
}

void TextLocalizationContainer::registerString(const std::string & modContext, const TextIdentifier & UID, const std::string & localized)
{
	assert(!getModLanguage(modContext).empty());
	registerString(modContext, UID, localized, getModLanguage(modContext));
}

bool TextLocalizationContainer::validateTranslation(const std::string & language, const std::string & modContext, const JsonNode & config) const
{
	std::lock_guard globalLock(globalTextMutex);

	bool allPresent = true;

	for(const auto & string : stringsLocalizations)
	{
		if (string.second.modContext != modContext)
			continue; // Not our mod

		if (string.second.overrideLanguage == language)
			continue; // Already translated

		if (string.second.baseLanguage == language && !string.second.baseValue.empty())
			continue; // Base string already uses our language

		if (string.second.baseLanguage.empty())
			continue; // String added in localization, not present in base language (e.g. maps/campaigns)

		if (config.Struct().count(string.first) > 0)
			continue;

		if (allPresent)
			logMod->warn("Translation into language '%s' in mod '%s' is incomplete! Missing lines:", language, modContext);

		std::string currentText;
		if (string.second.overrideValue.empty())
			currentText = string.second.baseValue;
		else
			currentText = string.second.overrideValue;

		logMod->warn(R"(    "%s" : "%s",)", string.first, TextOperations::escapeString(currentText));
		allPresent = false;
	}

	bool allFound = true;

	//	for(const auto & string : config.Struct())
	//	{
	//		if (stringsLocalizations.count(string.first) > 0)
	//			continue;
	//
	//		if (allFound)
	//			logMod->warn("Translation into language '%s' in mod '%s' has unused lines:", language, modContext);
	//
	//		logMod->warn(R"(    "%s" : "%s",)", string.first, TextOperations::escapeString(string.second.String()));
	//		allFound = false;
	//	}

	return allPresent && allFound;
}

void TextLocalizationContainer::loadTranslationOverrides(const std::string & language, const std::string & modContext, const JsonNode & config)
{
	for(const auto & node : config.Struct())
		registerStringOverride(modContext, language, node.first, node.second.String());
}

bool TextLocalizationContainer::identifierExists(const TextIdentifier & UID) const
{
	std::lock_guard globalLock(globalTextMutex);

	return stringsLocalizations.count(UID.get());
}

void TextLocalizationContainer::exportAllTexts(std::map<std::string, std::map<std::string, std::string>> & storage) const
{
	std::lock_guard globalLock(globalTextMutex);

	for (auto const & subContainer : subContainers)
		subContainer->exportAllTexts(storage);

	for (auto const & entry : stringsLocalizations)
	{
		std::string textToWrite;
		std::string modName = entry.second.modContext;

		if (modName.find('.') != std::string::npos)
			modName = modName.substr(0, modName.find('.'));

		if (!entry.second.overrideValue.empty())
			textToWrite = entry.second.overrideValue;
		else
			textToWrite = entry.second.baseValue;

		storage[modName][entry.first] = textToWrite;
	}
}

std::string TextLocalizationContainer::getModLanguage(const std::string & modContext)
{
	if (modContext == "core")
		return CGeneralTextHandler::getInstalledLanguage();
	return VLC->modh->getModLanguage(modContext);
}

void TextLocalizationContainer::jsonSerialize(JsonNode & dest) const
{
	std::lock_guard globalLock(globalTextMutex);

	for(auto & s : stringsLocalizations)
	{
		dest.Struct()[s.first].String() = s.second.baseValue;
		if(!s.second.overrideValue.empty())
			dest.Struct()[s.first].String() = s.second.overrideValue;
	}
}

TextContainerRegistrable::TextContainerRegistrable()
{
	VLC->generaltexth->addSubContainer(*this);
}

TextContainerRegistrable::~TextContainerRegistrable()
{
	VLC->generaltexth->removeSubContainer(*this);
}

TextContainerRegistrable::TextContainerRegistrable(const TextContainerRegistrable & other)
	: TextLocalizationContainer(other)
{
	VLC->generaltexth->addSubContainer(*this);
}

TextContainerRegistrable::TextContainerRegistrable(TextContainerRegistrable && other) noexcept
	:TextLocalizationContainer(other)
{
	VLC->generaltexth->addSubContainer(*this);
}


VCMI_LIB_NAMESPACE_END
