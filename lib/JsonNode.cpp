#define VCMI_DLL
#include "JsonNode.h"

#include <boost/assign.hpp>
#include <boost/foreach.hpp>

#include <assert.h>
#include <fstream>
#include <sstream>
#include <iostream>

const JsonNode JsonNode::nullNode;

JsonNode::JsonNode(JsonType Type):
	type(DATA_NULL)
{
	setType(Type);
}

JsonNode::JsonNode(const char *data, size_t datasize):
	type(DATA_NULL)
{
	JsonParser parser(data, datasize, *this);
	JsonValidator validator(*this);
}

JsonNode::JsonNode(std::string filename):
	type(DATA_NULL)
{
	FILE * file = fopen(filename.c_str(), "rb");
	fseek(file, 0, SEEK_END);
	size_t datasize = ftell(file);
	fseek(file, 0, SEEK_SET);

	char *input = new char[datasize];
	datasize = fread((void*)input, 1, datasize, file);
	fclose(file);

	JsonParser parser(input, datasize, *this);
	JsonValidator validator(*this);
	delete [] input;
}

JsonNode::JsonNode(const JsonNode &copy):
	type(DATA_NULL)
{
	setType(copy.getType());
	switch(type)
	{
		break; case DATA_NULL:
		break; case DATA_BOOL:   Bool() =   copy.Bool();
		break; case DATA_FLOAT:  Float() =  copy.Float();
		break; case DATA_STRING: String() = copy.String();
		break; case DATA_VECTOR: Vector() = copy.Vector();
		break; case DATA_STRUCT: Struct() = copy.Struct();
	}
}

JsonNode::~JsonNode()
{
	setType(DATA_NULL);
}

void JsonNode::swap(JsonNode &b)
{
	using std::swap;
	swap(data, b.data);
	swap(type, b.type);
}

JsonNode & JsonNode::operator =(JsonNode node)
{
	swap(node);
	return *this;
}

JsonNode::JsonType JsonNode::getType() const
{
	return type;
}

void JsonNode::setType(JsonType Type)
{
	if (type == Type)
		return;

	//Reset node to NULL
	if (Type != DATA_NULL)
		setType(DATA_NULL);

	switch (type)
	{
		break; case DATA_STRING:  delete data.String;
		break; case DATA_VECTOR:  delete data.Vector;
		break; case DATA_STRUCT:  delete data.Struct;
		break; default:
		break;
	}
	//Set new node type
	type = Type;
	switch(type)
	{
		break; case DATA_NULL:
		break; case DATA_BOOL:   data.Bool = false;
		break; case DATA_FLOAT:  data.Float = 0;
		break; case DATA_STRING: data.String = new std::string;
		break; case DATA_VECTOR: data.Vector = new JsonVector;
		break; case DATA_STRUCT: data.Struct = new JsonMap;
	}
}

bool JsonNode::isNull() const
{
	return type == DATA_NULL;
}

bool & JsonNode::Bool()
{
	setType(DATA_BOOL);
	return data.Bool;
}

float & JsonNode::Float()
{
	setType(DATA_FLOAT);
	return data.Float;
}

std::string & JsonNode::String()
{
	setType(DATA_STRING);
	return *data.String;
}

JsonVector & JsonNode::Vector()
{
	setType(DATA_VECTOR);
	return *data.Vector;
}

JsonMap & JsonNode::Struct()
{
	setType(DATA_STRUCT);
	return *data.Struct;
}

const bool boolDefault = false;
const bool & JsonNode::Bool() const
{
	if (type == DATA_NULL)
		return boolDefault;
	assert(type == DATA_BOOL);
	return data.Bool;
}

const float floatDefault = 0;
const float & JsonNode::Float() const
{
	if (type == DATA_NULL)
		return floatDefault;
	assert(type == DATA_FLOAT);
	return data.Float;
}

