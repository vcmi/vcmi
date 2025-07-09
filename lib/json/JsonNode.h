/*
 * JsonNode.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../filesystem/ResourcePath.h"

VCMI_LIB_NAMESPACE_BEGIN

class JsonNode;
using JsonMap = std::map<std::string, JsonNode>;
using JsonVector = std::vector<JsonNode>;

struct DLL_LINKAGE JsonParsingSettings
{
	enum class JsonFormatMode
	{
		JSON, // strict implementation of json format
		JSONC, // json format that also allows comments that start from '//'
		JSON5 // Partial support of 'json5' format
	};

	JsonFormatMode mode = JsonFormatMode::JSON5;

	/// Maximum depth of elements
	uint32_t maxDepth = 30;

	/// If set to true, parser will throw on any encountered error
	bool strict = false;
};

class DLL_LINKAGE JsonNode
{
public:
	enum class JsonType
	{
		DATA_NULL,
		DATA_BOOL,
		DATA_FLOAT,
		DATA_STRING,
		DATA_VECTOR,
		DATA_STRUCT,
		DATA_INTEGER
	};

private:
	using JsonData = std::variant<std::monostate, bool, double, std::string, JsonVector, JsonMap, int64_t>;

	JsonData data;

	/// Mod-origin of this particular field
	std::string modScope;

	bool overrideFlag = false;

public:
	JsonNode() = default;

	/// Create single node with specified value
	explicit JsonNode(bool boolean);
	explicit JsonNode(int32_t number);
	explicit JsonNode(uint32_t number);
	explicit JsonNode(int64_t number);
	explicit JsonNode(double number);
	explicit JsonNode(const char * string);
	explicit JsonNode(const std::string & string);

	/// Create tree from map
	explicit JsonNode(const JsonMap & map);

	/// Create tree from Json-formatted input
	explicit JsonNode(const std::byte * data, size_t datasize, const std::string & fileName);
	explicit JsonNode(const std::byte * data, size_t datasize, const JsonParsingSettings & parserSettings, const std::string & fileName);

	/// Create tree from JSON file
	explicit JsonNode(const JsonPath & fileURI);
	explicit JsonNode(const JsonPath & fileURI, const JsonParsingSettings & parserSettings);
	explicit JsonNode(const JsonPath & fileURI, const std::string & modName);
	explicit JsonNode(const JsonPath & fileURI, const std::string & modName, bool & isValidSyntax);

	bool operator==(const JsonNode & other) const;
	bool operator!=(const JsonNode & other) const;

	const std::string & getModScope() const;
	void setModScope(const std::string & metadata, bool recursive = true);

	void setOverrideFlag(bool value);
	bool getOverrideFlag() const;

	/// Convert node to another type. Converting to nullptr will clear all data
	void setType(JsonType Type);
	JsonType getType() const;

	bool isNull() const;
	bool isNumber() const;
	bool isString() const;
	bool isVector() const;
	bool isStruct() const;
	/// true if node contains not-null data that cannot be extended via merging
	/// used for generating common base node from multiple nodes (e.g. bonuses)
	bool containsBaseData() const;
	bool isCompact() const;
	/// removes all data from node and sets type to null
	void clear();

	/// non-const accessors, node will change type on type mismatch
	bool & Bool();
	double & Float();
	si64 & Integer();
	std::string & String();
	JsonVector & Vector();
	JsonMap & Struct();

	/// const accessors, will cause assertion failure on type mismatch
	bool Bool() const;
	///float and integer allowed
	double Float() const;
	///only integer allowed
	si64 Integer() const;
	const std::string & String() const;
	const JsonVector & Vector() const;
	const JsonMap & Struct() const;

	/// returns resolved "json pointer" (string in format "/path/to/node")
	const JsonNode & resolvePointer(const std::string & jsonPointer) const;
	JsonNode & resolvePointer(const std::string & jsonPointer);

	/// convert json tree into specified type. Json tree must have same type as Type
	/// Valid types: bool, string, any numeric, map and vector
	/// example: convertTo< std::map< std::vector<int> > >();
	template<typename Type>
	Type convertTo() const;

	//operator [], for structs only - get child node by name
	JsonNode & operator[](const std::string & child);
	const JsonNode & operator[](const std::string & child) const;

	JsonNode & operator[](size_t child);
	const JsonNode & operator[](size_t child) const;

	std::string toCompactString() const;
	std::string toString() const;
	std::vector<std::byte> toBytes() const;

	template<typename Handler>
	void serialize(Handler & h)
	{
		h & modScope;
		h & overrideFlag;
		h & data;
	}
};

namespace JsonDetail
{

inline void convert(bool & value, const JsonNode & node)
{
	value = node.Bool();
}

template<typename T>
auto convert(T & value, const JsonNode & node) -> std::enable_if_t<std::is_integral_v<T>>
{
	value = node.Integer();
}

template<typename T>
auto convert(T & value, const JsonNode & node) -> std::enable_if_t<std::is_floating_point_v<T>>
{
	value = node.Float();
}

inline void convert(std::string & value, const JsonNode & node)
{
	value = node.String();
}

template<typename Type>
void convert(std::map<std::string, Type> & value, const JsonNode & node)
{
	value.clear();
	for(const JsonMap::value_type & entry : node.Struct())
		value.emplace(entry.first, entry.second.convertTo<Type>());
}

template<typename Type>
void convert(std::set<Type> & value, const JsonNode & node)
{
	value.clear();
	for(const JsonVector::value_type & entry : node.Vector())
	{
		value.insert(entry.convertTo<Type>());
	}
}

template<typename Type>
void convert(std::vector<Type> & value, const JsonNode & node)
{
	value.clear();
	for(const JsonVector::value_type & entry : node.Vector())
	{
		value.push_back(entry.convertTo<Type>());
	}
}

}

template<typename Type>
Type JsonNode::convertTo() const
{
	Type result;
	JsonDetail::convert(result, *this);
	return result;
}

VCMI_LIB_NAMESPACE_END
