/*
 * Languages.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

namespace Languages
{

enum class ELanguages
{
	CZECH,
	CHINESE,
	ENGLISH,
	FINNISH,
	FRENCH,
	GERMAN,
	ITALIAN,
	KOREAN,
	POLISH,
	PORTUGUESE,
	RUSSIAN,
	SPANISH,
	SWEDISH,
	TURKISH,
	UKRAINIAN,

	// Pseudo-languages, that have no translations but can define H3 encoding to use
	OTHER_CP1250,
	OTHER_CP1251,
	OTHER_CP1252,

	COUNT
};

struct Options
{
	/// string identifier (ascii, lower-case), e.g. "english"
	std::string identifier;

	/// human-readable name of language in English
	std::string nameEnglish;

	/// human-readable name of language in its own language
	std::string nameNative;

	/// encoding that is used by H3 for this language
	std::string encoding;

	/// primary IETF language tag
	std::string tagIETF;

	/// VCMI supports translations into this language
	bool hasTranslation = false;
};

inline const auto & getLanguageList()
{
	static const std::array<Options, 18> languages
	{ {
		{ "czech",      "Czech",      "Čeština",    "CP1250", "cs", true },
		{ "chinese",    "Chinese",    "简体中文",       "GBK",    "zh", true }, // Note: actually Simplified Chinese
		{ "english",    "English",    "English",    "CP1252", "en", true },
		{ "finnish",    "Finnish",    "Suomi",      "CP1252", "fi", true },
		{ "french",     "French",     "Français",   "CP1252", "fr", true },
		{ "german",     "German",     "Deutsch",    "CP1252", "de", true },
		{ "italian",    "Italian",    "Italiano",   "CP1250", "it", true },
		{ "korean",     "Korean",     "한국어",        "CP949",  "ko", true },
		{ "polish",     "Polish",     "Polski",     "CP1250", "pl", true },
		{ "portuguese", "Portuguese", "Português",  "CP1252", "pt", true }, // Note: actually Brazilian Portuguese
		{ "russian",    "Russian",    "Русский",    "CP1251", "ru", true },
		{ "spanish",    "Spanish",    "Español",    "CP1252", "es", true },
		{ "swedish",    "Swedish",    "Svenska",    "CP1252", "sv", true },
		{ "turkish",    "Turkish",    "Türkçe",     "CP1254", "tr", true },
		{ "ukrainian",  "Ukrainian",  "Українська", "CP1251", "uk", true },

		{ "other_cp1250", "Other (East European)",   "", "CP1251", "", false },
		{ "other_cp1251", "Other (Cyrillic Script)", "", "CP1250", "", false },
		{ "other_cp1252", "Other (West European)",   "", "CP1252", "", false }
	} };
	static_assert(languages.size() == static_cast<size_t>(ELanguages::COUNT), "Languages array is missing a value!");

	return languages;
}

inline const Options & getLanguageOptions(ELanguages language)
{
	assert(language < ELanguages::COUNT);
	return getLanguageList()[static_cast<size_t>(language)];
}

inline const Options & getLanguageOptions(const std::string & language)
{
	for(const auto & entry : getLanguageList())
		if(entry.identifier == language)
			return entry;

	static const Options emptyValue;
	assert(0);
	return emptyValue;
}

}
