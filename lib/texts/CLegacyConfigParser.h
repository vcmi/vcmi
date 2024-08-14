/*
 * CLegacyConfigParser.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "filesystem/ResourcePath.h"

VCMI_LIB_NAMESPACE_BEGIN

/// Parser for any text files from H3
class DLL_LINKAGE CLegacyConfigParser
{
	std::string fileEncoding;

	std::unique_ptr<char[]> data;
	const char * curr;
	const char * end;

	/// extracts part of quoted string.
	std::string extractQuotedPart();

	/// extracts quoted string. Any end of lines are ignored, double-quote is considered as "escaping"
	std::string extractQuotedString();

	/// extracts non-quoted string
	std::string extractNormalString();

	/// reads "raw" string without encoding conversion
	std::string readRawString();

public:
	/// read one entry from current line. Return ""/0 if end of line reached
	std::string readString();
	float readNumber();

	template <typename numeric>
	std::vector<numeric> readNumArray(size_t size)
	{
		std::vector<numeric> ret;
		ret.reserve(size);
		while (size--)
			ret.push_back(readNumber());
		return ret;
	}

	/// returns true if next entry is empty
	bool isNextEntryEmpty() const;

	/// end current line
	bool endLine();

	explicit CLegacyConfigParser(const TextPath & URI);
};

VCMI_LIB_NAMESPACE_END
