/*
 * JsonParser.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "JsonNode.h"

VCMI_LIB_NAMESPACE_BEGIN

//Tiny string class that uses const char* as data for speed, members are private
//for ease of debugging and some compatibility with std::string
class constString
{
	const char *data;
	const size_t datasize;

public:
	constString(const char * inputString, size_t stringSize):
		data(inputString),
		datasize(stringSize)
	{
	}

	inline size_t size() const
	{
		return datasize;
	};

	inline const char& operator[] (size_t position)
	{
		assert (position < datasize);
		return data[position];
	}
};

//Internal class for string -> JsonNode conversion
class JsonParser
{
	std::string errors;     // Contains description of all encountered errors
	constString input;      // Input data
	ui32 lineCount; // Currently parsed line, starting from 1
	size_t lineStart;       // Position of current line start
	size_t pos;             // Current position of parser

	//Helpers
	bool extractEscaping(std::string &str);
	bool extractLiteral(const std::string &literal);
	bool extractString(std::string &string);
	bool extractWhitespace(bool verbose = true);
	bool extractSeparator();
	bool extractElement(JsonNode &node, char terminator);

	//Methods for extracting JSON data
	bool extractArray(JsonNode &node);
	bool extractFalse(JsonNode &node);
	bool extractFloat(JsonNode &node);
	bool extractNull(JsonNode &node);
	bool extractString(JsonNode &node);
	bool extractStruct(JsonNode &node);
	bool extractTrue(JsonNode &node);
	bool extractValue(JsonNode &node);

	//Add error\warning message to list
	bool error(const std::string &message, bool warning=false);

public:
	JsonParser(const char * inputString, size_t stringSize);

	/// do actual parsing. filename is name of file that will printed to console if any errors were found
	JsonNode parse(const std::string & fileName);

	/// returns true if parsing was successful
	bool isValid();
};

VCMI_LIB_NAMESPACE_END
