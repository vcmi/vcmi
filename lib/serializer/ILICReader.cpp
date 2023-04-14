/*
 * JsonTreeSerializer.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "ILICReader.h"

#include "../JsonNode.h"

#include <vstd/StringUtils.h>

VCMI_LIB_NAMESPACE_BEGIN

void ILICReader::readLICPart(const JsonNode & part, const JsonSerializeFormat::TDecoder & decoder, bool val, std::vector<bool> & value) const
{
	for(const auto & index : part.Vector())
	{
		const std::string & identifier = index.String();
		const std::string type = typeid(decltype(this)).name();

		const si32 rawId = decoder(identifier);
		if(rawId >= 0)
		{
			if(rawId < value.size())
				value[rawId] = val;
			else
				logGlobal->error("%s::serializeLIC: id out of bounds %d", type, rawId);
		}
	}
}

void ILICReader::readLICPart(const JsonNode & part, const JsonSerializeFormat::TDecoder & decoder, std::set<si32> & value) const
{
	for(const auto & index : part.Vector())
	{
		const std::string & identifier = index.String();

		const si32 rawId = decoder(identifier);
		if(rawId != -1)
			value.insert(rawId);
	}
}

VCMI_LIB_NAMESPACE_END