const std::string stringDefault = std::string();
const std::string & JsonNode::String() const
{
	if (type == DATA_NULL)
		return stringDefault;
	assert(type == DATA_STRING);
	return *data.String;
}

const JsonVector vectorDefault = JsonVector();
const JsonVector & JsonNode::Vector() const
{
	if (type == DATA_NULL)
		return vectorDefault;
	assert(type == DATA_VECTOR);
	return *data.Vector;
}

const JsonMap mapDefault = JsonMap();
const JsonMap & JsonNode::Struct() const
{
	if (type == DATA_NULL)
		return mapDefault;
	assert(type == DATA_STRUCT);
	return *data.Struct;
}

JsonNode & JsonNode::operator[](std::string child)
{
	return Struct()[child];
}

const JsonNode & JsonNode::operator[](std::string child) const
{
	JsonMap::const_iterator it = Struct().find(child);
	if (it != Struct().end())
		return it->second;
	return nullNode;
}

////////////////////////////////////////////////////////////////////////////////

template<typename Iterator>
void JsonWriter::writeContainer(Iterator begin, Iterator end)
{
	if (begin == end)
		return;
	
	prefix += '\t';
	end--;
	while (begin != end)
	{
		writeEntry(begin++);
		out<<",\n";
	}
	
	writeEntry(begin);
	out<<"\n";
	prefix.resize(prefix.size()-1);
}

void JsonWriter::writeEntry(JsonMap::const_iterator entry)
{
	out << prefix;
	writeString(entry->first);
	out << " : ";
	writeNode(entry->second);
}

void JsonWriter::writeEntry(JsonVector::const_iterator entry)
{
	out << prefix;
	writeNode(*entry);
}

void JsonWriter::writeString(const std::string &string)
{
	static const std::string escaped = "\"\\/\b\f\n\r\t";

	out <<'\"';
	size_t pos=0, start=0;
	for (; pos<string.size(); pos++)
	{
		size_t escapedChar = escaped.find(string[pos]);
		
		if (escapedChar != std::string::npos)
		{
			out.write(string.data()+start, pos - start);
			out << '\\' << escaped[escapedChar];
			start = pos;
		}
	}
	out.write(string.data()+start, pos - start);
	out <<'\"';
}

void JsonWriter::writeNode(const JsonNode &node)
{
	switch(node.getType())
	{
		break; case JsonNode::DATA_NULL:
			out << "null";

		break; case JsonNode::DATA_BOOL:
			if (node.Bool())
				out << "true";
			else
				out << "false";

		break; case JsonNode::DATA_FLOAT:
			out << node.Float();

		break; case JsonNode::DATA_STRING:
			writeString(node.String());

		break; case JsonNode::DATA_VECTOR:
			out << "[" << "\n";
			writeContainer(node.Vector().begin(), node.Vector().end());
			out << prefix << "]";

		break; case JsonNode::DATA_STRUCT:
			out << "{" << "\n";
			writeContainer(node.Struct().begin(), node.Struct().end());
			out << prefix << "}";
	}
}

JsonWriter::JsonWriter(std::ostream &output, const JsonNode &node):
	out(output)
{
	writeNode(node);
}

std::ostream & operator<<(std::ostream &out, const JsonNode &node)
{
	JsonWriter(out, node);
	return out << "\n";
}

////////////////////////////////////////////////////////////////////////////////

JsonParser::JsonParser(const char * inputString, size_t stringSize, JsonNode &root):
	input(inputString, stringSize),
	lineCount(1),
	lineStart(0),
	pos(0)
{
	extractValue(root);
	extractWhitespace(false);

	//Warn if there are any non-whitespace symbols left
	if (pos < input.size())
		error("Not all file was parsed!", true);

	//TODO: better way to show errors (like printing file name as well)
	std::cout<<errors;
}

bool JsonParser::extractSeparator()
{
	if (!extractWhitespace())
		return false;

	if ( input[pos] !=':')
		return error("Separator expected");

	pos++;
	return true;
}

