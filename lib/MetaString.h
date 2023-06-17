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

class DLL_LINKAGE MetaString
{
private:
	enum EMessage {TEXACT_STRING, TLOCAL_STRING, TNUMBER, TREPLACE_ESTRING, TREPLACE_LSTRING, TREPLACE_NUMBER, TREPLACE_PLUSNUMBER};
public:
	enum {GENERAL_TXT=1, OBJ_NAMES, RES_NAMES, ART_NAMES, ARRAY_TXT, CRE_PL_NAMES, CREGENS, MINE_NAMES,
		MINE_EVNTS, ADVOB_TXT, ART_EVNTS, SPELL_NAME, SEC_SKILL_NAME, CRE_SING_NAMES, CREGENS4, COLOR, ART_DESCR, JK_TXT};

	std::vector<ui8> message; //vector of EMessage

	std::vector<std::pair<ui8,ui32> > localStrings;
	std::vector<std::string> exactStrings;
	std::vector<int64_t> numbers;

	template <typename Handler> void serialize(Handler & h, const int version)
	{
		h & exactStrings;
		h & localStrings;
		h & message;
		h & numbers;
	}
	void addTxt(ui8 type, ui32 serial)
	{
		message.push_back(TLOCAL_STRING);
		localStrings.emplace_back(type, serial);
	}
	void addRawString(std::string value)
	{
		message.push_back(TEXACT_STRING);
		exactStrings.push_back(value);
	}
	void appendNumber(int64_t value)
	{
		message.push_back(TNUMBER);
		numbers.push_back(value);
	}
	void addReplacement(ui8 type, ui32 serial)
	{
		message.push_back(TREPLACE_LSTRING);
		localStrings.emplace_back(type, serial);
	}
	void addReplacement(const std::string &txt)
	{
		message.push_back(TREPLACE_ESTRING);
		exactStrings.push_back(txt);
	}
	void addReplacement(int64_t txt)
	{
		message.push_back(TREPLACE_NUMBER);
		numbers.push_back(txt);
	}
	void addReplacement2(int64_t txt)
	{
		message.push_back(TREPLACE_PLUSNUMBER);
		numbers.push_back(txt);
	}
	void addCreReplacement(const CreatureID & id, TQuantity count); //adds sing or plural name;
	void addReplacement(const CStackBasicDescriptor &stack); //adds sing or plural name;
	std::string buildList () const;
	void clear()
	{
		exactStrings.clear();
		localStrings.clear();
		message.clear();
		numbers.clear();
	}
	void toString(std::string &dst) const;
	std::string toString() const;
	void getLocalString(const std::pair<ui8, ui32> & txt, std::string & dst) const;

};

VCMI_LIB_NAMESPACE_END
