/*
 * JsonDetail.h, part of VCMI engine
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
	JsonWriter(std::ostream & output, bool compact = false);
};

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
class DLL_LINKAGE JsonParser
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

//Internal class for Json validation. Mostly compilant with json-schema v4 draft
namespace Validation
{
	/// struct used to pass data around during validation
	struct ValidationData
	{
		/// path from root node to current one.
		/// JsonNode is used as variant - either string (name of node) or as float (index in list)
		std::vector<JsonNode> currentPath;

		/// Stack of used schemas. Last schema is the one used currently.
		/// May contain multiple items in case if remote references were found
		std::vector<std::string> usedSchemas;

		/// generates error message
		std::string makeErrorMessage(const std::string &message);
	};

	using TFormatValidator = std::function<std::string(const JsonNode &)>;
	using TFormatMap = std::unordered_map<std::string, TFormatValidator>;
	using TFieldValidator = std::function<std::string(ValidationData &, const JsonNode &, const JsonNode &, const JsonNode &)>;
	using TValidatorMap = std::unordered_map<std::string, TFieldValidator>;

	/// map of known fields in schema
	const TValidatorMap & getKnownFieldsFor(JsonNode::JsonType type);
	const TFormatMap & getKnownFormats();

	std::string check(const std::string & schemaName, const JsonNode & data);
	std::string check(const std::string & schemaName, const JsonNode & data, ValidationData & validator);
	std::string check(const JsonNode & schema, const JsonNode & data, ValidationData & validator);
}

VCMI_LIB_NAMESPACE_END
