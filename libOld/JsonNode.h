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

class JsonNode;
typedef std::map <std::string, JsonNode> JsonMap;
typedef std::vector <JsonNode> JsonVector;

DLL_LINKAGE std::ostream & operator<<(std::ostream &out, const JsonNode &node);

struct Bonus;
class ResourceID;

class DLL_LINKAGE JsonNode
{
public:
	enum JsonType
	{
		DATA_NULL,
		DATA_BOOL,
		DATA_FLOAT,
		DATA_STRING,
		DATA_VECTOR,
		DATA_STRUCT
	};

private:
	union JsonData
	{
		bool Bool;
		double Float;
		std::string* String;
		JsonVector* Vector;
		JsonMap* Struct;
	};

	JsonType type;
	JsonData data;

public:
	/// free to use metadata field
	std::string meta;

	//Create empty node
	JsonNode(JsonType Type = DATA_NULL);
	//Create tree from Json-formatted input
	explicit JsonNode(const char * data, size_t datasize);
	//Create tree from JSON file
 	explicit JsonNode(ResourceID && fileURI);
 	explicit JsonNode(const ResourceID & fileURI);
	explicit JsonNode(ResourceID && fileURI, bool & isValidSyntax);
	//Copy c-tor
	JsonNode(const JsonNode &copy);

	~JsonNode();

	void swap(JsonNode &b);
	JsonNode& operator =(JsonNode node);

	bool operator == (const JsonNode &other) const;
	bool operator != (const JsonNode &other) const;

	void setMeta(std::string metadata, bool recursive = true);

	/// Convert node to another type. Converting to nullptr will clear all data
	void setType(JsonType Type);
	JsonType getType() const;

	bool isNull() const;
	/// removes all data from node and sets type to null
	void clear();

	/// non-const accessors, node will change type on type mismatch
	bool & Bool();
	double & Float();
	std::string & String();
	JsonVector & Vector();
	JsonMap & Struct();

	/// const accessors, will cause assertion failure on type mismatch
	const bool & Bool() const;
	const double & Float() const;
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
	JsonNode & operator[](std::string child);
	const JsonNode & operator[](std::string child) const;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & meta;
		h & type;
		switch (type) {
			break; case DATA_NULL:
			break; case DATA_BOOL:   h & data.Bool;
			break; case DATA_FLOAT:  h & data.Float;
			break; case DATA_STRING: h & data.String;
			break; case DATA_VECTOR: h & data.Vector;
			break; case DATA_STRUCT: h & data.Struct;
		}
	}
};

namespace JsonUtils
{
	/**
	 * @brief parse short bonus format, excluding type
	 * @note sets duration to Permament
	 */
	DLL_LINKAGE void parseTypedBonusShort(const JsonVector &source, Bonus *dest);

	///
	DLL_LINKAGE Bonus * parseBonus (const JsonVector &ability_vec);
	DLL_LINKAGE Bonus * parseBonus (const JsonNode &bonus);
	DLL_LINKAGE void unparseBonus (JsonNode &node, const Bonus * bonus);
	DLL_LINKAGE void resolveIdentifier (si32 &var, const JsonNode &node, std::string name);
	DLL_LINKAGE void resolveIdentifier (const JsonNode &node, si32 &var);

	/**
	 * @brief recursively merges source into dest, replacing identical fields
	 * struct : recursively calls this function
	 * arrays : each entry will be merged recursively
	 * values : value in source will replace value in dest
	 * null   : if value in source is present but set to null it will delete entry in dest
	 * @note this function will destroy data in source
	 */
	DLL_LINKAGE void merge(JsonNode & dest, JsonNode & source);

	/**
	 * @brief recursively merges source into dest, replacing identical fields
	 * struct : recursively calls this function
	 * arrays : each entry will be merged recursively
	 * values : value in source will replace value in dest
	 * null   : if value in source is present but set to null it will delete entry in dest
	 * @note this function will preserve data stored in source by creating copy
	 */
	DLL_LINKAGE void mergeCopy(JsonNode & dest, JsonNode source);

    /** @brief recursively merges descendant into copy of base node
     * Result emulates inheritance semantic
     *
     *
     */
	DLL_LINKAGE void inherit(JsonNode & descendant, const JsonNode & base);

	/**
	 * @brief generate one Json structure from multiple files
	 * @param files - list of filenames with parts of json structure
	 */
	DLL_LINKAGE JsonNode assembleFromFiles(std::vector<std::string> files);
	DLL_LINKAGE JsonNode assembleFromFiles(std::vector<std::string> files, bool & isValid);

	/// This version loads all files with same name (overridden by mods)
	DLL_LINKAGE JsonNode assembleFromFiles(std::string filename);

	/**
	 * @brief removes all nodes that are identical to default entry in schema
	 * @param node - JsonNode to minimize
	 * @param schemaName - name of schema to use
	 * @note for minimizing data must be valid against given schema
	 */
	DLL_LINKAGE void minimize(JsonNode & node, std::string schemaName);
	/// opposed to minimize, adds all missing, required entries that have default value
	DLL_LINKAGE void maximize(JsonNode & node, std::string schemaName);

	/**
	* @brief validate node against specified schema
	* @param node - JsonNode to check
	* @param schemaName - name of schema to use
	* @param dataName - some way to identify data (printed in console in case of errors)
	* @returns true if data in node fully compilant with schema
	*/
	DLL_LINKAGE bool validate(const JsonNode & node, std::string schemaName, std::string dataName);

	/// get schema by json URI: vcmi:<name of file in schemas directory>#<entry in file, optional>
	/// example: schema "vcmi:settings" is used to check user settings
	DLL_LINKAGE const JsonNode & getSchema(std::string URI);
}

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
			return node.Float();
		}
	};

	template<typename Type>
	struct JsonConverter
	{
		static Type convert(const JsonNode & node)
		{
			///this should be triggered only for numeric types and enums
			static_assert(boost::mpl::or_<std::is_arithmetic<Type>, std::is_enum<Type>, boost::is_class<Type> >::value, "Unsupported type for JsonNode::convertTo()!");
			return JsonConvImpl<Type, boost::mpl::or_<std::is_enum<Type>, boost::is_class<Type> >::value >::convertImpl(node);

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
} // namespace JsonDetail

template<typename Type>
Type JsonNode::convertTo() const
{
	return JsonDetail::JsonConverter<Type>::convert(*this);
}
