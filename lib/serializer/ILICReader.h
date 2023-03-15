/*
 * ILICReader.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "JsonTreeSerializer.h"

VCMI_LIB_NAMESPACE_BEGIN

class DLL_LINKAGE ILICReader
{
protected:
	void readLICPart(const JsonNode & part, const JsonSerializeFormat::TDecoder & decoder, bool val, std::vector<bool> & value) const;
	void readLICPart(const JsonNode & part, const JsonSerializeFormat::TDecoder & decoder, std::set<si32> & value) const;
};

VCMI_LIB_NAMESPACE_END
