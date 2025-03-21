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
	RO_3, // Three forms, special case for numbers ending in 00 or [2-9][0-9] (Romanian)
};

enum class ELanguages
{
	BELARUSIAN,
	BULGARIAN,
	CZECH,
	CHINESE,
	ENGLISH,
	FINNISH,
	FRENCH,
	GERMAN,
	GREEK,
	HUNGARIAN,
	ITALIAN,
	JAPANESE,
	KOREAN,
	NORWEGIAN,
	POLISH,
	PORTUGUESE,
	ROMANIAN,
	RUSSIAN,
	SPANISH,
	SWEDISH,
	TURKISH,
	UKRAINIAN,
	VIETNAMESE,

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

	/// proper locale name, e.g. "en_US.UTF-8"
	std::string localeName;

	/// primary IETF language tag
	std::string tagIETF;

	/// ISO 639-2 (B) language code
	std::string tagISO2;

	/// DateTime format
	std::string dateTimeFormat;

	/// Ruleset for plural forms in this language
	EPluralForms pluralForms = EPluralForms::NONE;

	/// Selectable in launcher
	bool selectable;
};

inline const auto & getLanguageList()
{
	static const std::array<Options, 23> languages
	{ {
		{ "belarusian",  "Belarusian",  "беларускі",		"CP1251", "be_BY.UTF-8", "be", "bel", "%d.%m.%Y %H:%M",		EPluralForms::UK_3, true },
		{ "bulgarian",   "Bulgarian",   "Български",  "CP1251", "bg_BG.UTF-8", "bg", "bul", "%d.%m.%Y %H:%M",    EPluralForms::EN_2, true  },
		{ "czech",       "Czech",       "Čeština",		"CP1250", "cs_CZ.UTF-8", "cs", "cze", "%d.%m.%Y %H:%M",		EPluralForms::CZ_3, true },
		{ "chinese",     "Chinese",     "简体中文",		"GBK",	  "zh_CN.UTF-8", "zh", "chi", "%Y-%m-%d %H:%M",		EPluralForms::VI_1, true }, // Note: actually Simplified Chinese
		{ "english",     "English",     "English",		"CP1252", "en_US.UTF-8", "en", "eng", "%Y-%m-%d %H:%M",		EPluralForms::EN_2, true }, // English uses international date/time format here
		{ "finnish",     "Finnish",     "Suomi",		"CP1252", "fi_FI.UTF-8", "fi", "fin", "%d.%m.%Y %H:%M",		EPluralForms::EN_2, true },
		{ "french",      "French",      "Français",		"CP1252", "fr_FR.UTF-8", "fr", "fre", "%d/%m/%Y %H:%M",		EPluralForms::FR_2, true },
		{ "german",      "German",      "Deutsch",		"CP1252", "de_DE.UTF-8", "de", "ger", "%d.%m.%Y %H:%M",		EPluralForms::EN_2, true },
		{ "greek",       "Greek",       "ελληνικά",		"CP1253", "el_GR.UTF-8", "el", "ell", "%d/%m/%Y %H:%M",		EPluralForms::EN_2, false },
		{ "hungarian",   "Hungarian",   "Magyar",		"CP1250", "hu_HU.UTF-8", "hu", "hun", "%Y. %m. %d. %H:%M",  EPluralForms::EN_2, true },
		{ "italian",     "Italian",     "Italiano",		"CP1250", "it_IT.UTF-8", "it", "ita", "%d/%m/%Y %H:%M",		EPluralForms::EN_2, true },
		{ "japanese",    "Japanese",    "日本語",		"JIS",    "ja_JP.UTF-8", "ja", "jpn", "%Y年%m月%d日 %H:%M",	EPluralForms::NONE, false },
		{ "korean",      "Korean",      "한국어",		"CP949",  "ko_KR.UTF-8", "ko", "kor", "%Y-%m-%d %H:%M",		EPluralForms::VI_1, true },
		{ "polish",      "Polish",      "Polski",		"CP1250", "pl_PL.UTF-8", "pl", "pol", "%d.%m.%Y %H:%M",		EPluralForms::PL_3, true },
		{ "portuguese",  "Portuguese",  "Português",	"CP1252", "pt_BR.UTF-8", "pt", "por", "%d/%m/%Y %H:%M",		EPluralForms::EN_2, true }, // Note: actually Brazilian Portuguese
		{ "romanian",    "Romanian",    "Română",     "CP28606", "ro_RO.UTF-8", "ro", "rum", "%Y-%m-%d %H:%M",    EPluralForms::RO_3, false },
		{ "russian",     "Russian",     "Русский",		"CP1251", "ru_RU.UTF-8", "ru", "rus", "%d.%m.%Y %H:%M",		EPluralForms::UK_3, true },
		{ "spanish",     "Spanish",     "Español",		"CP1252", "es_ES.UTF-8", "es", "spa", "%d/%m/%Y %H:%M",		EPluralForms::EN_2, true },
		{ "swedish",     "Swedish",     "Svenska",		"CP1252", "sv_SE.UTF-8", "sv", "swe", "%Y-%m-%d %H:%M",		EPluralForms::EN_2, true },
		{ "norwegian",   "Norwegian",   "Norsk Bokmål", "UTF-8",  "nb_NO.UTF-8", "nb", "nor", "%d/%m/%Y %H:%M",		EPluralForms::EN_2, false },
		{ "turkish",     "Turkish",     "Türkçe",		"CP1254", "tr_TR.UTF-8", "tr", "tur", "%d.%m.%Y %H:%M",		EPluralForms::EN_2, true },
		{ "ukrainian",   "Ukrainian",   "Українська",	"CP1251", "uk_UA.UTF-8", "uk", "ukr", "%d.%m.%Y %H:%M",		EPluralForms::UK_3, true },
		{ "vietnamese",  "Vietnamese",  "Tiếng Việt",	"UTF-8",  "vi_VN.UTF-8", "vi", "vie", "%d/%m/%Y %H:%M",		EPluralForms::VI_1, true }, // Fan translation uses special encoding
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
		case EPluralForms::RO_3:
			if (value == 1)
			    return 1;
			if (value==0 || (value%100 > 0 && value%100 < 20))
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
