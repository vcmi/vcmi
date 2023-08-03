/*
 * MetaString.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "MetaString.h"

#include "CArtHandler.h"
#include "CCreatureHandler.h"
#include "CCreatureSet.h"
#include "CGeneralTextHandler.h"
#include "CSkillHandler.h"
#include "GameConstants.h"
#include "VCMI_Lib.h"
#include "mapObjectConstructors/CObjectClassesHandler.h"
#include "spells/CSpellHandler.h"

VCMI_LIB_NAMESPACE_BEGIN

MetaString MetaString::createFromRawString(const std::string & value)
{
	MetaString result;
	result.appendRawString(value);
	return result;
}

MetaString MetaString::createFromTextID(const std::string & value)
{
	MetaString result;
	result.appendTextID(value);
	return result;
}

void MetaString::appendLocalString(EMetaText type, ui32 serial)
{
	message.push_back(EMessage::APPEND_LOCAL_STRING);
	localStrings.emplace_back(type, serial);
}

void MetaString::appendRawString(const std::string & value)
{
	message.push_back(EMessage::APPEND_RAW_STRING);
	exactStrings.push_back(value);
}

void MetaString::appendTextID(const std::string & value)
{
	message.push_back(EMessage::APPEND_TEXTID_STRING);
	stringsTextID.push_back(value);
}

void MetaString::appendNumber(int64_t value)
{
	message.push_back(EMessage::APPEND_NUMBER);
	numbers.push_back(value);
}

void MetaString::replaceLocalString(EMetaText type, ui32 serial)
{
	message.push_back(EMessage::REPLACE_LOCAL_STRING);
	localStrings.emplace_back(type, serial);
}

void MetaString::replaceRawString(const std::string &txt)
{
	message.push_back(EMessage::REPLACE_RAW_STRING);
	exactStrings.push_back(txt);
}

void MetaString::replaceTextID(const std::string & value)
{
	message.push_back(EMessage::REPLACE_TEXTID_STRING);
	stringsTextID.push_back(value);
}

void MetaString::replaceNumber(int64_t txt)
{
	message.push_back(EMessage::REPLACE_NUMBER);
	numbers.push_back(txt);
}

void MetaString::replacePositiveNumber(int64_t txt)
{
	message.push_back(EMessage::REPLACE_POSITIVE_NUMBER);
	numbers.push_back(txt);
}

void MetaString::clear()
{
	exactStrings.clear();
	localStrings.clear();
	stringsTextID.clear();
	message.clear();
	numbers.clear();
}

bool MetaString::empty() const
{
	return message.empty();
}

std::string MetaString::getLocalString(const std::pair<EMetaText, ui32> & txt) const
{
	EMetaText type = txt.first;
	int ser = txt.second;

	switch(type)
	{
		case EMetaText::ART_NAMES:
		{
			const auto * art = ArtifactID(ser).toArtifact(VLC->artifacts());
			if(art)
				return art->getNameTranslated();
			return "#!#";
		}
		case EMetaText::ART_DESCR:
		{
			const auto * art = ArtifactID(ser).toArtifact(VLC->artifacts());
			if(art)
				return art->getDescriptionTranslated();
			return "#!#";
		}
		case EMetaText::ART_EVNTS:
		{
			const auto * art = ArtifactID(ser).toArtifact(VLC->artifacts());
			if(art)
				return art->getEventTranslated();
			return "#!#";
		}
		case EMetaText::CRE_PL_NAMES:
		{
			const auto * cre = CreatureID(ser).toCreature(VLC->creatures());
			if(cre)
				return cre->getNamePluralTranslated();
			return "#!#";
		}
		case EMetaText::CRE_SING_NAMES:
		{
			const auto * cre = CreatureID(ser).toCreature(VLC->creatures());
			if(cre)
				return cre->getNameSingularTranslated();
			return "#!#";
		}
		case EMetaText::MINE_NAMES:
		{
			return VLC->generaltexth->translate("core.minename", ser);
		}
		case EMetaText::MINE_EVNTS:
		{
			return VLC->generaltexth->translate("core.mineevnt", ser);
		}
		case EMetaText::SPELL_NAME:
		{
			const auto * spell = SpellID(ser).toSpell(VLC->spells());
			if(spell)
				return spell->getNameTranslated();
			return "#!#";
		}
		case EMetaText::OBJ_NAMES:
			return VLC->objtypeh->getObjectName(ser, 0);
		case EMetaText::SEC_SKILL_NAME:
			return VLC->skillh->getByIndex(ser)->getNameTranslated();
		case EMetaText::GENERAL_TXT:
			return VLC->generaltexth->translate("core.genrltxt", ser);
		case EMetaText::RES_NAMES:
			return VLC->generaltexth->translate("core.restypes", ser);
		case EMetaText::ARRAY_TXT:
			return VLC->generaltexth->translate("core.arraytxt", ser);
		case EMetaText::CREGENS:
			return VLC->objtypeh->getObjectName(Obj::CREATURE_GENERATOR1, ser);
		case EMetaText::CREGENS4:
			return VLC->objtypeh->getObjectName(Obj::CREATURE_GENERATOR4, ser);
		case EMetaText::ADVOB_TXT:
			return VLC->generaltexth->translate("core.advevent", ser);
		case EMetaText::COLOR:
			return VLC->generaltexth->translate("vcmi.capitalColors", ser);
		case EMetaText::JK_TXT:
			return VLC->generaltexth->translate("core.jktext", ser);
		default:
			logGlobal->error("Failed string substitution because type is %d", static_cast<int>(type));
			return "#@#";
	}
}

DLL_LINKAGE std::string MetaString::toString() const
{
	std::string dst;

	size_t exSt = 0;
	size_t loSt = 0;
	size_t nums = 0;
	size_t textID = 0;
	dst.clear();

	for(const auto & elem : message)
	{
		switch(elem)
		{
			case EMessage::APPEND_RAW_STRING:
				dst += exactStrings[exSt++];
				break;
			case EMessage::APPEND_LOCAL_STRING:
				dst += getLocalString(localStrings[loSt++]);
				break;
			case EMessage::APPEND_TEXTID_STRING:
				dst += VLC->generaltexth->translate(stringsTextID[textID++]);
				break;
			case EMessage::APPEND_NUMBER:
				dst += std::to_string(numbers[nums++]);
				break;
			case EMessage::REPLACE_RAW_STRING:
				boost::replace_first(dst, "%s", exactStrings[exSt++]);
				break;
			case EMessage::REPLACE_LOCAL_STRING:
				boost::replace_first(dst, "%s", getLocalString(localStrings[loSt++]));
				break;
			case EMessage::REPLACE_TEXTID_STRING:
				boost::replace_first(dst, "%s", VLC->generaltexth->translate(stringsTextID[textID++]));
				break;
			case EMessage::REPLACE_NUMBER:
				boost::replace_first(dst, "%d", std::to_string(numbers[nums++]));
				break;
			case EMessage::REPLACE_POSITIVE_NUMBER:
				boost::replace_first(dst, "%+d", '+' + std::to_string(numbers[nums++]));
				break;
			default:
				logGlobal->error("MetaString processing error! Received message of type %d", static_cast<int>(elem));
				assert(0);
				break;
		}
	}
	return dst;
}

DLL_LINKAGE std::string MetaString::buildList() const
{
	size_t exSt = 0;
	size_t loSt = 0;
	size_t nums = 0;
	size_t textID = 0;
	std::string lista;
	for(int i = 0; i < message.size(); ++i)
	{
		if(i > 0 && (message[i] == EMessage::APPEND_RAW_STRING || message[i] == EMessage::APPEND_LOCAL_STRING))
		{
			if(exSt == exactStrings.size() - 1)
				lista += VLC->generaltexth->allTexts[141]; //" and "
			else
				lista += ", ";
		}
		switch(message[i])
		{
			case EMessage::APPEND_RAW_STRING:
				lista += exactStrings[exSt++];
				break;
			case EMessage::APPEND_LOCAL_STRING:
				lista += getLocalString(localStrings[loSt++]);
				break;
			case EMessage::APPEND_TEXTID_STRING:
				lista += VLC->generaltexth->translate(stringsTextID[textID++]);
				break;
			case EMessage::APPEND_NUMBER:
				lista += std::to_string(numbers[nums++]);
				break;
			case EMessage::REPLACE_RAW_STRING:
				lista.replace(lista.find("%s"), 2, exactStrings[exSt++]);
				break;
			case EMessage::REPLACE_LOCAL_STRING:
				lista.replace(lista.find("%s"), 2, getLocalString(localStrings[loSt++]));
				break;
			case EMessage::REPLACE_TEXTID_STRING:
				lista.replace(lista.find("%s"), 2, VLC->generaltexth->translate(stringsTextID[textID++]));
				break;
			case EMessage::REPLACE_NUMBER:
				lista.replace(lista.find("%d"), 2, std::to_string(numbers[nums++]));
				break;
			default:
				logGlobal->error("MetaString processing error! Received message of type %d", int(message[i]));
		}
	}
	return lista;
}

void MetaString::replaceCreatureName(const CreatureID & id, TQuantity count) //adds sing or plural name;
{
	if (count == 1)
		replaceLocalString (EMetaText::CRE_SING_NAMES, id);
	else
		replaceLocalString (EMetaText::CRE_PL_NAMES, id);
}

void MetaString::replaceCreatureName(const CStackBasicDescriptor & stack)
{
	assert(stack.type); //valid type
	replaceCreatureName(stack.type->getId(), stack.count);
}

bool MetaString::operator == (const MetaString & other) const
{
	return message == other.message && localStrings == other.localStrings && exactStrings == other.exactStrings && stringsTextID == other.stringsTextID && numbers == other.numbers;
}

void MetaString::jsonSerialize(JsonNode & dest) const
{
	JsonNode jsonMessage;
	JsonNode jsonLocalStrings;
	JsonNode jsonExactStrings;
	JsonNode jsonStringsTextID;
	JsonNode jsonNumbers;

	for (const auto & entry : message )
	{
		JsonNode value;
		value.Float() = static_cast<int>(entry);
		jsonMessage.Vector().push_back(value);
	}

	for (const auto & entry : localStrings )
	{
		JsonNode value;
		value.Integer() = static_cast<int>(entry.first) * 10000 + entry.second;
		jsonLocalStrings.Vector().push_back(value);
	}

	for (const auto & entry : exactStrings )
	{
		JsonNode value;
		value.String() = entry;
		jsonExactStrings.Vector().push_back(value);
	}

	for (const auto & entry : stringsTextID )
	{
		JsonNode value;
		value.String() = entry;
		jsonStringsTextID.Vector().push_back(value);
	}

	for (const auto & entry : numbers )
	{
		JsonNode value;
		value.Integer() = entry;
		jsonNumbers.Vector().push_back(value);
	}

	dest["message"] = jsonMessage;
	dest["localStrings"] = jsonLocalStrings;
	dest["exactStrings"] = jsonExactStrings;
	dest["stringsTextID"] = jsonStringsTextID;
	dest["numbers"] = jsonNumbers;
}

void MetaString::jsonDeserialize(const JsonNode & source)
{
	clear();

	if (source.isString())
	{
		// compatibility with fields that were converted from string to MetaString
		if(boost::starts_with(source.String(), "core.") || boost::starts_with(source.String(), "vcmi."))
			appendTextID(source.String());
		else
			appendRawString(source.String());
		return;
	}

	for (const auto & entry : source["message"].Vector() )
		message.push_back(static_cast<EMessage>(entry.Integer()));

	for (const auto & entry : source["localStrings"].Vector() )
		localStrings.push_back({ static_cast<EMetaText>(entry.Integer() / 10000), entry.Integer() % 10000 });

	for (const auto & entry : source["exactStrings"].Vector() )
		exactStrings.push_back(entry.String());

	for (const auto & entry : source["stringsTextID"].Vector() )
		stringsTextID.push_back(entry.String());

	for (const auto & entry : source["numbers"].Vector() )
		numbers.push_back(entry.Integer());
}

VCMI_LIB_NAMESPACE_END
