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

	// load string override only in following cases:
	// a) string was not modified in another mod (e.g. rebalance mod gave skill new description)
	// b) this string override is defined in the same mod as one that provided modified version of this string
	if (entry.identifierModContext == entry.baseStringModContext || modContext == entry.baseStringModContext)
	{
		entry.translatedText = localized;
		if (entry.identifierModContext.empty())
		{
			entry.identifierModContext = modContext;
			entry.baseStringModContext = modContext;
		}
	}
	else
	{
		logGlobal->debug("Skipping translation override for string %s: changed in a different mod", UID.get());
	}
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

const std::string & TextLocalizationContainer::translateString(const TextIdentifier & identifier) const
{
	std::lock_guard globalLock(globalTextMutex);

	if(stringsLocalizations.count(identifier.get()) == 0)
	{
		for(auto containerIter = subContainers.rbegin(); containerIter != subContainers.rend(); ++containerIter)
			if((*containerIter)->identifierExists(identifier))
				return (*containerIter)->translateString(identifier);

		logGlobal->error("Unable to find localization for string '%s'", identifier.get());
		return identifier.get();
	}

	const auto & entry = stringsLocalizations.at(identifier.get());
	return entry.translatedText;
}

void TextLocalizationContainer::registerString(const std::string & modContext, const TextIdentifier & inputUID, const JsonNode & localized)
{
	assert(localized.isNull() || !localized.getModScope().empty());
	assert(localized.isNull() || !getModLanguage(localized.getModScope()).empty());

	if (localized.isNull())
		registerString(modContext, modContext, inputUID, localized.String());
	else
		registerString(modContext, localized.getModScope(), inputUID, localized.String());
}

void TextLocalizationContainer::registerString(const std::string & modContext, const TextIdentifier & UID, const std::string & localized)
{
	registerString(modContext, modContext, UID, localized);
}

void TextLocalizationContainer::registerString(const std::string & identifierModContext, const std::string & localizedStringModContext, const TextIdentifier & UID, const std::string & localized)
{
	std::lock_guard globalLock(globalTextMutex);

	assert(!identifierModContext.empty());
	assert(!localizedStringModContext.empty());
	assert(UID.get().find("..") == std::string::npos); // invalid identifier - there is section that was evaluated to empty string
	assert(stringsLocalizations.count(UID.get()) == 0 || identifierModContext == "map"); // registering already registered string?

	if(stringsLocalizations.count(UID.get()) > 0)
	{
		auto & value = stringsLocalizations[UID.get()];
		value.translatedText = localized;
		value.identifierModContext = identifierModContext;
		value.baseStringModContext = localizedStringModContext;
	}
	else
	{
		StringState value;
		value.translatedText = localized;
		value.identifierModContext = identifierModContext;
		value.baseStringModContext = localizedStringModContext;

		stringsLocalizations[UID.get()] = value;
	}
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
		std::string modName = entry.second.baseStringModContext;

		if (entry.second.baseStringModContext == entry.second.identifierModContext)
		{
			if (modName.find('.') != std::string::npos)
				modName = modName.substr(0, modName.find('.'));
		}
		boost::range::replace(modName, '.', '_');

		textToWrite = entry.second.translatedText;

		if (!textToWrite.empty())
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
		dest.Struct()[s.first].String() = s.second.translatedText;
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
