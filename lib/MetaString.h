/*
 * MetaString.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

VCMI_LIB_NAMESPACE_BEGIN

class CreatureID;
class CStackBasicDescriptor;
using TQuantity = si32;

/// Strings classes that can be used as replacement in MetaString
enum class EMetaText : uint8_t
{
	GENERAL_TXT = 1,
	OBJ_NAMES,
	RES_NAMES,
	ART_NAMES,
	ARRAY_TXT,
	CRE_PL_NAMES,
	CREGENS,
	MINE_NAMES,
	MINE_EVNTS,
	ADVOB_TXT,
	ART_EVNTS,
	SPELL_NAME,
	SEC_SKILL_NAME,
	CRE_SING_NAMES,
	CREGENS4,
	COLOR,
	ART_DESCR,
	JK_TXT
};

/// Class for string formatting tools that also support transfer over network with localization using language of local install
/// Can be used to compose resulting text from multiple line segments and with placeholder replacement
class DLL_LINKAGE MetaString
{
private:
	enum class EMessage : uint8_t
	{
		APPEND_RAW_STRING,
		APPEND_LOCAL_STRING,
		APPEND_TEXTID_STRING,
		APPEND_NUMBER,
		REPLACE_RAW_STRING,
		REPLACE_LOCAL_STRING,
		REPLACE_TEXTID_STRING,
		REPLACE_NUMBER,
		REPLACE_POSITIVE_NUMBER
	};

	std::vector<EMessage> message;

	std::vector<std::pair<EMetaText,ui32> > localStrings;
	std::vector<std::string> exactStrings;
	std::vector<std::string> stringsTextID;
	std::vector<int64_t> numbers;

	std::string getLocalString(const std::pair<EMetaText, ui32> & txt) const;

public:
	/// Creates MetaString and appends provided raw string to it
	static MetaString createFromRawString(const std::string & value);
	/// Creates MetaString and appends provided text ID string to it
	static MetaString createFromTextID(const std::string & value);

	/// Appends local string to resulting string
	void appendLocalString(EMetaText type, ui32 serial);
	/// Appends raw string, without translation to resulting string
	void appendRawString(const std::string & value);
	/// Appends text ID that will be translated in output
	void appendTextID(const std::string & value);
	/// Appends specified number to resulting string
	void appendNumber(int64_t value);

	/// Replaces first '%s' placeholder in string with specified local string
	void replaceLocalString(EMetaText type, ui32 serial);
	/// Replaces first '%s' placeholder in string with specified fixed, untranslated string
	void replaceRawString(const std::string & txt);
	/// Repalces first '%s' placeholder with string ID that will be translated in output
	void replaceTextID(const std::string & value);
	/// Replaces first '%d' placeholder in string with specified number
	void replaceNumber(int64_t txt);
	/// Replaces first '%+d' placeholder in string with specified number using '+' sign as prefix
	void replacePositiveNumber(int64_t txt);

	/// Replaces first '%s' placeholder with singular or plural name depending on creatures count
	void replaceCreatureName(const CreatureID & id, TQuantity count);
	/// Replaces first '%s' placeholder with singular or plural name depending on creatures count
	void replaceCreatureName(const CStackBasicDescriptor & stack);

	/// erases any existing content in the string
	void clear();

	///used to handle loot from creature bank
	std::string buildList() const;

	/// Convert all stored values into a single, user-readable string
	std::string toString() const;

	/// Returns true if current string is empty
	bool empty() const;

	template <typename Handler> void serialize(Handler & h, const int version)
	{
		h & exactStrings;
		h & localStrings;
		h & stringsTextID;
		h & message;
		h & numbers;
	}
};

VCMI_LIB_NAMESPACE_END
