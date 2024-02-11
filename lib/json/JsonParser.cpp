/*
 * JsonParser.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "JsonParser.h"

#include "../TextOperations.h"

VCMI_LIB_NAMESPACE_BEGIN

JsonParser::JsonParser(const char * inputString, size_t stringSize):
	input(inputString, stringSize),
	lineCount(1),
	lineStart(0),
	pos(0)
{
}

JsonNode JsonParser::parse(const std::string & fileName)
{
	JsonNode root;

	if (input.size() == 0)
	{
		error("File is empty", false);
	}
	else
	{
		if (!TextOperations::isValidUnicodeString(&input[0], input.size()))
			error("Not a valid UTF-8 file", false);

		extractValue(root);
		extractWhitespace(false);

		//Warn if there are any non-whitespace symbols left
		if (pos < input.size())
			error("Not all file was parsed!", true);
	}

	if (!errors.empty())
	{
		logMod->warn("File %s is not a valid JSON file!", fileName);
		logMod->warn(errors);
	}
	return root;
}

bool JsonParser::isValid()
{
	return errors.empty();
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
		while(pos < input.size() && static_cast<ui8>(input[pos]) <= ' ')
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
			error("Comments must consist of two slashes!", true);

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
		break; case 'b': str += '\b';
		break; case 'f': str += '\f';
		break; case 'n': str += '\n';
		break; case 'r': str += '\r';
		break; case 't': str += '\t';
		break; case '/': str += '/';
		break; default: return error("Unknown escape sequence!", true);
	}
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
		if(static_cast<unsigned char>(input[pos]) < ' ') // control character
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

	node.setType(JsonNode::JsonType::DATA_STRING);
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
	node.setType(JsonNode::JsonType::DATA_STRUCT);
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

		// split key string into actual key and meta-flags
		std::vector<std::string> keyAndFlags;
		boost::split(keyAndFlags, key, boost::is_any_of("#"));
		key = keyAndFlags[0];
		// check for unknown flags - helps with debugging
		std::vector<std::string> knownFlags = { "override" };
		for(int i = 1; i < keyAndFlags.size(); i++)
		{
			if(!vstd::contains(knownFlags, keyAndFlags[i]))
				error("Encountered unknown flag #" + keyAndFlags[i], true);
		}

		if (node.Struct().find(key) != node.Struct().end())
			error("Duplicate element encountered!", true);

		if (!extractSeparator())
			return false;

		if (!extractElement(node.Struct()[key], '}'))
			return false;

		// flags from key string belong to referenced element
		for(int i = 1; i < keyAndFlags.size(); i++)
			node.Struct()[key].flags.push_back(keyAndFlags[i]);

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
	node.setType(JsonNode::JsonType::DATA_VECTOR);

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
	{
		//FIXME: MOD COMPATIBILITY: Too many of these right now, re-enable later
		//if (comma)
			//error("Extra comma found!", true);
		return true;
	}

	if (!comma)
		error("Comma expected!", true);

	return true;
}

bool JsonParser::extractFloat(JsonNode &node)
{
	assert(input[pos] == '-' || (input[pos] >= '0' && input[pos] <= '9'));
	bool negative=false;
	double result=0;
	si64 integerPart = 0;
	bool isFloat = false;

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
		integerPart = integerPart*10+(input[pos]-'0');
		pos++;
	}

	result = static_cast<double>(integerPart);

	if (input[pos] == '.')
	{
		//extract fractional part
		isFloat = true;
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

	if(input[pos] == 'e')
	{
		//extract exponential part
		pos++;
		isFloat = true;
		bool powerNegative = false;
		double power = 0;

		if(input[pos] == '-')
		{
			pos++;
			powerNegative = true;
		}
		else if(input[pos] == '+')
		{
			pos++;
		}

		if (input[pos] < '0' || input[pos] > '9')
			return error("Exponential part expected!");

		while (input[pos] >= '0' && input[pos] <= '9')
		{
			power = power*10 + (input[pos]-'0');
			pos++;
		}

		if(powerNegative)
			power = -power;

		result *= std::pow(10, power);
	}

	if(isFloat)
	{
		if(negative)
			result = -result;

		node.setType(JsonNode::JsonType::DATA_FLOAT);
		node.Float() = result;
	}
	else
	{
		if(negative)
			integerPart = -integerPart;

		node.setType(JsonNode::JsonType::DATA_INTEGER);
		node.Integer() = integerPart;
	}

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

VCMI_LIB_NAMESPACE_END
