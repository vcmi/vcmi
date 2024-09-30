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

void TextLocalizationContainer::registerStringOverride(const std::string & modContext, const TextIdentifier & UID, const std::string & localized)
{
	std::lock_guard globalLock(globalTextMutex);

	assert(!modContext.empty());

	// NOTE: implicitly creates entry, intended - strings added by maps, campaigns, vcmi and potentially - UI mods are not registered anywhere at the moment
	auto & entry = stringsLocalizations[UID.get()];

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

void TextLocalizationContainer::registerString(const std::string & modContext, const TextIdentifier & inputUID, const JsonNode & localized)
{
	assert(!localized.getModScope().empty());
	assert(!getModLanguage(localized.getModScope()).empty());

	std::lock_guard globalLock(globalTextMutex);
	registerString(modContext, inputUID, localized.String(), getModLanguage(modContext));
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
		value.baseValue = localized;
	}
	else
	{
		StringState value;
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

void TextLocalizationContainer::loadTranslationOverrides(const std::string & modContext, const JsonNode & config)
{
	for(const auto & node : config.Struct())
		registerStringOverride(modContext, node.first, node.second.String());
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