bool JsonParser::extractValue(JsonNode &node)
{
	if (!extractWhitespace())
		return false;

	switch (input[pos])
	{
		case '\"': return extractString(node);
		case 'n' : return extractNull(node);
		case 't' : return extractTrue(node);
		case 'f' : return extractFalse(node);
		case '{' : return extractStruct(node);
		case '[' : return extractArray(node);
		case '-' : return extractFloat(node);
		default:
		{
			if (input[pos] >= '0' && input[pos] <= '9')
				return extractFloat(node);
			return error("Value expected!");
		}
	}
}

bool JsonParser::extractWhitespace(bool verbose)
{
	while (true)
	{
		while (pos < input.size() && (unsigned char)input[pos] <= ' ')
		{
			if (input[pos] == '\n')
			{
				lineCount++;
				lineStart = pos+1;
			}
			pos++;
		}
		if (pos >= input.size() || input[pos] != '/')
			break;

		pos++;
		if (pos == input.size())
			break;
		if (input[pos] == '/')
			pos++;
		else
			error("Comments must consist from two slashes!", true);

		while (pos < input.size() && input[pos] != '\n')
			pos++;
	}

	if (pos >= input.size() && verbose)
		return error("Unexpected end of file!");
	return true;
}

bool JsonParser::extractEscaping(std::string &str)
{
	switch(input[pos++])
	{
		break; case '\"': str += '\"';
		break; case '\\': str += '\\';
		break; case  '/': str += '/';
		break; case '\b': str += '\b';
		break; case '\f': str += '\f';
		break; case '\n': str += '\n';
		break; case '\r': str += '\r';
		break; case '\t': str += '\t';
		break; default: return error("Unknown escape sequence!", true);
	};
	return true;
}

bool JsonParser::extractString(std::string &str)
{
	if (input[pos] != '\"')
		return error("String expected!");
	pos++;

	size_t first = pos;

	while (pos != input.size())
	{
		if (input[pos] == '\"') // Correct end of string
		{
			str.append( &input[first], pos-first);
			pos++;
			return true;
		}
		if (input[pos] == '\\') // Escaping
		{
			str.append( &input[first], pos-first);
			first = pos++;
			if (pos == input.size())
				break;
			extractEscaping(str);
		}
		if (input[pos] == '\n') // end-of-line
		{
			str.append( &input[first], pos-first);
			return error("Closing quote not found!", true);
		}
		if (input[pos] < ' ') // control character
		{
			str.append( &input[first], pos-first);
			first = pos+1;
			error("Illegal character in the string!", true);
		}
		pos++;
	}
	return error("Unterminated string!");
}

bool JsonParser::extractString(JsonNode &node)
{
	std::string str;
	if (!extractString(str))
		return false;

	node.setType(JsonNode::DATA_STRING);
	node.String() = str;
	return true;
}

bool JsonParser::extractLiteral(const std::string &literal)
{
	if (literal.compare(0, literal.size(), &input[pos], literal.size()) != 0)
	{
		while (pos < input.size() && ((input[pos]>'a' && input[pos]<'z')
		                           || (input[pos]>'A' && input[pos]<'Z')))
			pos++;
		return error("Unknown literal found", true);
	}

	pos += literal.size();
	return true;
}

bool JsonParser::extractNull(JsonNode &node)
{
	if (!extractLiteral("null"))
		return false;

	node.setType(JsonNode::DATA_NULL);
	return true;
}

bool JsonParser::extractTrue(JsonNode &node)
{
	if (!extractLiteral("true"))
		return false;

	node.Bool() = true;
	return true;
}

bool JsonParser::extractFalse(JsonNode &node)
{
	if (!extractLiteral("false"))
		return false;

	node.Bool() = false;
	return true;
}

