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

#include "TextIdentifier.h"

VCMI_LIB_NAMESPACE_BEGIN

class JsonNode;

class DLL_LINKAGE TextLocalizationContainer
{
protected:
	static std::recursive_mutex globalTextMutex;

	struct StringState
	{
		/// Human-readable string that was added on registration
		std::string baseValue;

		/// Language of base string
		std::string baseLanguage;

		/// Translated human-readable string
		std::string overrideValue;

		/// Language of the override string
		std::string overrideLanguage;

		/// ID of mod that created this string
		std::string modContext;

		template <typename Handler>
		void serialize(Handler & h)
		{
			h & baseValue;
			h & baseLanguage;
			h & modContext;
		}
	};

	/// map identifier -> localization
	std::unordered_map<std::string, StringState> stringsLocalizations;

	std::vector<const TextLocalizationContainer *> subContainers;

	/// add selected string to internal storage as high-priority strings
	void registerStringOverride(const std::string & modContext, const std::string & language, const TextIdentifier & UID, const std::string & localized);

	std::string getModLanguage(const std::string & modContext);

	// returns true if identifier with such name was registered, even if not translated to current language
	bool identifierExists(const TextIdentifier & UID) const;

public:
	/// validates translation of specified language for specified mod
	/// returns true if localization is valid and complete
	/// any error messages will be written to log file
	bool validateTranslation(const std::string & language, const std::string & modContext, JsonNode const & file) const;

	/// Loads translation from provided json
	/// Any entries loaded by this will have priority over texts registered normally
	void loadTranslationOverrides(const std::string & language, const std::string & modContext, JsonNode const & file);

	/// add selected string to internal storage
	void registerString(const std::string & modContext, const TextIdentifier & UID, const std::string & localized);
	void registerString(const std::string & modContext, const TextIdentifier & UID, const std::string & localized, const std::string & language);

	/// returns translated version of a string that can be displayed to user
	template<typename  ... Args>
	std::string translate(std::string arg1, Args ... args) const
	{
		TextIdentifier id(arg1, args ...);
		return deserialize(id);
	}

	/// converts identifier into user-readable string
	const std::string & deserialize(const TextIdentifier & identifier) const;

	/// Debug method, returns all currently stored texts
	/// Format: [mod ID][string ID] -> human-readable text
	void exportAllTexts(std::map<std::string, std::map<std::string, std::string>> & storage) const;

	/// Add or override subcontainer which can store identifiers
	void addSubContainer(const TextLocalizationContainer & container);

	/// Remove subcontainer with give name
	void removeSubContainer(const TextLocalizationContainer & container);

	void jsonSerialize(JsonNode & dest) const;

	template <typename Handler>
	void serialize(Handler & h)
	{
		std::lock_guard globalLock(globalTextMutex);

		if (h.version >= Handler::Version::SIMPLE_TEXT_CONTAINER_SERIALIZATION)
		{
			h & stringsLocalizations;
		}
		else
		{
			std::string key;
			int64_t sz = stringsLocalizations.size();

			if (h.version >= Handler::Version::REMOVE_TEXT_CONTAINER_SIZE_T)
			{
				int64_t size = sz;
				h & size;
				sz = size;
			}
			else
			{
				h & sz;
			}

			if(h.saving)
			{
				for(auto & s : stringsLocalizations)
				{
					key = s.first;
					h & key;
					h & s.second;
				}
			}
			else
			{
				for(size_t i = 0; i < sz; ++i)
				{
					h & key;
					h & stringsLocalizations[key];
				}
			}
		}
	}
};

class DLL_LINKAGE TextContainerRegistrable : public TextLocalizationContainer
{
public:
	TextContainerRegistrable();
	~TextContainerRegistrable();

	TextContainerRegistrable(const TextContainerRegistrable & other);
	TextContainerRegistrable(TextContainerRegistrable && other) noexcept;

	TextContainerRegistrable& operator=(const TextContainerRegistrable & b) = default;
};

VCMI_LIB_NAMESPACE_END
