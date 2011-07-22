#define VCMI_DLL
#include "JsonNode.h"

#include <assert.h>
#include <fstream>
#include <sstream>

const JsonNode JsonNode::nullNode;

JsonNode::JsonNode(JsonType Type):
	type(DATA_NULL)
{
	setType(Type);
}

JsonNode::JsonNode(std::string input):
	type(DATA_NULL)
{
	JsonParser parser(input, *this);
}

JsonNode::JsonNode(const JsonNode &copy):
	type(DATA_NULL)
{
	*this = copy;
}

JsonNode::~JsonNode()
{
	setType(DATA_NULL);
}

JsonNode & JsonNode::operator =(const JsonNode &node)
{
	setType(node.getType());
	switch(type)
	{
		break; case DATA_NULL:
		break; case DATA_BOOL:   Bool() =   node.Bool();
		break; case DATA_FLOAT:  Float() =  node.Float();
		break; case DATA_STRING: String() = node.String();
		break; case DATA_VECTOR: Vector() = node.Vector();
		break; case DATA_STRUCT: Struct() = node.Struct();
	}
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

	if (Type != DATA_NULL)
		setType(DATA_NULL);

	switch (type)
	{
		break; case DATA_STRING:  delete data.String;
		break; case DATA_VECTOR : delete data.Vector;
		break; case DATA_STRUCT:  delete data.Struct;
		break; default:
		break;
	}
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


const bool & JsonNode::Bool() const
{
	assert(type == DATA_BOOL);
	return data.Bool;
}

const float & JsonNode::Float() const
{
	assert(type == DATA_FLOAT);
	return data.Float;
}

const std::string & JsonNode::String() const
{
	assert(type == DATA_STRING);
	return *data.String;
}

const JsonVector & JsonNode::Vector() const
{
	assert(type == DATA_VECTOR);
	return *data.Vector;
}

const JsonMap & JsonNode::Struct() const
{
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

//Helper to write content of map/vector
template<class iterator>
void writeContainer(const iterator &begin, const iterator &end, std::ostream &out, std::string prefix)
{
	if (begin == end)
		return;

	iterator last = end;
	last--;

	for (iterator it=begin; it != last; ++it)
	{
		writeNode(it, out, prefix);
		out<<",\n";
	}
	writeNode(last, out, prefix);
	out<<"\n";
}

void writeNode(JsonVector::const_iterator it, std::ostream &out, std::string prefix)
{
	out << prefix;
	it->write(out, prefix);
}

void writeNode(JsonMap::const_iterator it, std::ostream &out, std::string prefix)
{
	out << prefix << '\"' << it->first << '\"' << " : ";
	it->second.write(out, prefix);
}

void JsonNode::write(std::ostream &out, std::string prefix) const
{
	switch(type)
	{
		break; case DATA_NULL:
			out << "null";

		break; case DATA_BOOL:
			if (Bool())
				out << "true";
			else
				out << "false";

		break; case DATA_FLOAT:
			out << Float();

		break; case DATA_STRING:
			out << "\"" << String() << "\"";

		break; case DATA_VECTOR:
			out << "[" << "\n";
			writeContainer(Vector().begin(), Vector().end(), out, prefix+'\t');
			out << prefix << "]";

		break; case DATA_STRUCT:
			out << "{" << "\n";
			writeContainer(Struct().begin(), Struct().end(), out, prefix+'\t');
			out << prefix << "}";
	}
}

std::ostream & operator<<(std::ostream &out, const JsonNode &node)
{
	node.write(out);
	return out << "\n";
}

////////////////////////////////////////////////////////////////////////////////

JsonParser::JsonParser(const std::string inputString, JsonNode &root):
	input(inputString),
	lineCount(1),
	lineStart(0),
	pos(0)
{
	extractValue(root);

	//Warn if there are any non-whitespace symbols left
	if (input.find_first_not_of(" \r\t\n", pos) != std::string::npos)
		error("Not all file was parsed!", true);

	//TODO: better way to show errors
	tlog2<<errors;
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

bool JsonParser::extractWhitespace()
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
			error("Comments should have two slashes!", true);

		pos = input.find('\n', pos);
	}

	if (pos >= input.size())
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
		break; default: return error("Uknown escape sequence!", true);
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
			str += input.substr(first, pos-first);
			pos++;
			return true;
		}
		if (input[pos] == '\\') // Escaping
		{
			str += input.substr(first, pos-first);
			first = pos++;
			if (pos == input.size())
				break;
			extractEscaping(str);
		}
		if (input[pos] == '\n') // end-of-line
		{
			str += input.substr(first, pos-first);
			return error("Closing quote not found!", true);
		}
		if (input[pos] < ' ') // control character
		{
			str += input.substr(first, pos-first);
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
	if (input.compare(pos, literal.size(), literal) != 0)
	{
		pos = input.find_first_of(" \n\r\t", pos);
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

	node.setType(JsonNode::DATA_BOOL);
	node.Bool() = true;
	return true;
}

bool JsonParser::extractFalse(JsonNode &node)
{
	if (!extractLiteral("false"))
		return false;

	node.setType(JsonNode::DATA_BOOL);
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

		JsonNode &child = node.Struct()[key];

		if (!extractSeparator())
			return false;

		if (!extractValue(child))
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

		if (input[pos] == '}')
		{
			pos++;
			return true;
		}

		if (!comma)
			error("Comma expected!", true);
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
		node.Vector().resize(node.Vector().size()+1);

		if (!extractValue(node.Vector().back()))
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

		if (input[pos] == ']')
		{
			pos++;
			return true;
		}

		if (!comma)
			error("Comma expected!", true);
	}
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
