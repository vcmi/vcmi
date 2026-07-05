/*
 * JsonWriter.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "JsonNode.h"

class JsonWriter
{
	//prefix for each line (tabulation)
	std::string prefix;
	std::ostream & out;
	//sets whether compact nodes are written in single-line format
	bool compact;
	//tracks whether we are currently using single-line format
	bool compactMode = false;

public:
	template<typename Iterator>
	void writeContainer(Iterator begin, Iterator end);
	void writeEntry(JsonMap::const_iterator entry);
	void writeEntry(JsonVector::const_iterator entry);
	void writeString(const std::string & string);
	void writeNode(const JsonNode & node);
	JsonWriter(std::ostream & output, bool compact);
};
