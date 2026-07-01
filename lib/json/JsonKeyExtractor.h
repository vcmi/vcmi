#pragma once

#include "../GameLibrary.h"
#include "../modding/IdentifierStorage.h"
#include "GameConstants.h"
#include "JsonNode.h"

VCMI_LIB_NAMESPACE_BEGIN

class DLL_LINKAGE JsonKeyExtractor
{

public:
	JsonKeyExtractor(IGameInfoCallback * cb);

	using Variables = std::map<std::string, int>;

	template<typename IdentifierType>
	std::set<IdentifierType> filterKeys(const JsonNode & value, const std::set<IdentifierType> & valuesSet, const Variables & variables = {});

	template<typename IdentifierType>
	IdentifierType decodeKey(const std::string & modScope, const std::string & value, const Variables & variables);

	si32 loadVariable(const std::string & variableGroup, const std::string & value, const Variables & variables, si32 defaultValue);

private:
	template<typename IdentifierType>
	IdentifierType decodeKey(const JsonNode & value, const Variables & variables);

	template<typename IdentifierType>
	std::set<IdentifierType> filterKeysTyped(const JsonNode & value, const std::set<IdentifierType> & valuesSet);

	std::set<ArtifactID> filterKeysTyped(const JsonNode & value, const std::set<ArtifactID> & valuesSet);
	std::set<SpellID> filterKeysTyped(const JsonNode & value, const std::set<SpellID> & valuesSet);
	std::set<SecondarySkill> filterKeysTyped(const JsonNode & value, const std::set<SecondarySkill> & valuesSet);

	IGameInfoCallback * cb;
};

template<typename IdentifierType>
std::set<IdentifierType> JsonKeyExtractor::filterKeys(const JsonNode & value, const std::set<IdentifierType> & valuesSet, const Variables & variables)
{
    // if value is string do not filter value through valueSet. It allows objects like scholar to override map settings
    // (i.e grant a skill that is blocked by map settings). It is intentional.
    // TODO: refactor class so this behaviour is clearly reflected by api.
	if(value.isString())
        return {decodeKey<IdentifierType>(value, variables)};

	assert(value.isStruct());

	if(value.isStruct())
	{
		if(!value["type"].isNull())
			return filterKeys(value["type"], valuesSet, variables);

		std::set<IdentifierType> filteredTypes = filterKeysTyped(value, valuesSet);

		if(!value["anyOf"].isNull())
		{
			std::set<IdentifierType> filteredAnyOf;
			for(const auto & entry : value["anyOf"].Vector())
			{
				std::set<IdentifierType> subset = filterKeys(entry, valuesSet, variables);
				filteredAnyOf.insert(subset.begin(), subset.end());
			}

			vstd::erase_if(
				filteredTypes,
				[&filteredAnyOf](const IdentifierType & filteredValue)
				{
					return filteredAnyOf.count(filteredValue) == 0;
				}
			);
		}

		if(!value["noneOf"].isNull())
		{
			for(const auto & entry : value["noneOf"].Vector())
			{
				std::set<IdentifierType> subset = filterKeys(entry, valuesSet, variables);
				for(auto bannedEntry : subset)
					filteredTypes.erase(bannedEntry);
			}
		}

		return filteredTypes;
	}
	return valuesSet;
}

template<typename IdentifierType>
IdentifierType JsonKeyExtractor::decodeKey(const std::string & modScope, const std::string & value, const Variables & variables)
{
	if(value.empty() || value[0] != '@')
		return IdentifierType(LIBRARY->identifiers()->getIdentifier(modScope, IdentifierType::entityType(), value).value_or(-1));
	else
		return loadVariable(IdentifierType::entityType(), value, variables, IdentifierType::NONE);
}

template<>
inline PrimarySkill JsonKeyExtractor::decodeKey(const std::string & modScope, const std::string & value, const Variables & variables)
{
	if(value.empty() || value[0] != '@')
		return PrimarySkill(*LIBRARY->identifiers()->getIdentifier(modScope, "primarySkill", value));
	else
		return PrimarySkill(loadVariable("primarySkill", value, variables, PrimarySkill::NONE.getNum()));
}

template<typename IdentifierType>
IdentifierType JsonKeyExtractor::decodeKey(const JsonNode & value, const Variables & variables)
{
	if(value.String().empty() || value.String()[0] != '@')
		return IdentifierType(LIBRARY->identifiers()->getIdentifier(IdentifierType::entityType(), value).value_or(-1));
	else
		return loadVariable(IdentifierType::entityType(), value.String(), variables, IdentifierType::NONE);
}

template<>
inline ArtifactPosition JsonKeyExtractor::decodeKey(const JsonNode & value, const Variables & variables)
{
	return ArtifactPosition::decode(value.String());
}

template<>
inline PlayerColor JsonKeyExtractor::decodeKey(const JsonNode & value, const Variables & variables)
{
	return PlayerColor(*LIBRARY->identifiers()->getIdentifier("playerColor", value));
}

template<>
inline PrimarySkill JsonKeyExtractor::decodeKey(const JsonNode & value, const Variables & variables)
{
	return PrimarySkill(*LIBRARY->identifiers()->getIdentifier("primarySkill", value));
}


/// Method that allows type-specific object filtering
/// Default implementation is to accept all input objects
template<typename IdentifierType>
std::set<IdentifierType> JsonKeyExtractor::filterKeysTyped(const JsonNode & value, const std::set<IdentifierType> & valuesSet)
{
	return valuesSet;
}

VCMI_LIB_NAMESPACE_END
