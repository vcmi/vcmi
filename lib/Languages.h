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

enum class EPluralForms
{
	NONE,
	VI_1, // Single plural form, (Vietnamese)
	EN_2, // Two forms, singular used for one only (English)
	FR_2, // Two forms, singular used for zero and one (French)
	UK_3, // Three forms, special cases for numbers ending in 1 and 2, 3, 4, except those ending in 1[1-4] (Ukrainian)
	CZ_3, // Three forms, special cases for 1 and 2, 3, 4 (Czech)
	PL_3, // Three forms, special case for one and some numbers ending in 2, 3, or 4 (Polish)
};

enum class ELanguages
{
	CZECH,
	CHINESE,
	ENGLISH,
	FINNISH,
	FRENCH,
	GERMAN,
	HUNGARIAN,
	ITALIAN,
	KOREAN,
	POLISH,
	PORTUGUESE,
	RUSSIAN,
	SPANISH,
	SWEDISH,
	TURKISH,
	UKRAINIAN,
	VIETNAMESE,

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

	/// DateTime format
	std::string dateTimeFormat;

	/// Ruleset for plural forms in this language
	EPluralForms pluralForms = EPluralForms::NONE;

	/// VCMI supports translations into this language
	bool hasTranslation = false;
};

inline const auto & getLanguageList()
{
	static const std::array<Options, 20> languages
	{ {
		{ "czech",       "Czech",       "Čeština",    "CP1250", "cs", "%d.%m.%Y %T", EPluralForms::CZ_3, true },
		{ "chinese",     "Chinese",     "简体中文",       "GBK",    "zh", "%F %T", EPluralForms::VI_1, true }, // Note: actually Simplified Chinese
		{ "english",     "English",     "English",    "CP1252", "en", "%F %T", EPluralForms::EN_2, true }, // English uses international date/time format here
		{ "finnish",     "Finnish",     "Suomi",      "CP1252", "fi", "%d.%m.%Y %T", EPluralForms::EN_2, true },
		{ "french",      "French",      "Français",   "CP1252", "fr", "%d/%m/%Y %T", EPluralForms::FR_2, true },
		{ "german",      "German",      "Deutsch",    "CP1252", "de", "%d.%m.%Y %T", EPluralForms::EN_2, true },
		{ "hungarian",   "Hungarian",   "Magyar",     "CP1250", "hu", "%Y. %m. %d. %T", EPluralForms::EN_2, true },
		{ "italian",     "Italian",     "Italiano",   "CP1250", "it", "%d/%m/%Y %T", EPluralForms::EN_2, true },
		{ "korean",      "Korean",      "한국어",        "CP949",  "ko", "%F %T", EPluralForms::VI_1, true },
		{ "polish",      "Polish",      "Polski",     "CP1250", "pl", "%d.%m.%Y %T", EPluralForms::PL_3, true },
		{ "portuguese",  "Portuguese",  "Português",  "CP1252", "pt", "%d/%m/%Y %T", EPluralForms::EN_2, true }, // Note: actually Brazilian Portuguese
		{ "russian",     "Russian",     "Русский",    "CP1251", "ru", "%d.%m.%Y %T", EPluralForms::UK_3, true },
		{ "spanish",     "Spanish",     "Español",    "CP1252", "es", "%d/%m/%Y %T", EPluralForms::EN_2, true },
		{ "swedish",     "Swedish",     "Svenska",    "CP1252", "sv", "%F %T", EPluralForms::EN_2, true },
		{ "turkish",     "Turkish",     "Türkçe",     "CP1254", "tr", "%d.%m.%Y %T", EPluralForms::EN_2, true },
		{ "ukrainian",   "Ukrainian",   "Українська", "CP1251", "uk", "%d.%m.%Y %T", EPluralForms::UK_3, true },
		{ "vietnamese",  "Vietnamese",  "Tiếng Việt", "UTF-8",  "vi", "%d/%m/%Y %T", EPluralForms::VI_1, true }, // Fan translation uses special encoding

		{ "other_cp1250", "Other (East European)",   "", "CP1250", "", "", EPluralForms::NONE, false },
		{ "other_cp1251", "Other (Cyrillic Script)", "", "CP1251", "", "", EPluralForms::NONE, false },
		{ "other_cp1252", "Other (West European)",   "", "CP1252", "", "", EPluralForms::NONE, false }
	} };
	static_assert(languages.size() == static_cast<size_t>(ELanguages::COUNT), "Languages array is missing a value!");

	return languages;
}

inline const Options & getLanguageOptions(ELanguages language)
{
	return getLanguageList().at(static_cast<size_t>(language));
}

inline const Options & getLanguageOptions(const std::string & language)
{
	for(const auto & entry : getLanguageList())
		if(entry.identifier == language)
			return entry;

	throw std::out_of_range("Language " + language + " does not exists!");
}

template<typename Numeric>
inline constexpr int getPluralFormIndex(EPluralForms form, Numeric value)
{
	// Based on https://www.gnu.org/software/gettext/manual/html_node/Plural-forms.html
	switch(form)
	{
		case EPluralForms::NONE:
		case EPluralForms::VI_1:
			return 0;
		case EPluralForms::EN_2:
			if (value == 1)
				return 1;
			return 2;
		case EPluralForms::FR_2:
			if (value == 1 || value == 0)
				return 1;
			return 2;
		case EPluralForms::UK_3:
			if (value % 10 == 1 && value % 100 != 11)
				return 1;
			if (value%10>=2 && value%10<=4 && (value%100<10 || value%100>=20))
				return 2;
			return 0;
		case EPluralForms::CZ_3:
			if (value == 1)
				return 1;
			if (value>=2 && value<=4)
				return 2;
			return 0;
		case EPluralForms::PL_3:
			if (value == 1)
				return 1;
			if (value%10>=2 && value%10<=4 && (value%100<10 || value%100>=20))
				return 2;
			return 0;
	}
	throw std::runtime_error("Invalid plural form enumeration received!");
}

template<typename Numeric>
inline std::string getPluralFormTextID(std::string languageName, Numeric value, std::string textID)
{
	int formIndex = getPluralFormIndex(getLanguageOptions(languageName).pluralForms, value);
	return textID + '.' + std::to_string(formIndex);
}

}