bool JsonParser::extractStruct(JsonNode &node)
{
	node.setType(JsonNode::DATA_STRUCT);
	pos++;

	if (!extractWhitespace())
		return false;

	//Empty struct found
	if (input[pos] == '}')
	{
		pos++;
		return true;
	}

	while (true)
	{
		if (!extractWhitespace())
			return false;

		std::string key;
		if (!extractString(key))
			return false;

		if (node.Struct().find(key) != node.Struct().end())
			error("Dublicated element encountered!", true);

		if (!extractSeparator())
			return false;

		if (!extractElement(node.Struct()[key], '}'))
			return false;

		if (input[pos] == '}')
		{
			pos++;
			return true;
		}
	}
}

bool JsonParser::extractArray(JsonNode &node)
{
	pos++;
	node.setType(JsonNode::DATA_VECTOR);

	if (!extractWhitespace())
		return false;

	//Empty array found
	if (input[pos] == ']')
	{
		pos++;
		return true;
	}

	while (true)
	{
		//NOTE: currently 50% of time is this vector resizing.
		//May be useful to use list during parsing and then swap() all items to vector
		node.Vector().resize(node.Vector().size()+1);

		if (!extractElement(node.Vector().back(), ']'))
			return false;

		if (input[pos] == ']')
		{
			pos++;
			return true;
		}
	}
}

bool JsonParser::extractElement(JsonNode &node, char terminator)
{
	if (!extractValue(node))
		return false;

	if (!extractWhitespace())
		return false;

	bool comma = (input[pos] == ',');
	if (comma )
	{
		pos++;
		if (!extractWhitespace())
			return false;
	}

	if (input[pos] == terminator)
		return true;

	if (!comma)
		error("Comma expected!", true);

	return true;
}

bool JsonParser::extractFloat(JsonNode &node)
{
	assert(input[pos] == '-' || (input[pos] >= '0' && input[pos] <= '9'));
	bool negative=false;
	float result=0;

	if (input[pos] == '-')
	{
		pos++;
		negative = true;
	}

	if (input[pos] < '0' || input[pos] > '9')
		return error("Number expected!");

	//Extract integer part
	while (input[pos] >= '0' && input[pos] <= '9')
	{
		result = result*10+(input[pos]-'0');
		pos++;
	}

	if (input[pos] == '.')
	{
		//extract fractional part
		pos++;
		float fractMult = 0.1;
		if (input[pos] < '0' || input[pos] > '9')
			return error("Decimal part expected!");

		while (input[pos] >= '0' && input[pos] <= '9')
		{
			result = result + fractMult*(input[pos]-'0');
			fractMult /= 10;
			pos++;
		}
	}
	//TODO: exponential part
	if (negative)
		result = -result;

	node.setType(JsonNode::DATA_FLOAT);
	node.Float() = result;
	return true;
}

bool JsonParser::error(const std::string &message, bool warning)
{
	std::ostringstream stream;
	std::string type(warning?" warning: ":" error: ");

	stream << "At line " << lineCount << ", position "<<pos-lineStart
	       << type << message <<"\n";
	errors += stream.str();

	return warning;
}

static const std::map<std::string, JsonNode::JsonType> stringToType =
	boost::assign::map_list_of
		("null",   JsonNode::DATA_NULL)   ("bool",   JsonNode::DATA_BOOL)
		("number", JsonNode::DATA_FLOAT)  ("string", JsonNode::DATA_STRING)
		("array",  JsonNode::DATA_VECTOR) ("object", JsonNode::DATA_STRUCT);

//Check current schema entry for validness and converts "type" string to JsonType
bool JsonValidator::validateSchema(JsonNode::JsonType &type, const JsonNode &schema)
{
	if (schema.isNull())
		return addMessage("Missing schema for current entry!");

	const JsonNode &nodeType = schema["type"];
	if (nodeType.isNull())
		return addMessage("Entry type is not defined in schema!");

	if (nodeType.getType() != JsonNode::DATA_STRING)
		return addMessage("Entry type must be string!");

	std::map<std::string, JsonNode::JsonType>::const_iterator iter = stringToType.find(nodeType.String());

	if (iter == stringToType.end())
		return addMessage("Unknown entry type found!");

	type = iter->second;
	return true;
}

