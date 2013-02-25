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
	//Create empty node
	JsonNode(JsonType Type = DATA_NULL);
	//Create tree from Json-formatted input
	explicit JsonNode(const char * data, size_t datasize);
	//Create tree from JSON file
 	explicit JsonNode(ResourceID && fileURI);
	//Copy c-tor
	JsonNode(const JsonNode &copy);

	~JsonNode();

	void swap(JsonNode &b);
	JsonNode& operator =(JsonNode node);

	bool operator == (const JsonNode &other) const;
	bool operator != (const JsonNode &other) const;

	//Convert node to another type. Converting to NULL will clear all data
	void setType(JsonType Type);
	JsonType getType() const;

	bool isNull() const;

	//non-const accessors, node will change type on type mismatch
	bool & Bool();
	double & Float();
	std::string & String();
	JsonVector & Vector();
	JsonMap & Struct();

	//const accessors, will cause assertion failure on type mismatch
	const bool & Bool() const;
	const double & Float() const;
	const std::string & String() const;
	const JsonVector & Vector() const;
	const JsonMap & Struct() const;

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
		// simple saving - save json in its string interpretation
		if (h.saving)
		{
			std::ostringstream stream;
			stream << *this;
			std::string str = stream.str();
			h & str;
		}
		else
		{
			std::string str;
			h & str;
			JsonNode(str.c_str(), str.size()).swap(*this);
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
	 * @brief recursivly merges source into dest, replacing identical fields
	 * struct : recursively calls this function
	 * arrays : each entry will be merged recursively
	 * values : value in source will replace value in dest
	 * null   : if value in source is present but set to null it will delete entry in dest
	 * @note this function will destroy data in source
	 */
	DLL_LINKAGE void merge(JsonNode & dest, JsonNode & source);
	
	/**
	 * @brief recursivly merges source into dest, replacing identical fields
	 * struct : recursively calls this function
	 * arrays : each entry will be merged recursively
	 * values : value in source will replace value in dest
	 * null   : if value in source is present but set to null it will delete entry in dest
	 * @note this function will preserve data stored in source by creating copy
	 */ 
	DLL_LINKAGE void mergeCopy(JsonNode & dest, JsonNode source);

	/**
	 * @brief generate one Json structure from multiple files
	 * @param files - list of filenames with parts of json structure
	 */
	DLL_LINKAGE JsonNode assembleFromFiles(std::vector<std::string> files);

	/// removes all nodes that are identical to default entry in schema
	DLL_LINKAGE void minimize(JsonNode & node, const JsonNode& schema);

	/// check schema
	DLL_LINKAGE void validate(JsonNode & node, const JsonNode& schema);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////
// End of public section of the file. Anything below should be only used internally in JsonNode.cpp //
//////////////////////////////////////////////////////////////////////////////////////////////////////

namespace JsonDetail
{
	// convertion helpers for JsonNode::convertTo (partial template function instantiation is illegal in c++)

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
			BOOST_FOREACH(auto entry, node.Struct())
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
			BOOST_FOREACH(auto entry, node.Vector())
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
			BOOST_FOREACH(auto entry, node.Vector())
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

	class JsonWriter
	{
		//prefix for each line (tabulation)
		std::string prefix;
		std::ostream &out;
	public:
		template<typename Iterator>
		void writeContainer(Iterator begin, Iterator end);
		void writeEntry(JsonMap::const_iterator entry);
		void writeEntry(JsonVector::const_iterator entry);
		void writeString(const std::string &string);
		void writeNode(const JsonNode &node);
		JsonWriter(std::ostream &output, const JsonNode &node);
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
		JsonParser(const char * inputString, size_t stringSize, JsonNode &root);
	};

	//Internal class for Json validation, used automaticaly in JsonNode constructor. Behaviour:
	// - "schema" entry from root node is used for validation and will be removed
	// - any missing entries will be replaced with default value from schema (if present)
	// - if entry uses different type than defined in schema it will be removed
	// - entries nod described in schema will be kept unchanged
	class JsonValidator
	{
		std::string errors;     // Contains description of all encountered errors
		std::list<std::string> currentPath; // path from root node to current one
		bool minimize;

		bool validateType(JsonNode &node, const JsonNode &schema, JsonNode::JsonType type);
		bool validateSchema(JsonNode::JsonType &type, const JsonNode &schema);
		bool validateNode(JsonNode &node, const JsonNode &schema, const std::string &name);
		bool validateItems(JsonNode &node, const JsonNode &schema);
		bool validateProperties(JsonNode &node, const JsonNode &schema);

		bool addMessage(const std::string &message);
	public:
		// validate node with "schema" entry
		JsonValidator(JsonNode &root, bool minimize=false);
		// validate with external schema
		JsonValidator(JsonNode &root, const JsonNode &schema, bool minimize=false);
	};

} // namespace JsonDetail

template<typename Type>
Type JsonNode::convertTo() const
{
	return JsonDetail::JsonConverter<Type>::convert(*this);
}