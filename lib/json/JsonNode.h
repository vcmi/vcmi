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

struct Bonus;
class CSelector;
class CAddInfo;
class ILimiter;

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

public:
	/// free to use metadata fields
	std::string meta;
	/// meta-flags like override
	std::vector<std::string> flags;

	JsonNode() = default;

	/// Create single node with specified value
	explicit JsonNode(bool boolean);
	explicit JsonNode(int32_t number);
	explicit JsonNode(uint32_t number);
	explicit JsonNode(int64_t number);
	explicit JsonNode(double number);
	explicit JsonNode(const std::string & string);

	/// Create tree from Json-formatted input
	explicit JsonNode(const std::byte * data, size_t datasize);

	/// Create tree from JSON file
	explicit JsonNode(const JsonPath & fileURI);
	explicit JsonNode(const JsonPath & fileURI, const std::string & modName);
	explicit JsonNode(const JsonPath & fileURI, bool & isValidSyntax);

	bool operator == (const JsonNode &other) const;
	bool operator != (const JsonNode &other) const;

	void setMeta(const std::string & metadata, bool recursive = true);

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

	/// returns bool or bool equivalent of string value if 'success' is true, or false otherwise
	bool TryBoolFromString(bool & success) const;

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
	const JsonNode & operator[](size_t  child) const;

	std::string toCompactString() const;
	std::string toString() const;
	std::vector<std::byte> toBytes() const;

	template <typename Handler> void serialize(Handler &h)
	{
		h & meta;
		h & flags;
		h & data;
	}
};

namespace JsonDetail
{
	// conversion helpers for JsonNode::convertTo (partial template function instantiation is illegal in c++)

	template <typename T, int arithm>
	struct JsonConvImpl;

	template <typename T>
	struct JsonConvImpl<T, 1>
	{
		static T convertImpl(const JsonNode & node)
		{
			return T((int)node.Float());
		}
	};

	template <typename T>
	struct JsonConvImpl<T, 0>
	{
		static T convertImpl(const JsonNode & node)
		{
			return T(node.Float());
		}
	};

	template<typename Type>
	struct JsonConverter
	{
		static Type convert(const JsonNode & node)
		{
			///this should be triggered only for numeric types and enums
			static_assert(std::is_arithmetic_v<Type> || std::is_enum_v<Type> || std::is_class_v<Type>, "Unsupported type for JsonNode::convertTo()!");
			return JsonConvImpl<Type, std::is_enum_v<Type> || std::is_class_v<Type> >::convertImpl(node);

		}
	};

	template<typename Type>
	struct JsonConverter<std::map<std::string, Type> >
	{
		static std::map<std::string, Type> convert(const JsonNode & node)
		{
			std::map<std::string, Type> ret;
			for (const JsonMap::value_type & entry : node.Struct())
			{
				ret.insert(entry.first, entry.second.convertTo<Type>());
			}
			return ret;
		}
	};

	template<typename Type>
	struct JsonConverter<std::set<Type> >
	{
		static std::set<Type> convert(const JsonNode & node)
		{
			std::set<Type> ret;
			for(const JsonVector::value_type & entry : node.Vector())
			{
				ret.insert(entry.convertTo<Type>());
			}
			return ret;
		}
	};

	template<typename Type>
	struct JsonConverter<std::vector<Type> >
	{
		static std::vector<Type> convert(const JsonNode & node)
		{
			std::vector<Type> ret;
			for (const JsonVector::value_type & entry: node.Vector())
			{
				ret.push_back(entry.convertTo<Type>());
			}
			return ret;
		}
	};

	template<>
	struct JsonConverter<std::string>
	{
		static std::string convert(const JsonNode & node)
		{
			return node.String();
		}
	};

	template<>
	struct JsonConverter<bool>
	{
		static bool convert(const JsonNode & node)
		{
			return node.Bool();
		}
	};
}

template<typename Type>
Type JsonNode::convertTo() const
{
	return JsonDetail::JsonConverter<Type>::convert(*this);
}

VCMI_LIB_NAMESPACE_END
