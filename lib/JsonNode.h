#pragma once

#include "../global.h"

#include <iostream>
#include <map>
#include <string>
#include <vector>

class JsonNode;
typedef std::map <std::string, JsonNode> JsonMap;
typedef std::vector <JsonNode> JsonVector;

class DLL_EXPORT JsonNode
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
		float Float;
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
	explicit JsonNode(std::string input);
	//Copy c-tor
	JsonNode(const JsonNode &copy);

	~JsonNode();

	// Deep copy of this node
	JsonNode& operator =(const JsonNode &node);

	//Convert node to another type. Converting to NULL will clear all data
	void setType(JsonType Type);
	JsonType getType() const;

	bool isNull() const;

	//non-const acessors, node will change type on type mismatch
	bool & Bool();
	int & Int();
	float & Float();
	std::string & String();
	JsonVector & Vector();
	JsonMap & Struct();

	//const acessors, will cause assertion failure on type mismatch
	const bool & Bool() const;
	const int & Int() const;
	const float & Float() const;
	const std::string & String() const;
	const JsonVector & Vector() const;
	const JsonMap & Struct() const;

	//formatted output of this node in JSON format
	void write(std::ostream &out, std::string prefix="") const;

	//operator [], for structs only - get child node by name
	JsonNode & operator[](std::string child);
	const JsonNode & operator[](std::string child) const;

	//error value for const operator[]
	static const JsonNode nullNode;
};

std::ostream & operator<<(std::ostream &out, const JsonNode &node);


//Internal class for std::string -> JsonNode conversion
class JsonParser
{
	std::string errors;     // Contains description of all encountered errors
	const std::string input;// Input data
	unsigned int lineCount; // Currently parsed line, starting from 1
	size_t lineStart;       // Position of current line start
	size_t pos;             // Current position of parser

	//Helpers
	bool extractEscaping(std::string &str);
	bool extractLiteral(const std::string &literal);
	bool extractString(std::string &string);
	bool extractWhitespace();
	bool extractSeparator();

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
	JsonParser(const std::string inputString, JsonNode &root);
};
