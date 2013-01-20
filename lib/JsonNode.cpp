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

#include "HeroBonus.h"
#include "Filesystem/CResourceLoader.h"
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
	JsonParser parser(data, datasize, *this);
	JsonValidator validator(*this);
}

JsonNode::JsonNode(ResourceID && fileURI):
	type(DATA_NULL)
{
	std::string filename = CResourceHandler::get()->getResourceName(fileURI);
	FILE * file = fopen(filename.c_str(), "rb");
	if (!file)
	{
		tlog1 << "Failed to open file " << filename << "\n";
		perror("Last system error was ");
		return;
	}

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
	tlog3<<errors;
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
	if (minimize && node == schema["default"])
	{
		node.setType(JsonNode::DATA_NULL);
		return false;
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
	if (!validateSchema(type, schema)
	 || !validateType(node, schema, type))
	{
		node.setType(JsonNode::DATA_NULL);
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

	bool result = true;
	BOOST_FOREACH(JsonNode &entry, node.Vector())
	{
		if (!validateType(entry, schema, type))
		{
			result = false;
			entry.setType(JsonNode::DATA_NULL);
		}
	}
	return result;
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
		if (nodeIter->first < schemaIter->first) //No schema for entry
		{
			validateNode(nodeIter->second, nullNode, nodeIter->first);

			JsonMap::iterator toRemove = nodeIter++;
			node.Struct().erase(toRemove);
		}
		else
		if (schemaIter->first < nodeIter->first) //No entry
		{
			if (!validateNode(node[schemaIter->first], schemaIter->second, schemaIter->first))
				node.Struct().erase(schemaIter->first);
			schemaIter++;
		}
		else //both entry and schema are present
		{
			JsonMap::iterator current = nodeIter++;
			if (!validateNode(current->second, schemaIter->second, current->first))
				node.Struct().erase(current);

			schemaIter++;
		}
	}
	while (nodeIter != node.Struct().end())
	{
		validateNode(nodeIter->second, nullNode, nodeIter->first);
		JsonMap::iterator toRemove = nodeIter++;
		node.Struct().erase(toRemove);
	}

	while (schemaIter != schema.Struct().end())
	{
		if (!validateNode(node[schemaIter->first], schemaIter->second, schemaIter->first))
			node.Struct().erase(schemaIter->first);
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

JsonValidator::JsonValidator(JsonNode &root, bool Minimize):
	minimize(Minimize)
{
	if (root.getType() != JsonNode::DATA_STRUCT)
		return;

	JsonNode schema;
	schema.swap(root["schema"]);
	root.Struct().erase("schema");

	if (!schema.isNull())
	{
		validateProperties(root, schema);
	}
	//This message is quite annoying now - most files do not have schemas. May be re-enabled later
	//else
	//	addMessage("Schema not found!", true);

	//TODO: better way to show errors (like printing file name as well)
	tlog3<<errors;
}

JsonValidator::JsonValidator(JsonNode &root, const JsonNode &schema, bool Minimize):
	minimize(Minimize)
{
	validateProperties(root, schema);
	if (schema.isNull())
		addMessage("Schema not found!");
	tlog3<<errors;
}

Bonus * JsonUtils::parseBonus (const JsonVector &ability_vec) //TODO: merge with AddAbility, create universal parser for all bonus properties
{
	Bonus * b = new Bonus();
	std::string type = ability_vec[0].String();
	auto it = bonusNameMap.find(type);
	if (it == bonusNameMap.end())
	{
		tlog1 << "Error: invalid ability type " << type << " in creatures.txt" << std::endl;
		return b;
	}
	b->type = it->second;
	b->val = ability_vec[1].Float();
	b->subtype = ability_vec[2].Float();
	b->additionalInfo = ability_vec[3].Float();
	b->duration = Bonus::PERMANENT; //TODO: handle flags (as integer)
	b->turnsRemain = 0;
	return b;
}
template <typename T>
const T & parseByMap(const std::map<std::string, T> & map, const JsonNode * val, std::string err)
{
	static T defaultValue;
	if (!val->isNull())
	{
		std::string type = val->String();
		auto it = map.find(type);
		if (it == map.end())
		{
			tlog1 << "Error: invalid " << err << type << std::endl;
			return defaultValue;
		}
		else
		{
			return it->second;
		}
	}
	else
		return defaultValue;
};

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
				tlog2 << "Error! Wrong indentifier used for value of " << name;
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
			tlog2 << "Error! Wrong indentifier used for identifier!";
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
		tlog1 << "Error: invalid ability type " << type << std::endl;
		return b;
	}
	b->type = it->second;

	resolveIdentifier (b->subtype, ability, "subtype");

	value = &ability["val"];
	if (!value->isNull())
		b->val = value->Float();

	value = &ability["valueType"];
	if (!value->isNull())
		b->valType = parseByMap(bonusValueMap, value, "value type ");

	resolveIdentifier (b->additionalInfo, ability, "addInfo");

	value = &ability["turns"];
	if (!value->isNull())
		b->turnsRemain = value->Float();

	value = &ability["sourceID"];
	if (!value->isNull())
		b->sid = value->Float();

	value = &ability["description"];
	if (!value->isNull())
		b->description = value->String();

	value = &ability["effectRange"];
	if (!value->isNull())
		b->effectRange = parseByMap(bonusLimitEffect, value, "effect range ");

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
			tlog2 << "Error! Wrong bonus duration format.";
		}
	}

	value = &ability["source"];
	if (!value->isNull())
		b->source = parseByMap(bonusSourceMap, value, "source type ");

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
								l2->setCreature (creature);
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
								tlog1 << "Error: invalid ability type " << anotherBonusType << std::endl;
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
	node["type"].String() = reverseMapFirst<std::string, int>(bonus->type, bonusNameMap);
	node["subtype"].Float() = bonus->subtype;
	node["val"].Float() = bonus->val;
	node["valueType"].String() = reverseMapFirst<std::string, int>(bonus->valType, bonusValueMap);
	node["additionalInfo"].Float() = bonus->additionalInfo;
	node["turns"].Float() = bonus->turnsRemain;
	node["sourceID"].Float() = bonus->source;
	node["description"].String() = bonus->description;
	node["effectRange"].String() = reverseMapFirst<std::string, int>(bonus->effectRange, bonusLimitEffect);
	node["duration"].String() = reverseMapFirst<std::string, int>(bonus->duration, bonusDurationMap);
	node["source"].String() = reverseMapFirst<std::string, int>(bonus->source, bonusSourceMap);
	if(bonus->limiter)
	{
		node["limiter"].String() = reverseMapFirst<std::string, TLimiterPtr>(bonus->limiter, bonusLimiterMap);
	}
	if(bonus->propagator)
	{
		node["propagator"].String() = reverseMapFirst<std::string, TPropagatorPtr>(bonus->propagator, bonusPropagatorMap);
	}
}

void JsonUtils::minimize(JsonNode & node, const JsonNode& schema)
{
	JsonValidator validator(node, schema, true);
}

void JsonUtils::validate(JsonNode & node, const JsonNode& schema)
{
	JsonValidator validator(node, schema, false);
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
		break; case JsonNode::DATA_NULL:   dest.setType(JsonNode::DATA_NULL);
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