//Replaces node with default value if needed and calls type-specific validators
bool JsonValidator::validateType(JsonNode &node, const JsonNode &schema, JsonNode::JsonType type)
{
	if (node.isNull())
	{
		const JsonNode & defaultValue = schema["default"];
		if (defaultValue.isNull())
			return addMessage("Null entry without default entry!");
		else
			node = defaultValue;
	}

	if (type != node.getType())
	{
		node.setType(JsonNode::DATA_NULL);
		return addMessage("Type mismatch!");
	}

	if (type == JsonNode::DATA_VECTOR)
		return validateItems(node, schema["items"]);

	if (type == JsonNode::DATA_STRUCT)
		return validateProperties(node, schema["properties"]);

	return true;
}

// Basic checks common for any nodes
bool JsonValidator::validateNode(JsonNode &node, const JsonNode &schema, const std::string &name)
{
	currentPath.push_back(name);

	JsonNode::JsonType type = JsonNode::DATA_NULL;
	if (!validateSchema(type, schema))
	{
		currentPath.pop_back();
		return false;
	}

	if (!validateType(node, schema, type))
	{
		currentPath.pop_back();
		return false;
	}

	currentPath.pop_back();
	return true;
}

//Checks "items" entry from schema (type-specific check for Vector)
bool JsonValidator::validateItems(JsonNode &node, const JsonNode &schema)
{
	JsonNode::JsonType type = JsonNode::DATA_NULL;
	if (!validateSchema(type, schema))
		return false;

	BOOST_FOREACH(JsonNode &entry, node.Vector())
	{
		if (!validateType(entry, schema, type))
			return false;
	}
	return true;
}

//Checks "propertries" entry from schema (type-specific check for Struct)
//Function is similar to merging of two sorted lists - check every entry that present in one of the input nodes
bool JsonValidator::validateProperties(JsonNode &node, const JsonNode &schema)
{
	if (schema.isNull())
		return addMessage("Properties entry is missing for struct in schema");

	JsonMap::iterator nodeIter = node.Struct().begin();
	JsonMap::const_iterator schemaIter = schema.Struct().begin();

	while (nodeIter != node.Struct().end() && schemaIter != schema.Struct().end())
	{
		std::string current = std::min(nodeIter->first, schemaIter->first);
		validateNode(node[current], schema[current], current);

		if (nodeIter->first < schemaIter->first)
			nodeIter++;
		else
		if (schemaIter->first < nodeIter->first)
			schemaIter++;
		else
		{
			nodeIter++;
			schemaIter++;
		}
	}
	while (nodeIter != node.Struct().end())
	{
		validateNode(nodeIter->second, JsonNode(), nodeIter->first);
		nodeIter++;
	}

	while (schemaIter != schema.Struct().end())
	{
		validateNode(node[schemaIter->first], schemaIter->second, schemaIter->first);
		schemaIter++;
	}
	return true;
}

bool JsonValidator::addMessage(const std::string &message)
{
	std::ostringstream stream;

	stream << "At ";
	BOOST_FOREACH(const std::string &path, currentPath)
		stream << path<<"/";
	stream << "\t Error: " << message <<"\n";
	errors += stream.str();
	return false;
}

JsonValidator::JsonValidator(JsonNode &root)
{
	const JsonNode schema = root["schema"];

	if (!schema.isNull())
	{
		root.Struct().erase("schema");
		validateProperties(root, schema);
	}
	//This message is quite annoying now - most files do not have schemas. May be re-enabled later
	//else
	//	addMessage("Schema not found!", true);

	//TODO: better way to show errors (like printing file name as well)
	std::cout<<errors;
}
