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

#include "CCreatureHandler.h"
#include "CCreatureSet.h"
#include "entities/artifact/CArtifact.h"
#include "entities/faction/CFaction.h"
#include "entities/hero/CHero.h"
#include "texts/CGeneralTextHandler.h"
#include "CSkillHandler.h"
#include "GameConstants.h"
#include "GameLibrary.h"
#include "mapObjectConstructors/CObjectClassesHandler.h"
#include "spells/CSpellHandler.h"
#include "serializer/JsonSerializeFormat.h"

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

MetaString MetaString::createFromName(const GameResID& id)
{
	MetaString result;
	result.appendName(id);
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
	if (!value.empty())
	{
		message.push_back(EMessage::APPEND_TEXTID_STRING);
		stringsTextID.push_back(value);
	}
}

void MetaString::appendNumber(int64_t value)
{
	message.push_back(EMessage::APPEND_NUMBER);
	numbers.push_back(value);
}

void MetaString::appendEOL()
{
	message.push_back(EMessage::APPEND_EOL);
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
	return message.empty() || toString().empty();
}

std::string MetaString::getLocalString(const std::pair<EMetaText, ui32> & txt) const
{
	EMetaText type = txt.first;
	int ser = txt.second;

	switch(type)
	{
		case EMetaText::GENERAL_TXT:
			return LIBRARY->generaltexth->translate("core.genrltxt", ser);
		case EMetaText::ARRAY_TXT:
			return LIBRARY->generaltexth->translate("core.arraytxt", ser);
		case EMetaText::ADVOB_TXT:
			return LIBRARY->generaltexth->translate("core.advevent", ser);
		case EMetaText::JK_TXT:
			return LIBRARY->generaltexth->translate("core.jktext", ser);
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
				dst += exactStrings.at(exSt++);
				break;
			case EMessage::APPEND_LOCAL_STRING:
				dst += getLocalString(localStrings.at(loSt++));
				break;
			case EMessage::APPEND_TEXTID_STRING:
				dst += LIBRARY->generaltexth->translate(stringsTextID.at(textID++));
				break;
			case EMessage::APPEND_NUMBER:
				dst += std::to_string(numbers.at(nums++));
				break;
			case EMessage::APPEND_EOL:
				dst += '\n';
				break;
			case EMessage::REPLACE_RAW_STRING:
				boost::replace_first(dst, "%s", exactStrings.at(exSt++));
				break;
			case EMessage::REPLACE_LOCAL_STRING:
				boost::replace_first(dst, "%s", getLocalString(localStrings.at(loSt++)));
				break;
			case EMessage::REPLACE_TEXTID_STRING:
				boost::replace_first(dst, "%s", LIBRARY->generaltexth->translate(stringsTextID.at(textID++)));
				break;
			case EMessage::REPLACE_NUMBER:
				boost::replace_first(dst, "%d", std::to_string(numbers.at(nums++)));
				break;
			case EMessage::REPLACE_POSITIVE_NUMBER:
				if (dst.find("%+d") != std::string::npos)
				{
					int64_t value = numbers.at(nums);
					if (value > 0)
						boost::replace_first(dst, "%+d", '+' + std::to_string(value));
					else
						boost::replace_first(dst, "%+d", std::to_string(value));

					nums++;
				}
				else
					boost::replace_first(dst, "%d", std::to_string(numbers.at(nums++)));
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
		if(i > 0 && (message.at(i) == EMessage::APPEND_RAW_STRING || message.at(i) == EMessage::APPEND_LOCAL_STRING))
		{
			if(exSt == exactStrings.size() - 1)
				lista += LIBRARY->generaltexth->allTexts[141]; //" and "
			else
				lista += ", ";
		}
		switch(message.at(i))
		{
			case EMessage::APPEND_RAW_STRING:
				lista += exactStrings.at(exSt++);
				break;
			case EMessage::APPEND_LOCAL_STRING:
				lista += getLocalString(localStrings.at(loSt++));
				break;
			case EMessage::APPEND_TEXTID_STRING:
				lista += LIBRARY->generaltexth->translate(stringsTextID.at(textID++));
				break;
			case EMessage::APPEND_NUMBER:
				lista += std::to_string(numbers.at(nums++));
				break;
			case EMessage::APPEND_EOL:
				lista += '\n';
				break;
			case EMessage::REPLACE_RAW_STRING:
				lista.replace(lista.find("%s"), 2, exactStrings.at(exSt++));
				break;
			case EMessage::REPLACE_LOCAL_STRING:
				lista.replace(lista.find("%s"), 2, getLocalString(localStrings.at(loSt++)));
				break;
			case EMessage::REPLACE_TEXTID_STRING:
				lista.replace(lista.find("%s"), 2, LIBRARY->generaltexth->translate(stringsTextID.at(textID++)));
				break;
			case EMessage::REPLACE_NUMBER:
				lista.replace(lista.find("%d"), 2, std::to_string(numbers.at(nums++)));
				break;
			default:
				logGlobal->error("MetaString processing error! Received message of type %d", int(message.at(i)));
		}
	}
	return lista;
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

void MetaString::serializeJson(JsonSerializeFormat & handler)
{
	if(handler.saving)
		jsonSerialize(const_cast<JsonNode&>(handler.getCurrent()));

	if(!handler.saving)
		jsonDeserialize(handler.getCurrent());
}

void MetaString::appendName(const ArtifactID & id)
{
	appendTextID(id.toEntity(LIBRARY)->getNameTextID());
}

void MetaString::appendName(const SpellID & id)
{
	appendTextID(id.toEntity(LIBRARY)->getNameTextID());
}

void MetaString::appendName(const PlayerColor & id)
{
	appendTextID(TextIdentifier("vcmi.capitalColors", id.getNum()).get());
}

void MetaString::appendName(const CreatureID & id, TQuantity count)
{
	if(count == 1)
		appendNameSingular(id);
	else
		appendNamePlural(id);
}

void MetaString::appendName(const GameResID& id)
{
	appendTextID(TextIdentifier("core.restypes", id.getNum()).get());
}

void MetaString::appendNameSingular(const CreatureID & id)
{
	appendTextID(id.toEntity(LIBRARY)->getNameSingularTextID());
}

void MetaString::appendNamePlural(const CreatureID & id)
{
	appendTextID(id.toEntity(LIBRARY)->getNamePluralTextID());
}

void MetaString::replaceName(const ArtifactID & id)
{
	replaceTextID(id.toEntity(LIBRARY)->getNameTextID());
}

void MetaString::replaceName(const FactionID & id)
{
	replaceTextID(id.toEntity(LIBRARY)->getNameTextID());
}

void MetaString::replaceName(const MapObjectID & id, const MapObjectSubID & subId)
{
	replaceTextID(LIBRARY->objtypeh->getObjectName(id, subId));
}

void MetaString::replaceName(const PlayerColor & id)
{
	replaceTextID(TextIdentifier("vcmi.capitalColors", id.getNum()).get());
}

void MetaString::replaceName(const SecondarySkill & id)
{
	replaceTextID(LIBRARY->skillh->getById(id)->getNameTextID());
}

void MetaString::replaceName(const SpellID & id)
{
	replaceTextID(id.toEntity(LIBRARY)->getNameTextID());
}

void MetaString::replaceName(const GameResID& id)
{
	replaceTextID(TextIdentifier("core.restypes", id.getNum()).get());
}

void MetaString::replaceNameSingular(const CreatureID & id)
{
	replaceTextID(id.toEntity(LIBRARY)->getNameSingularTextID());
}

void MetaString::replaceNamePlural(const CreatureID & id)
{
	replaceTextID(id.toEntity(LIBRARY)->getNamePluralTextID());
}

void MetaString::replaceName(const CreatureID & id, TQuantity count) //adds sing or plural name;
{
	if(count == 1)
		replaceNameSingular(id);
	else
		replaceNamePlural(id);
}

void MetaString::replaceName(const CStackBasicDescriptor & stack)
{
	replaceName(stack.getId(), stack.getCount());
}

VCMI_LIB_NAMESPACE_END
