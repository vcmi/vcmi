/*
 * JsonNode.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "JsonNode.h"

#include "ScopeGuard.h"

#include "HeroBonus.h"
#include "filesystem/CResourceLoader.h"
#include "VCMI_Lib.h" //for identifier resolution
#include "CModHandler.h"

using namespace JsonDetail;

class LibClasses;
class CModHandler;

static const JsonNode nullNode;

JsonNode::JsonNode(JsonType Type):
	type(DATA_NULL)
{
	setType(Type);
}

JsonNode::JsonNode(const char *data, size_t datasize):
	type(DATA_NULL)
{
	JsonParser parser(data, datasize);
	*this = parser.parse("<unknown>");
}

JsonNode::JsonNode(ResourceID && fileURI):
	type(DATA_NULL)
{
	auto file = CResourceHandler::get()->loadData(fileURI);

	JsonParser parser(reinterpret_cast<char*>(file.first.get()), file.second);
	*this = parser.parse(fileURI.getName());
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

bool JsonNode::operator == (const JsonNode &other) const
{
	if (getType() == other.getType())
	{
		switch(type)
		{
			break; case DATA_NULL:   return true;
			break; case DATA_BOOL:   return Bool() == other.Bool();
			break; case DATA_FLOAT:  return Float() == other.Float();
			break; case DATA_STRING: return String() == other.String();
			break; case DATA_VECTOR: return Vector() == other.Vector();
			break; case DATA_STRUCT: return Struct() == other.Struct();
		}
	}
	return false;
}

bool JsonNode::operator != (const JsonNode &other) const
{
	return !(*this == other);
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
		break; case DATA_STRING: data.String = new std::string();
		break; case DATA_VECTOR: data.Vector = new JsonVector();
		break; case DATA_STRUCT: data.Struct = new JsonMap();
	}
}

bool JsonNode::isNull() const
{
	return type == DATA_NULL;
}

void JsonNode::clear()
{
	setType(DATA_NULL);
}

bool & JsonNode::Bool()
{
	setType(DATA_BOOL);
	return data.Bool;
}

double & JsonNode::Float()
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

const double floatDefault = 0;
const double & JsonNode::Float() const
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

// to avoid duplicating const and non-const code
template<typename Node>
Node & resolvePointer(Node & in, const std::string & pointer)
{
	if (pointer.empty())
		return in;
	assert(pointer[0] == '/');

	size_t splitPos = pointer.find('/', 1);

	std::string entry   =   pointer.substr(1, splitPos -1);
	std::string remainer =  splitPos == std::string::npos ? "" : pointer.substr(splitPos);

	if (in.getType() == JsonNode::DATA_VECTOR)
	{
		if (entry.find_first_not_of("0123456789") != std::string::npos) // non-numbers in string
			throw std::runtime_error("Invalid Json pointer");

		if (entry.size() > 1 && entry[0] == '0') // leading zeros are not allowed
			throw std::runtime_error("Invalid Json pointer");

		size_t index = boost::lexical_cast<size_t>(entry);

		if (in.Vector().size() > index)
			return in.Vector()[index].resolvePointer(remainer);
	}
	return in[entry].resolvePointer(remainer);
}

const JsonNode & JsonNode::resolvePointer(const std::string &jsonPointer) const
{
	return ::resolvePointer(*this, jsonPointer);
}

JsonNode & JsonNode::resolvePointer(const std::string &jsonPointer)
{
	return ::resolvePointer(*this, jsonPointer);
}

////////////////////////////////////////////////////////////////////////////////

template<typename Iterator>
void JsonWriter::writeContainer(Iterator begin, Iterator end)
{
	if (begin == end)
		return;

	prefix += '\t';

	writeEntry(begin++);

	while (begin != end)
	{
		out<<",\n";
		writeEntry(begin++);
	}

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

JsonParser::JsonParser(const char * inputString, size_t stringSize):
	input(inputString, stringSize),
	lineCount(1),
	lineStart(0),
	pos(0)
{
}

JsonNode JsonParser::parse(std::string fileName)
{
	JsonNode root;

	extractValue(root);
	extractWhitespace(false);

	//Warn if there are any non-whitespace symbols left
	if (pos < input.size())
		error("Not all file was parsed!", true);

	if (!errors.empty())
	{
        logGlobal->warnStream()<<"File " << fileName << " is not a valid JSON file!";
        logGlobal->warnStream()<<errors;
	}
	return root;
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
		while (pos < input.size() && (ui8)input[pos] <= ' ')
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
	switch(input[pos])
	{
		break; case '\"': str += '\"';
		break; case '\\': str += '\\';
		break; case  '/': str += '/';
		break; case 'b': str += '\b';
		break; case 'f': str += '\f';
		break; case 'n': str += '\n';
		break; case 'r': str += '\r';
		break; case 't': str += '\t';
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
			pos++;
			if (pos == input.size())
				break;
			extractEscaping(str);
			first = pos + 1;
		}
		if (input[pos] == '\n') // end-of-line
		{
			str.append( &input[first], pos-first);
			return error("Closing quote not found!", true);
		}
		if ((unsigned char)(input[pos]) < ' ') // control character
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

	node.clear();
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
	double result=0;

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
		double fractMult = 0.1;
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
		("null",   JsonNode::DATA_NULL)   ("boolean", JsonNode::DATA_BOOL)
		("number", JsonNode::DATA_FLOAT)  ("string",  JsonNode::DATA_STRING)
		("array",  JsonNode::DATA_VECTOR) ("object",  JsonNode::DATA_STRUCT);

std::string JsonValidator::validateEnum(const JsonNode &node, const JsonVector &enumeration)
{
	BOOST_FOREACH(auto & enumEntry, enumeration)
	{
		if (node == enumEntry)
			return "";
	}
	return fail("Key must have one of predefined values");
}

std::string JsonValidator::validatesSchemaList(const JsonNode &node, const JsonNode &schemas, std::string errorMsg, std::function<bool(size_t)> isValid)
{
	if (!schemas.isNull())
	{
		std::string errors = "<tested schemas>\n";
		size_t result = 0;

		BOOST_FOREACH(auto & schema, schemas.Vector())
		{
			std::string error = validateNode(node, schema);
			if (error.empty())
			{
				result++;
			}
			else
			{
				errors += error;
				errors += "<end of schema>\n";
			}
		}
		if (isValid(result))
		{
			return "";
		}
		return fail(errorMsg) + errors;
	}
	return "";
}

std::string JsonValidator::validateNodeType(const JsonNode &node, const JsonNode &schema)
{
	std::string errors;

	// data must be valid against all schemas in the list
	errors += validatesSchemaList(node, schema["allOf"], "Failed to pass all schemas", [&](size_t count)
	{
		return count == schema["allOf"].Vector().size();
	});

	// data must be valid against any non-zero number of schemas in the list
	errors += validatesSchemaList(node, schema["anyOf"], "Failed to pass any schema", [&](size_t count)
	{
		return count > 0;
	});

	// data must be valid against one and only one schema
	errors += validatesSchemaList(node, schema["oneOf"], "Failed to pass one and only one schema", [&](size_t count)
	{
		return count == 1;
	});

	// data must NOT be valid against schema
	if (!schema["not"].isNull())
	{
		if (validateNode(node, schema["not"]).empty())
			errors += fail("Successful validation against negative check");
	}
	return errors;
}

// Basic checks common for any nodes
std::string JsonValidator::validateNode(const JsonNode &node, const JsonNode &schema)
{
	std::string errors;

	assert(!schema.isNull()); // can this error be triggered?

	if (node.isNull())
		return ""; // node not present. consider to be "valid"

	if (!schema["$ref"].isNull())
	{
		std::string URI = schema["$ref"].String();
		//node must be validated using schema pointed by this reference and not by data here
		//Local reference. Turn it into more easy to handle remote ref
		if (boost::algorithm::starts_with(URI, "#"))
			URI = usedSchemas.back() + URI;

		return validateRoot(node, URI);
	}

	// basic schema check
	auto & typeNode = schema["type"];
	if ( !typeNode.isNull())
	{
		JsonNode::JsonType type = stringToType.find(typeNode.String())->second;
		if(type != node.getType())
			return errors + fail("Type mismatch!"); // different type. Any other checks are useless
	}

	errors += validateNodeType(node, schema);

	// enumeration - data must be equeal to one of items in list
	if (!schema["enum"].isNull())
		errors += validateEnum(node, schema["enum"].Vector());

	// try to run any type-specific checks
	if (node.getType() == JsonNode::DATA_VECTOR) errors += validateVector(node, schema);
	if (node.getType() == JsonNode::DATA_STRUCT) errors += validateStruct(node, schema);
	if (node.getType() == JsonNode::DATA_STRING) errors += validateString(node, schema);
	if (node.getType() == JsonNode::DATA_FLOAT)  errors += validateNumber(node, schema);

	return errors;
}

std::string JsonValidator::validateVectorItem(const JsonVector items, const JsonNode & schema, const JsonNode & additional, size_t index)
{
	currentPath.push_back(JsonNode());
	currentPath.back().Float() = index;
	auto onExit = vstd::makeScopeGuard([&]
	{
		currentPath.pop_back();
	});

	if (!schema.isNull())
	{
		// case 1: schema is vector. Validate items agaist corresponding items in vector
		if (schema.getType() == JsonNode::DATA_VECTOR)
		{
			if (schema.Vector().size() > index)
				return validateNode(items[index], schema.Vector()[index]);
		}
		else // case 2: schema has to be struct. Apply it to all items, completely ignore additionalItems
		{
			return validateNode(items[index], schema);
		}
	}

	// othervice check against schema in additional items field
	if (additional.getType() == JsonNode::DATA_STRUCT)
		return validateNode(items[index], additional);

	// or, additionalItems field can be bool which indicates if such items are allowed
	if (!additional.isNull() && additional.Bool() == false) // present and set to false - error
		return fail("Unknown entry found");

	// by default - additional items are allowed
	return "";
}

//Checks "items" entry from schema (type-specific check for Vector)
std::string JsonValidator::validateVector(const JsonNode &node, const JsonNode &schema)
{
	std::string errors;
	auto & vector = node.Vector();

	{
		auto & items = schema["items"];
		auto & additional = schema["additionalItems"];

		for (size_t i=0; i<vector.size(); i++)
			errors += validateVectorItem(vector, items, additional, i);
	}

	if (vstd::contains(schema.Struct(), "maxItems") && vector.size() > schema["maxItems"].Float())
		errors += fail("Too many items in the list!");

	if (vstd::contains(schema.Struct(), "minItems") && vector.size() < schema["minItems"].Float())
		errors += fail("Too few items in the list");

	if (schema["uniqueItems"].Bool())
	{
		for (auto itA = vector.begin(); itA != vector.end(); itA++)
		{
			auto itB = itA;
			while (++itB != vector.end())
			{
				if (*itA == *itB)
					errors += fail("List must consist from unique items");
			}
		}
	}
	return errors;
}

std::string JsonValidator::validateStructItem(const JsonNode &node, const JsonNode & schema, const JsonNode & additional, std::string nodeName)
{
	currentPath.push_back(JsonNode());
	currentPath.back().String() = nodeName;
	auto onExit = vstd::makeScopeGuard([&]
	{
		currentPath.pop_back();
	});

	// there is schema specifically for this item
	if (!schema[nodeName].isNull())
		return validateNode(node, schema[nodeName]);

	// try generic additionalItems schema
	if (additional.getType() == JsonNode::DATA_STRUCT)
		return validateNode(node, additional);

	// or, additionalItems field can be bool which indicates if such items are allowed
	if (!additional.isNull() && additional.Bool() == false) // present and set to false - error
		return fail("Unknown entry found: " + nodeName);

	// by default - additional items are allowed
	return "";
}

//Checks "properties" entry from schema (type-specific check for Struct)
std::string JsonValidator::validateStruct(const JsonNode &node, const JsonNode &schema)
{
	std::string errors;
	auto & map = node.Struct();

	{
		auto & properties = schema["properties"];
		auto & additional = schema["additionalProperties"];

		BOOST_FOREACH(auto & entry, map)
			errors += validateStructItem(entry.second, properties, additional, entry.first);
	}

	BOOST_FOREACH(auto & required, schema["required"].Vector())
	{
		if (node[required.String()].isNull())
			errors += fail("Required entry " + required.String() + " is missing");
	}

	//Copy-paste from vector code. yay!
	if (vstd::contains(schema.Struct(), "maxProperties") && map.size() > schema["maxProperties"].Float())
		errors += fail("Too many items in the list!");

	if (vstd::contains(schema.Struct(), "minItems") && map.size() < schema["minItems"].Float())
		errors += fail("Too few items in the list");

	if (schema["uniqueItems"].Bool())
	{
		for (auto itA = map.begin(); itA != map.end(); itA++)
		{
			auto itB = itA;
			while (++itB != map.end())
			{
				if (itA->second == itB->second)
					errors += fail("List must consist from unique items");
			}
		}
	}

	// dependencies. Format is object/struct where key is the name of key in data
	// and value is either:
	// a) array of fields that must be present
	// b) struct with schema against which data should be valid
	// These checks are triggered only if key is present
	BOOST_FOREACH(auto & deps, schema["dependencies"].Struct())
	{
		if (vstd::contains(map, deps.first))
		{
			if (deps.second.getType() == JsonNode::DATA_VECTOR)
			{
				JsonVector depList = deps.second.Vector();
				BOOST_FOREACH(auto & depEntry, depList)
				{
					if (!vstd::contains(map, depEntry.String()))
						errors += fail("Property " + depEntry.String() + " required for " + deps.first + " is missing");
				}
			}
			else
			{
				if (!validateNode(node, deps.second).empty())
					errors += fail("Requirements for " + deps.first + " are not fulfilled");
			}
		}
	}

	// TODO: missing fields from draft v4
	// patternProperties
	return errors;
}

std::string JsonValidator::validateString(const JsonNode &node, const JsonNode &schema)
{
	std::string errors;
	auto & string = node.String();

	if (vstd::contains(schema.Struct(), "maxLength") && string.size() > schema["maxLength"].Float())
		errors += fail("String too long");

	if (vstd::contains(schema.Struct(), "minLength") && string.size() < schema["minLength"].Float())
		errors += fail("String too short");

	// TODO: missing fields from draft v4
	// pattern
	return errors;
}

std::string JsonValidator::validateNumber(const JsonNode &node, const JsonNode &schema)
{
	std::string errors;
	auto & value = node.Float();
	if (vstd::contains(schema.Struct(), "maximum"))
	{
		if (schema["exclusiveMaximum"].Bool())
		{
			if (value >= schema["maximum"].Float())
				errors += fail("Value is too large");
		}
		else
		{
			if (value >  schema["maximum"].Float())
				errors += fail("Value is too large");
		}
	}

	if (vstd::contains(schema.Struct(), "minimum"))
	{
		if (schema["exclusiveMinimum"].Bool())
		{
			if (value <= schema["minimum"].Float())
				errors += fail("Value is too small");
		}
		else
		{
			if (value <  schema["minimum"].Float())
				errors += fail("Value is too small");
		}
	}

	if (vstd::contains(schema.Struct(), "multipleOf"))
	{
		double result = value / schema["multipleOf"].Float();
		if (floor(result) != result)
			errors += ("Value is not divisible");
	}
	return errors;
}

//basic schema validation (like checking $schema entry).
std::string JsonValidator::validateRoot(const JsonNode &node, std::string schemaName)
{
	const JsonNode & schema = JsonUtils::getSchema(schemaName);

	usedSchemas.push_back(schemaName.substr(0, schemaName.find('#')));
	auto onExit = vstd::makeScopeGuard([&]
	{
		usedSchemas.pop_back();
	});

	if (!schema.isNull())
		return validateNode(node, schema);
	else
		return fail("Schema not found!");
}

std::string JsonValidator::fail(const std::string &message)
{
	std::string errors;
	errors += "At ";
	if (!currentPath.empty())
	{
		BOOST_FOREACH(const JsonNode &path, currentPath)
		{
			errors += "/";
			if (path.getType() == JsonNode::DATA_STRING)
				errors += path.String();
			else
				errors += boost::lexical_cast<std::string>(static_cast<unsigned>(path.Float()));
		}
	}
	else
		errors += "<root>";
	errors += "\n\t Error: " + message + "\n";
	return errors;
}

bool JsonValidator::validate(const JsonNode &root, std::string schemaName, std::string name)
{
	std::string errors = validateRoot(root, schemaName);

	if (!errors.empty())
	{
		logGlobal->warnStream() << "Data in " << name << " is invalid!";
		logGlobal->warnStream() << errors;
	}

	return errors.empty();
}

///JsonUtils

void JsonUtils::parseTypedBonusShort(const JsonVector& source, Bonus *dest)
{
	dest->val = source[1].Float();
	resolveIdentifier(source[2],dest->subtype);
	dest->additionalInfo = source[3].Float();
	dest->duration = Bonus::PERMANENT; //TODO: handle flags (as integer)
	dest->turnsRemain = 0;	
}


Bonus * JsonUtils::parseBonus (const JsonVector &ability_vec) //TODO: merge with AddAbility, create universal parser for all bonus properties
{
	Bonus * b = new Bonus();
	std::string type = ability_vec[0].String();
	auto it = bonusNameMap.find(type);
	if (it == bonusNameMap.end())
	{
        logGlobal->errorStream() << "Error: invalid ability type " << type;
		return b;
	}
	b->type = it->second;
	
	parseTypedBonusShort(ability_vec, b);
	return b;
}
template <typename T>
const T & parseByMap(const std::map<std::string, T> & map, const JsonNode * val, std::string err)
{
	static T defaultValue = T();
	if (!val->isNull())
	{
		std::string type = val->String();
		auto it = map.find(type);
		if (it == map.end())
		{
            logGlobal->errorStream() << "Error: invalid " << err << type;
			return defaultValue;
		}
		else
		{
			return it->second;
		}
	}
	else
		return defaultValue;
}

void JsonUtils::resolveIdentifier (si32 &var, const JsonNode &node, std::string name)
{
	const JsonNode *value;
	value = &node[name];
	if (!value->isNull())
	{
		switch (value->getType())
		{
			case JsonNode::DATA_FLOAT:
				var = value->Float();
				break;
			case JsonNode::DATA_STRING:
				VLC->modh->identifiers.requestIdentifier (value->String(), [&](si32 identifier)
				{
					var = identifier;
				});
				break;
			default:
                logGlobal->errorStream() << "Error! Wrong indentifier used for value of " << name;
		}
	}
}

void JsonUtils::resolveIdentifier (const JsonNode &node, si32 &var)
{
	switch (node.getType())
	{
		case JsonNode::DATA_FLOAT:
			var = node.Float();
			break;
		case JsonNode::DATA_STRING:
			VLC->modh->identifiers.requestIdentifier (node.String(), [&](si32 identifier)
			{
				var = identifier;
			});
			break;
		default:
            logGlobal->errorStream() << "Error! Wrong indentifier used for identifier!";
	}
}

Bonus * JsonUtils::parseBonus (const JsonNode &ability)
{

	Bonus * b = new Bonus();
	const JsonNode *value;

	std::string type = ability["type"].String();
	auto it = bonusNameMap.find(type);
	if (it == bonusNameMap.end())
	{
        logGlobal->errorStream() << "Error: invalid ability type " << type;
		return b;
	}
	b->type = it->second;

	resolveIdentifier (b->subtype, ability, "subtype");

	b->val = ability["val"].Float();

	value = &ability["valueType"];
	if (!value->isNull())
		b->valType = static_cast<Bonus::ValueType>(parseByMap(bonusValueMap, value, "value type "));

	resolveIdentifier (b->additionalInfo, ability, "addInfo");

	b->turnsRemain = ability["turns"].Float();

	b->sid = ability["sourceID"].Float();

	b->description = ability["description"].String();

	value = &ability["effectRange"];
	if (!value->isNull())
		b->effectRange = static_cast<Bonus::LimitEffect>(parseByMap(bonusLimitEffect, value, "effect range "));

	value = &ability["duration"];
	if (!value->isNull())
	{
		switch (value->getType())
		{
		case JsonNode::DATA_STRING:
			b->duration = parseByMap(bonusDurationMap, value, "duration type ");
			break;
		case JsonNode::DATA_VECTOR:
			{
				ui16 dur = 0;
				BOOST_FOREACH (const JsonNode & d, value->Vector())
				{
					dur |= parseByMap(bonusDurationMap, &d, "duration type ");
				}
				b->duration = dur;
			}
			break;
		default:
            logGlobal->errorStream() << "Error! Wrong bonus duration format.";
		}
	}

	value = &ability["source"];
	if (!value->isNull())
		b->source = static_cast<Bonus::BonusSource>(parseByMap(bonusSourceMap, value, "source type "));

	value = &ability["limiters"];
	if (!value->isNull())
	{
		BOOST_FOREACH (const JsonNode & limiter, value->Vector())
		{
			switch (limiter.getType())
			{
				case JsonNode::DATA_STRING: //pre-defined limiters
					b->limiter = parseByMap(bonusLimiterMap, &limiter, "limiter type ");
					break;
				case JsonNode::DATA_STRUCT: //customizable limiters
					{
						shared_ptr<ILimiter> l;
						if (limiter["type"].String() == "CREATURE_TYPE_LIMITER")
						{
							shared_ptr<CCreatureTypeLimiter> l2 = make_shared<CCreatureTypeLimiter>(); //TODO: How the hell resolve pointer to creature?
							const JsonVector vec = limiter["parameters"].Vector();
							VLC->modh->identifiers.requestIdentifier(std::string("creature.") + vec[0].String(), [=](si32 creature)
							{
								l2->setCreature (CreatureID(creature));
							});
							if (vec.size() > 1)
							{
								l2->includeUpgrades = vec[1].Bool();
							}
							else
								l2->includeUpgrades = false;

							l = l2;
						}
						if (limiter["type"].String() == "HAS_ANOTHER_BONUS_LIMITER")
						{
							shared_ptr<HasAnotherBonusLimiter> l2 = make_shared<HasAnotherBonusLimiter>();
							const JsonVector vec = limiter["parameters"].Vector();
							std::string anotherBonusType = vec[0].String();

							auto it = bonusNameMap.find (anotherBonusType);
							if (it == bonusNameMap.end())
							{
                                logGlobal->errorStream() << "Error: invalid ability type " << anotherBonusType;
								continue;
							}
							l2->type = it->second;

							if (vec.size() > 1 )
							{
								resolveIdentifier (vec[1], l2->subtype);
								l2->isSubtypeRelevant = true;
							}
							l = l2;
						}
						b->addLimiter(l);
					}
					break;
			}
		}
	}

	value = &ability["propagator"];
	if (!value->isNull())
		b->propagator = parseByMap(bonusPropagatorMap, value, "propagator type ");

	return b;
}

//returns first Key with value equal to given one
template<class Key, class Val>
Key reverseMapFirst(const Val & val, const std::map<Key, Val> map)
{
	BOOST_FOREACH(auto it, map)
	{
		if(it.second == val)
		{
			return it.first;
		}
	}
	assert(0);
	return "";
}

void JsonUtils::unparseBonus( JsonNode &node, const Bonus * bonus )
{
	node["type"].String() = reverseMapFirst<std::string, Bonus::BonusType>(bonus->type, bonusNameMap);
	node["subtype"].Float() = bonus->subtype;
	node["val"].Float() = bonus->val;
	node["valueType"].String() = reverseMapFirst<std::string, Bonus::ValueType>(bonus->valType, bonusValueMap);
	node["additionalInfo"].Float() = bonus->additionalInfo;
	node["turns"].Float() = bonus->turnsRemain;
	node["sourceID"].Float() = bonus->source;
	node["description"].String() = bonus->description;
	node["effectRange"].String() = reverseMapFirst<std::string, Bonus::LimitEffect>(bonus->effectRange, bonusLimitEffect);
	node["duration"].String() = reverseMapFirst<std::string, ui16>(bonus->duration, bonusDurationMap);
	node["source"].String() = reverseMapFirst<std::string, Bonus::BonusSource>(bonus->source, bonusSourceMap);
	if(bonus->limiter)
	{
		node["limiter"].String() = reverseMapFirst<std::string, TLimiterPtr>(bonus->limiter, bonusLimiterMap);
	}
	if(bonus->propagator)
	{
		node["propagator"].String() = reverseMapFirst<std::string, TPropagatorPtr>(bonus->propagator, bonusPropagatorMap);
	}
}

void minimizeNode(JsonNode & node, const JsonNode & schema)
{
	if (schema["type"].String() == "object")
	{
		std::set<std::string> foundEntries;

		BOOST_FOREACH(auto & entry, schema["required"].Vector())
		{
			std::string name = entry.String();
			foundEntries.insert(name);

			minimizeNode(node[name], schema["properties"][name]);

			if (vstd::contains(node.Struct(), name) &&
				node[name] == schema["properties"][name]["default"])
			{
				node.Struct().erase(name);
			}
		}

		// erase all unhandled entries
		for (auto it = node.Struct().begin(); it != node.Struct().end();)
		{
			if (!vstd::contains(foundEntries, it->first))
				it = node.Struct().erase(it);
			else
				it++;
		}
	}
}

void JsonUtils::minimize(JsonNode & node, std::string schemaName)
{
	minimizeNode(node, getSchema(schemaName));
}

// FIXME: except for several lines function is identical to minimizeNode. Some way to reduce duplication?
void maximizeNode(JsonNode & node, const JsonNode & schema)
{
	// "required" entry can only be found in object/struct
	if (schema["type"].String() == "object")
	{
		std::set<std::string> foundEntries;

		// check all required entries that have default version
		BOOST_FOREACH(auto & entry, schema["required"].Vector())
		{
			std::string name = entry.String();
			foundEntries.insert(name);

			if (node[name].isNull() &&
				!schema["properties"][name]["default"].isNull())
			{
				node[name] = schema["properties"][name]["default"];
			}
			maximizeNode(node[name], schema["properties"][name]);
		}

		// erase all unhandled entries
		for (auto it = node.Struct().begin(); it != node.Struct().end();)
		{
			if (!vstd::contains(foundEntries, it->first))
				it = node.Struct().erase(it);
			else
				it++;
		}
	}
}

void JsonUtils::maximize(JsonNode & node, std::string schemaName)
{
	maximizeNode(node, getSchema(schemaName));
}

bool JsonUtils::validate(const JsonNode &node, std::string schemaName, std::string dataName)
{
	JsonValidator validator;
	return validator.validate(node, schemaName, dataName);
}

const JsonNode & getSchemaByName(std::string name)
{
	// cached schemas to avoid loading json data multiple times
	static std::map<std::string, JsonNode> loadedSchemas;

	if (vstd::contains(loadedSchemas, name))
		return loadedSchemas[name];

	std::string filename = "config/schemas/" + name + ".json";

	if (CResourceHandler::get()->existsResource(ResourceID(filename)))
	{
		loadedSchemas[name] = JsonNode(ResourceID(filename));
		return loadedSchemas[name];
	}

    logGlobal->errorStream() << "Error: missing schema with name " << name << "!";
	assert(0);
	return nullNode;
}

const JsonNode & JsonUtils::getSchema(std::string URI)
{
	std::vector<std::string> segments;

	size_t posColon = URI.find(':');
	size_t posHash  = URI.find('#');
	assert(posColon != std::string::npos);

	std::string protocolName = URI.substr(0, posColon);
	std::string filename =     URI.substr(posColon + 1, posHash - posColon - 1);

	if (protocolName != "vcmi")
	{
        logGlobal->errorStream() << "Error: unsupported URI protocol for schema: " << segments[0];
		return nullNode;
	}

	// check if json pointer if present (section after hash in string)
	if (posHash == std::string::npos || posHash == URI.size() - 1)
		return getSchemaByName(filename);
	else
		return getSchemaByName(filename).resolvePointer(URI.substr(posHash + 1));
}

void JsonUtils::merge(JsonNode & dest, JsonNode & source)
{
	if (dest.getType() == JsonNode::DATA_NULL)
	{
		std::swap(dest, source);
		return;
	}

	switch (source.getType())
	{
		break; case JsonNode::DATA_NULL:   dest.clear();
		break; case JsonNode::DATA_BOOL:   std::swap(dest.Bool(), source.Bool());
		break; case JsonNode::DATA_FLOAT:  std::swap(dest.Float(), source.Float());
		break; case JsonNode::DATA_STRING: std::swap(dest.String(), source.String());
		break; case JsonNode::DATA_VECTOR:
		{
			size_t total = std::min(source.Vector().size(), dest.Vector().size());

			for (size_t i=0; i< total; i++)
				merge(dest.Vector()[i], source.Vector()[i]);

			if (source.Vector().size() < dest.Vector().size())
			{
				//reserve place and *move* data from source to dest
				source.Vector().reserve(source.Vector().size() + dest.Vector().size());

				std::move(source.Vector().begin(), source.Vector().end(),
				          std::back_inserter(dest.Vector()));
			}
		}
		break; case JsonNode::DATA_STRUCT:
		{
			//recursively merge all entries from struct
			BOOST_FOREACH(auto & node, source.Struct())
				merge(dest[node.first], node.second);
		}
	}
}

void JsonUtils::mergeCopy(JsonNode & dest, JsonNode source)
{
	// uses copy created in stack to safely merge two nodes
	merge(dest, source);
}

JsonNode JsonUtils::assembleFromFiles(std::vector<std::string> files)
{
	JsonNode result;

	BOOST_FOREACH(std::string file, files)
	{
		JsonNode section(ResourceID(file, EResType::TEXT));
		merge(result, section);
	}
	return result;
}
