/*
 * CGeneralTextHandler.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "filesystem/ResourcePath.h"

VCMI_LIB_NAMESPACE_BEGIN

class CInputStream;
class JsonNode;
class JsonSerializeFormat;

/// Parser for any text files from H3
class DLL_LINKAGE CLegacyConfigParser
{
	std::string fileEncoding;

	std::unique_ptr<char[]> data;
	char * curr;
	char * end;

	/// extracts part of quoted string.
	std::string extractQuotedPart();

	/// extracts quoted string. Any end of lines are ignored, double-quote is considered as "escaping"
	std::string extractQuotedString();

	/// extracts non-quoted string
	std::string extractNormalString();

	/// reads "raw" string without encoding conversion
	std::string readRawString();

public:
	/// read one entry from current line. Return ""/0 if end of line reached
	std::string readString();
	float readNumber();

	template <typename numeric>
	std::vector<numeric> readNumArray(size_t size)
	{
		std::vector<numeric> ret;
		ret.reserve(size);
		while (size--)
			ret.push_back((numeric)readNumber());
		return ret;
	}

	/// returns true if next entry is empty
	bool isNextEntryEmpty() const;

	/// end current line
	bool endLine();

	explicit CLegacyConfigParser(const TextPath & URI);
};

class CGeneralTextHandler;

/// Small wrapper that provides text access API compatible with old code
class DLL_LINKAGE LegacyTextContainer
{
	CGeneralTextHandler & owner;
	std::string basePath;

public:
	LegacyTextContainer(CGeneralTextHandler & owner, std::string basePath);
	std::string operator [](size_t index) const;
};

/// Small wrapper that provides help text access API compatible with old code
class DLL_LINKAGE LegacyHelpContainer
{
	CGeneralTextHandler & owner;
	std::string basePath;

public:
	LegacyHelpContainer(CGeneralTextHandler & owner, std::string basePath);
	std::pair<std::string, std::string> operator[](size_t index) const;
};

class TextIdentifier
{
	std::string identifier;
public:
	const std::string & get() const 
	{
		return identifier;
	}

	TextIdentifier(const char * id):
		identifier(id)
	{}

	TextIdentifier(const std::string & id):
		identifier(id) 
	{}

	template<typename... T>
	TextIdentifier(const std::string & id, size_t index, T... rest):
		TextIdentifier(id + '.' + std::to_string(index), rest...)
	{}

	template<typename... T>
	TextIdentifier(const std::string & id, const std::string & id2, T... rest):
		TextIdentifier(id + '.' + id2, rest...)
	{}
};

class DLL_LINKAGE TextLocalizationContainer
{
protected:
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
	
public:
	/// validates translation of specified language for specified mod
	/// returns true if localization is valid and complete
	/// any error messages will be written to log file
	bool validateTranslation(const std::string & language, const std::string & modContext, JsonNode const & file) const;

	/// Loads translation from provided json
	/// Any entries loaded by this will have priority over texts registered normally
	void loadTranslationOverrides(const std::string & language, const std::string & modContext, JsonNode const & file);

	// returns true if identifier with such name was registered, even if not translated to current language
	bool identifierExists(const TextIdentifier & UID) const;
	
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
	
	/// Debug method, dumps all currently known texts into console using Json-like format
	void dumpAllTexts();
	
	/// Add or override subcontainer which can store identifiers
	void addSubContainer(const TextLocalizationContainer & container);
	
	/// Remove subcontainer with give name
	void removeSubContainer(const TextLocalizationContainer & container);
	
	void jsonSerialize(JsonNode & dest) const;
	
	template <typename Handler>
	void serialize(Handler & h)
	{
		std::string key;
		auto sz = stringsLocalizations.size();
		h & sz;
		if(h.saving)
		{
			for(auto s : stringsLocalizations)
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

/// Handles all text-related data in game
class DLL_LINKAGE CGeneralTextHandler: public TextLocalizationContainer
{
	void readToVector(const std::string & sourceID, const std::string & sourceName);

	/// number of scenarios in specific campaign. TODO: move to a better location
	std::vector<size_t> scenariosCountPerCampaign;

public:
	LegacyTextContainer allTexts;

	LegacyTextContainer arraytxt;
	LegacyTextContainer primarySkillNames;
	LegacyTextContainer jktexts;
	LegacyTextContainer heroscrn;
	LegacyTextContainer overview;//text for Kingdom Overview window
	LegacyTextContainer colors; //names of player colors ("red",...)
	LegacyTextContainer capColors; //names of player colors with first letter capitalized ("Red",...)
	LegacyTextContainer turnDurations; //turn durations for pregame (1 Minute ... Unlimited)

	//towns
	LegacyTextContainer tcommands, hcommands, fcommands; //texts for town screen, town hall screen and fort screen
	LegacyTextContainer tavernInfo;
	LegacyTextContainer tavernRumors;

	LegacyTextContainer qeModCommands;

	LegacyHelpContainer zelp;
	LegacyTextContainer lossCondtions;
	LegacyTextContainer victoryConditions;

	//objects
	LegacyTextContainer advobtxt;
	LegacyTextContainer restypes; //names of resources
	LegacyTextContainer randsign;
	LegacyTextContainer seerEmpty;
	LegacyTextContainer seerNames;
	LegacyTextContainer tentColors;

	//sec skills
	LegacyTextContainer levels;
	//commanders
	LegacyTextContainer znpc00; //more or less useful content of that file

	std::vector<std::string> findStringsWithPrefix(const std::string & prefix);

	int32_t pluralText(int32_t textIndex, int32_t count) const;

	size_t getCampaignLength(size_t campaignID) const;

	CGeneralTextHandler();
	CGeneralTextHandler(const CGeneralTextHandler&) = delete;
	CGeneralTextHandler operator=(const CGeneralTextHandler&) = delete;

	/// Attempts to detect encoding & language of H3 files
	static void detectInstallParameters();

	/// Returns name of language preferred by user
	static std::string getPreferredLanguage();

	/// Returns name of language of Heroes III text files
	static std::string getInstalledLanguage();

	/// Returns name of encoding of Heroes III text files
	static std::string getInstalledEncoding();
};

VCMI_LIB_NAMESPACE_END
