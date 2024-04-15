/*
 * ModVerificationInfo.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "ModVerificationInfo.h"

#include "../json/JsonNode.h"

JsonNode ModVerificationInfo::jsonSerializeList(const ModCompatibilityInfo & input)
{
	JsonNode output;

	for(const auto & mod : input)
	{
		JsonNode modWriter;
		modWriter["modId"].String() = mod.first;
		modWriter["name"].String() = mod.second.name;
		if (!mod.second.parent.empty())
			modWriter["parent"].String() = mod.second.parent;
		modWriter["version"].String() = mod.second.version.toString();
		output.Vector().push_back(modWriter);
	}

	return output;
}

ModCompatibilityInfo ModVerificationInfo::jsonDeserializeList(const JsonNode & input)
{
	ModCompatibilityInfo output;

	for(const auto & mod : input.Vector())
	{
		ModVerificationInfo info;
		info.version = CModVersion::fromString(mod["version"].String());
		info.name = mod["name"].String();
		info.parent = mod["parent"].String();
		info.checksum = 0;
		info.impactsGameplay = true;

		if(!mod["modId"].isNull())
			output[mod["modId"].String()] = info;
		else
			output[mod["name"].String()] = info;
	}

	return output;
}
