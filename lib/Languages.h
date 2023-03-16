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
	CHINESE,
	ENGLISH,
	FRENCH,
	GERMAN,
	KOREAN, // currently has no translations or detection
	POLISH,
	RUSSIAN,
	SPANISH,
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

	/// VCMI is capable of detecting H3 install in this language
	bool hasDetection = false;

	/// VCMI supports translations into this language
	bool hasTranslation = false;
};

inline const auto & getLanguageList()
{
	static const std::array<Options, 12> languages
	{ {
		{ "chinese",   "Chinese",   "简体中文",       "GBK",    true,  true }, // Note: actually Simplified Chinese
		{ "english",   "English",   "English",    "CP1252", true,  true },
		{ "french",    "French",    "Français",   "CP1252", true,  true },
		{ "german",    "German",    "Deutsch",    "CP1252", true,  true },
		{ "korean",    "Korean",    "한국어",        "CP949",  false, false },
		{ "polish",    "Polish",    "Polski",     "CP1250", true,  true },
		{ "russian",   "Russian",   "Русский",    "CP1251", true,  true },
		{ "spanish",   "Spanish",   "Español",    "CP1252", false, true },
		{ "ukrainian", "Ukrainian", "Українська", "CP1251", true,  true },

		{ "other_cp1250", "Other (East European)",   "", "CP1251", false, false },
		{ "other_cp1251", "Other (Cyrillic Script)", "", "CP1250", false, false },
		{ "other_cp1252", "Other (West European)",   "", "CP1252", false, false }
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
