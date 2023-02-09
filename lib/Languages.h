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
	ENGLISH,
	FRENCH,
	GERMAN,
	POLISH,
	RUSSIAN,
	UKRAINIAN,

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

inline auto const & getLanguageList( )
{
	static const std::array<Options, 6> languages
	{ {
		{ "english",   "English",   "English",    "CP1252", true,  true },
		{ "french",    "French",    "Français",   "CP1252", true, false },
		{ "german",    "German",    "Deutsch",    "CP1252", true,  true },
		{ "polish",    "Polish",    "Polski",     "CP1250", true,  true },
		{ "russian",   "Russian",   "Русский",    "CP1251", true,  true },
		{ "ukrainian", "Ukrainian", "Українська", "CP1251", true,  true }
	} };
	static_assert (languages.size() == static_cast<size_t>(ELanguages::COUNT), "Languages array is missing a value!" );

	return languages;
}

inline const Options & getLanguageOptions( ELanguages language )
{
	assert(language < ELanguages::COUNT);
	return getLanguageList()[static_cast<size_t>(language)];
}

inline const Options & getLanguageOptions( std::string language )
{
	for (auto const & entry : getLanguageList())
		if (entry.identifier == language)
			return entry;

	static const Options emptyValue;
	assert(0);
	return emptyValue;
}

}
