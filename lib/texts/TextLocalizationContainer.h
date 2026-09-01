/*
 * TextLocalizationContainer.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "ITranslator.h"
#include "TextIdentifier.h"

class JsonNode;

struct ExportedStrings
{
	/// Strings (string ID -> translation) that were added by this mod
	std::map<std::string, std::string> strings;

	/// mods that had one or more of their strings overriden by this mod
	std::vector<std::string> overridenMods;
};

class DLL_LINKAGE TextLocalizationContainer : public ITranslator
{
protected:
	struct StringState
	{
		/// Human-readable string that was added on registration
		std::string translatedText;

		/// ID of mod that created this string
		std::string identifierModContext;

		/// ID of mod that provides original, untranslated version of this string
		/// Different from identifierModContext if mod has modified object from another mod (e.g. rebalance mods)
		std::string baseStringModContext;

		bool overriden = false;

		template <typename Handler>
		void serialize(Handler & h)
		{
			h & translatedText;
			h & identifierModContext;
			h & baseStringModContext;
		}
	};

	/// map identifier -> localization
	std::unordered_map<std::string, StringState> stringsLocalizations;

	/// add selected string to internal storage as high-priority strings
	void registerStringOverride(const std::string & modContext, const TextIdentifier & UID, const std::string & localized, const std::string & language);

	std::string getModLanguage(const std::string & modContext) const;

public:
	/// returns true if identifier with such name was registered, even if not translated to current language
	bool identifierExists(const TextIdentifier & UID) const;

	/// Loads translation from provided json
	/// Any entries loaded by this will have priority over texts registered normally
	void loadTranslationOverrides(const std::string & modContext, const std::string & language, JsonNode const & file);

	/// add selected string to internal storage
	void registerString(const std::string & modContext, const TextIdentifier & UID, const JsonNode & localized);
	void registerString(const std::string & modContext, const TextIdentifier & UID, const std::string & localized);
	void registerString(const std::string & identifierModContext, const std::string & localizedStringModContext, const TextIdentifier & UID, const std::string & localized);

	/// converts identifier into user-readable string
	const std::string & translateString(const TextIdentifier & identifier) const override;

	/// Debug method, returns all currently stored texts
	/// Format: [mod ID][string ID] -> human-readable text
	void exportAllTexts(std::map<std::string, ExportedStrings> & storage, bool onlyMissing) const;

	void jsonSerialize(JsonNode & dest) const;

	template <typename Handler>
	void serialize(Handler & h)
	{
		h & stringsLocalizations;
	}

protected:
	/// Map and campaign overlays legitimately re-register strings as the game renames things.
	/// The static store is written once during load, so a second write there is a bug.
	virtual bool allowsStringOverride() const { return true; }
};
