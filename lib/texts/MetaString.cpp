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
#include "entities/artifact/CArtifact.h"
#include "entities/faction/CFaction.h"
#include "entities/hero/CHero.h"
#include "entities/ResourceTypeHandler.h"
#include "texts/CGeneralTextHandler.h"
#include "texts/ITranslator.h"
#include "CSkillHandler.h"
#include "GameConstants.h"
#include "GameLibrary.h"
#include "mapObjects/army/CStackBasicDescriptor.h"
#include "mapObjectConstructors/CObjectClassesHandler.h"
#include "serializer/JsonSerializeFormat.h"

#include <vcmi/spells/Spell.h>

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

MetaString MetaString::createFromTextID(const std::string & prefix, int index)
{
	return createFromTextID(prefix + '.' + std::to_string(index));
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
	// appending nothing has to leave no trace, so that emptiness can be told from the ops alone
	if (!value.empty())
	{
		message.push_back(EMessage::APPEND_RAW_STRING);
		exactStrings.push_back(value);
	}
}

void MetaString::appendTextID(const std::string & value)
{
	if (!value.empty())
	{
		message.push_back(EMessage::APPEND_TEXTID_STRING);
		stringsTextID.push_back(value);
	}
}

void MetaString::appendTextID(const std::string & prefix, int index)
{
	appendTextID(prefix + '.' + std::to_string(index));
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

void MetaString::append(const MetaString & other)
{
	vstd::concatenate(message, other.message);
	vstd::concatenate(localStrings, other.localStrings);
	vstd::concatenate(exactStrings, other.exactStrings);
	vstd::concatenate(stringsTextID, other.stringsTextID);
	vstd::concatenate(numbers, other.numbers);
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

void MetaString::replaceTextID(const std::string & prefix, int index)
{
	replaceTextID(prefix + '.' + std::to_string(index));
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

void MetaString::replaceTokenTextID(const std::string & token, const std::string & value)
{
	message.push_back(EMessage::REPLACE_TOKEN_TEXTID);
	exactStrings.push_back(token);
	stringsTextID.push_back(value);
}

void MetaString::replaceTokenNumber(const std::string & token, int64_t value)
{
	message.push_back(EMessage::REPLACE_TOKEN_NUMBER);
	exactStrings.push_back(token);
	numbers.push_back(value);
}

void MetaString::replaceTokenRawString(const std::string & token, const std::string & value)
{
	message.push_back(EMessage::REPLACE_TOKEN_RAW_STRING);
	exactStrings.push_back(token);
	exactStrings.push_back(value);
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
	// resolving here would need a translator the callers do not have, and for map text the static
	// store alone cannot answer - it holds no map strings, so every check would report a failed lookup
	return message.empty();
}

std::string MetaString::getLocalString(const ITranslator * translator, const std::pair<EMetaText, ui32> & txt) const
{
	EMetaText type = txt.first;
	int ser = txt.second;

	switch(type)
	{
		case EMetaText::GENERAL_TXT:
			return translator->translate("core.genrltxt", ser);
		case EMetaText::ARRAY_TXT:
			return translator->translate("core.arraytxt", ser);
		case EMetaText::ADVOB_TXT:
			return translator->translate("core.advevent", ser);
		case EMetaText::JK_TXT:
			return translator->translate("core.jktext", ser);
		default:
			logGlobal->error("Failed string substitution because type is %d", static_cast<int>(type));
			return "#@#";
	}
}

DLL_LINKAGE std::string MetaString::toString(const ITranslator * translator) const
{
	assert(translator != nullptr);

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
				dst += getLocalString(translator, localStrings.at(loSt++));
				break;
			case EMessage::APPEND_TEXTID_STRING:
				dst += translator->translate(stringsTextID.at(textID++));
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
				boost::replace_first(dst, "%s", getLocalString(translator, localStrings.at(loSt++)));
				break;
			case EMessage::REPLACE_TEXTID_STRING:
				boost::replace_first(dst, "%s", translator->translate(stringsTextID.at(textID++)));
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
			case EMessage::REPLACE_TOKEN_TEXTID:
				boost::replace_first(dst, exactStrings.at(exSt++), translator->translate(stringsTextID.at(textID++)));
				break;
			case EMessage::REPLACE_TOKEN_NUMBER:
				boost::replace_first(dst, exactStrings.at(exSt++), std::to_string(numbers.at(nums++)));
				break;
			case EMessage::REPLACE_TOKEN_RAW_STRING:
			{
				// token and value share one vector, so the reads must be sequenced
				const std::string & token = exactStrings.at(exSt++);
				boost::replace_first(dst, token, exactStrings.at(exSt++));
				break;
			}
			default:
				logGlobal->error("MetaString processing error! Received message of type %d", static_cast<int>(elem));
				assert(0);
				break;
		}
	}
	return dst;
}

DLL_LINKAGE std::string MetaString::buildList(const ITranslator * translator) const
{
	assert(translator != nullptr);

	// only appends of a whole string form list entries - replacements act on an entry that is already in the list
	auto isListEntry = [](EMessage message)
	{
		return message == EMessage::APPEND_RAW_STRING || message == EMessage::APPEND_LOCAL_STRING || message == EMessage::APPEND_TEXTID_STRING;
	};

	size_t lastEntry = 0;
	for(size_t i = 0; i < message.size(); ++i)
		if(isListEntry(message.at(i)))
			lastEntry = i;

	size_t exSt = 0;
	size_t loSt = 0;
	size_t nums = 0;
	size_t textID = 0;
	std::string lista;
	for(int i = 0; i < message.size(); ++i)
	{
		if(i > 0 && isListEntry(message.at(i)))
		{
			if(i == lastEntry)
				lista += translator->translate("core.genrltxt", 141); //" and "
			else
				lista += ", ";
		}
		switch(message.at(i))
		{
			case EMessage::APPEND_RAW_STRING:
				lista += exactStrings.at(exSt++);
				break;
			case EMessage::APPEND_LOCAL_STRING:
				lista += getLocalString(translator, localStrings.at(loSt++));
				break;
			case EMessage::APPEND_TEXTID_STRING:
				lista += translator->translate(stringsTextID.at(textID++));
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
				lista.replace(lista.find("%s"), 2, getLocalString(translator, localStrings.at(loSt++)));
				break;
			case EMessage::REPLACE_TEXTID_STRING:
				lista.replace(lista.find("%s"), 2, translator->translate(stringsTextID.at(textID++)));
				break;
			case EMessage::REPLACE_NUMBER:
				lista.replace(lista.find("%d"), 2, std::to_string(numbers.at(nums++)));
				break;
			case EMessage::REPLACE_TOKEN_TEXTID:
				boost::replace_first(lista, exactStrings.at(exSt++), translator->translate(stringsTextID.at(textID++)));
				break;
			case EMessage::REPLACE_TOKEN_NUMBER:
				boost::replace_first(lista, exactStrings.at(exSt++), std::to_string(numbers.at(nums++)));
				break;
			case EMessage::REPLACE_TOKEN_RAW_STRING:
			{
				const std::string & token = exactStrings.at(exSt++);
				boost::replace_first(lista, token, exactStrings.at(exSt++));
				break;
			}
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
	appendTextID("vcmi.capitalColors", id.getNum());
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
	appendTextID(id.toResource()->getNameTextID());
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

void MetaString::replaceName(const PlayerColor & id)
{
	replaceTextID("vcmi.capitalColors", id.getNum());
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
	replaceTextID(id.toResource()->getNameTextID());
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
