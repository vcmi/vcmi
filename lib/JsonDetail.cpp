/*
 * JsonDetail.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "JsonDetail.h"

#include "VCMI_Lib.h"
#include "TextOperations.h"

#include "filesystem/Filesystem.h"
#include "modding/ModScope.h"
#include "modding/CModHandler.h"
#include "ScopeGuard.h"

VCMI_LIB_NAMESPACE_BEGIN

static const JsonNode nullNode;

template<typename Iterator>
void JsonWriter::writeContainer(Iterator begin, Iterator end)
{
	if (begin == end)
		return;

	prefix += '\t';

	writeEntry(begin++);

	while (begin != end)
	{
		out << (compactMode ? ", " : ",\n");
		writeEntry(begin++);
	}

	out << (compactMode ? "" : "\n");
	prefix.resize(prefix.size()-1);
}

void JsonWriter::writeEntry(JsonMap::const_iterator entry)
{
	if(!compactMode)
	{
		if (!entry->second.meta.empty())
			out << prefix << " // " << entry->second.meta << "\n";
		if(!entry->second.flags.empty())
			out << prefix << " // flags: " << boost::algorithm::join(entry->second.flags, ", ") << "\n";
		out << prefix;
	}
	writeString(entry->first);
	out << " : ";
	writeNode(entry->second);
}

void JsonWriter::writeEntry(JsonVector::const_iterator entry)
{
	if(!compactMode)
	{
		if (!entry->meta.empty())
			out << prefix << " // " << entry->meta << "\n";
		if(!entry->flags.empty())
			out << prefix << " // flags: " << boost::algorithm::join(entry->flags, ", ") << "\n";
		out << prefix;
	}
	writeNode(*entry);
}

void JsonWriter::writeString(const std::string &string)
{
	static const std::string escaped = "\"\\\b\f\n\r\t/";

	static const std::array<char, 8> escaped_code = {'\"', '\\', 'b', 'f', 'n', 'r', 't', '/'};

	out <<'\"';
	size_t pos = 0;
	size_t start = 0;
	for (; pos<string.size(); pos++)
	{
		//we need to check if special character was been already escaped
		if((string[pos] == '\\')
			&& (pos+1 < string.size())
			&& (std::find(escaped_code.begin(), escaped_code.end(), string[pos+1]) != escaped_code.end()) )
		{
			pos++; //write unchanged, next simbol also checked
		}
		else
		{
			size_t escapedPos = escaped.find(string[pos]);

			if (escapedPos != std::string::npos)
			{
				out.write(string.data()+start, pos - start);
				out << '\\' << escaped_code[escapedPos];
				start = pos+1;
			}
		}

	}
	out.write(string.data()+start, pos - start);
	out <<'\"';
}

void JsonWriter::writeNode(const JsonNode &node)
{
	bool originalMode = compactMode;
	if(compact && !compactMode && node.isCompact())
		compactMode = true;

	switch(node.getType())
	{
		break; case JsonNode::JsonType::DATA_NULL:
			out << "null";

		break; case JsonNode::JsonType::DATA_BOOL:
			if (node.Bool())
				out << "true";
			else
				out << "false";

		break; case JsonNode::JsonType::DATA_FLOAT:
			out << node.Float();

		break; case JsonNode::JsonType::DATA_STRING:
			writeString(node.String());

		break; case JsonNode::JsonType::DATA_VECTOR:
			out << "[" << (compactMode ? " " : "\n");
			writeContainer(node.Vector().begin(), node.Vector().end());
			out << (compactMode ? " " : prefix) << "]";

		break; case JsonNode::JsonType::DATA_STRUCT:
			out << "{" << (compactMode ? " " : "\n");
			writeContainer(node.Struct().begin(), node.Struct().end());
			out << (compactMode ? " " : prefix) << "}";

		break; case JsonNode::JsonType::DATA_INTEGER:
			out << node.Integer();
	}

	compactMode = originalMode;
}

JsonWriter::JsonWriter(std::ostream & output, bool compact)
	: out(output), compact(compact)
{
}

////////////////////////////////////////////////////////////////////////////////

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
			error("Dublicated element encountered!", true);

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

///////////////////////////////////////////////////////////////////////////////

//TODO: integer support

static const std::unordered_map<std::string, JsonNode::JsonType> stringToType =
{
	{"null",   JsonNode::JsonType::DATA_NULL},
	{"boolean", JsonNode::JsonType::DATA_BOOL},
	{"number", JsonNode::JsonType::DATA_FLOAT},
	{"string",  JsonNode::JsonType::DATA_STRING},
	{"array",  JsonNode::JsonType::DATA_VECTOR},
	{"object",  JsonNode::JsonType::DATA_STRUCT}
};

namespace
{
	namespace Common
	{
		std::string emptyCheck(Validation::ValidationData & validator, const JsonNode & baseSchema, const JsonNode & schema, const JsonNode & data)
		{
			// check is not needed - e.g. incorporated into another check
			return "";
		}

		std::string notImplementedCheck(Validation::ValidationData & validator,
										const JsonNode & baseSchema,
										const JsonNode & schema,
										const JsonNode & data)
		{
			return "Not implemented entry in schema";
		}

		std::string schemaListCheck(Validation::ValidationData & validator,
									const JsonNode & baseSchema,
									const JsonNode & schema,
									const JsonNode & data,
									const std::string & errorMsg,
									const std::function<bool(size_t)> & isValid)
		{
			std::string errors = "<tested schemas>\n";
			size_t result = 0;

			for(const auto & schemaEntry : schema.Vector())
			{
				std::string error = check(schemaEntry, data, validator);
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
				return "";
			else
				return validator.makeErrorMessage(errorMsg) + errors;
		}

		std::string allOfCheck(Validation::ValidationData & validator, const JsonNode & baseSchema, const JsonNode & schema, const JsonNode & data)
		{
			return schemaListCheck(validator, baseSchema, schema, data, "Failed to pass all schemas", [&](size_t count)
			{
				return count == schema.Vector().size();
			});
		}

		std::string anyOfCheck(Validation::ValidationData & validator, const JsonNode & baseSchema, const JsonNode & schema, const JsonNode & data)
		{
			return schemaListCheck(validator, baseSchema, schema, data, "Failed to pass any schema", [&](size_t count)
			{
				return count > 0;
			});
		}

		std::string oneOfCheck(Validation::ValidationData & validator, const JsonNode & baseSchema, const JsonNode & schema, const JsonNode & data)
		{
			return schemaListCheck(validator, baseSchema, schema, data, "Failed to pass exactly one schema", [&](size_t count)
			{
				return count == 1;
			});
		}

		std::string notCheck(Validation::ValidationData & validator, const JsonNode & baseSchema, const JsonNode & schema, const JsonNode & data)
		{
			if (check(schema, data, validator).empty())
				return validator.makeErrorMessage("Successful validation against negative check");
			return "";
		}

		std::string enumCheck(Validation::ValidationData & validator, const JsonNode & baseSchema, const JsonNode & schema, const JsonNode & data)
		{
			for(const auto & enumEntry : schema.Vector())
			{
				if (data == enumEntry)
					return "";
			}
			return validator.makeErrorMessage("Key must have one of predefined values");
		}

		std::string typeCheck(Validation::ValidationData & validator, const JsonNode & baseSchema, const JsonNode & schema, const JsonNode & data)
		{
			const auto & typeName = schema.String();
			auto it = stringToType.find(typeName);
			if(it == stringToType.end())
			{
				return validator.makeErrorMessage("Unknown type in schema:" + typeName);
			}

			JsonNode::JsonType type = it->second;

			//FIXME: hack for integer values
			if(data.isNumber() && type == JsonNode::JsonType::DATA_FLOAT)
				return "";

			if(type != data.getType() && data.getType() != JsonNode::JsonType::DATA_NULL)
				return validator.makeErrorMessage("Type mismatch! Expected " + schema.String());
			return "";
		}

		std::string refCheck(Validation::ValidationData & validator, const JsonNode & baseSchema, const JsonNode & schema, const JsonNode & data)
		{
			std::string URI = schema.String();
			//node must be validated using schema pointed by this reference and not by data here
			//Local reference. Turn it into more easy to handle remote ref
			if (boost::algorithm::starts_with(URI, "#"))
				URI = validator.usedSchemas.back() + URI;
			return check(URI, data, validator);
		}

		std::string formatCheck(Validation::ValidationData & validator, const JsonNode & baseSchema, const JsonNode & schema, const JsonNode & data)
		{
			auto formats = Validation::getKnownFormats();
			std::string errors;
			auto checker = formats.find(schema.String());
			if (checker != formats.end())
			{
				std::string result = checker->second(data);
				if (!result.empty())
					errors += validator.makeErrorMessage(result);
			}
			else
				errors += validator.makeErrorMessage("Unsupported format type: " + schema.String());
			return errors;
		}
	}

	namespace String
	{
		std::string maxLengthCheck(Validation::ValidationData & validator, const JsonNode & baseSchema, const JsonNode & schema, const JsonNode & data)
		{
			if (data.String().size() > schema.Float())
				return validator.makeErrorMessage((boost::format("String is longer than %d symbols") % schema.Float()).str());
			return "";
		}

		std::string minLengthCheck(Validation::ValidationData & validator, const JsonNode & baseSchema, const JsonNode & schema, const JsonNode & data)
		{
			if (data.String().size() < schema.Float())
				return validator.makeErrorMessage((boost::format("String is shorter than %d symbols") % schema.Float()).str());
			return "";
		}
	}

	namespace Number
	{

		std::string maximumCheck(Validation::ValidationData & validator, const JsonNode & baseSchema, const JsonNode & schema, const JsonNode & data)
		{
			if (baseSchema["exclusiveMaximum"].Bool())
			{
				if (data.Float() >= schema.Float())
					return validator.makeErrorMessage((boost::format("Value is bigger than %d") % schema.Float()).str());
			}
			else
			{
				if (data.Float() >  schema.Float())
					return validator.makeErrorMessage((boost::format("Value is bigger than %d") % schema.Float()).str());
			}
			return "";
		}

		std::string minimumCheck(Validation::ValidationData & validator, const JsonNode & baseSchema, const JsonNode & schema, const JsonNode & data)
		{
			if (baseSchema["exclusiveMinimum"].Bool())
			{
				if (data.Float() <= schema.Float())
					return validator.makeErrorMessage((boost::format("Value is smaller than %d") % schema.Float()).str());
			}
			else
			{
				if (data.Float() <  schema.Float())
					return validator.makeErrorMessage((boost::format("Value is smaller than %d") % schema.Float()).str());
			}
			return "";
		}

		std::string multipleOfCheck(Validation::ValidationData & validator, const JsonNode & baseSchema, const JsonNode & schema, const JsonNode & data)
		{
			double result = data.Float() / schema.Float();
			if (floor(result) != result)
				return validator.makeErrorMessage((boost::format("Value is not divisible by %d") % schema.Float()).str());
			return "";
		}
	}

	namespace Vector
	{
		std::string itemEntryCheck(Validation::ValidationData & validator, const JsonVector & items, const JsonNode & schema, size_t index)
		{
			validator.currentPath.emplace_back();
			validator.currentPath.back().Float() = static_cast<double>(index);
			auto onExit = vstd::makeScopeGuard([&]()
			{
				validator.currentPath.pop_back();
			});

			if (!schema.isNull())
				return check(schema, items[index], validator);
			return "";
		}

		std::string itemsCheck(Validation::ValidationData & validator, const JsonNode & baseSchema, const JsonNode & schema, const JsonNode & data)
		{
			std::string errors;
			for (size_t i=0; i<data.Vector().size(); i++)
			{
				if (schema.getType() == JsonNode::JsonType::DATA_VECTOR)
				{
					if (schema.Vector().size() > i)
						errors += itemEntryCheck(validator, data.Vector(), schema.Vector()[i], i);
				}
				else
				{
					errors += itemEntryCheck(validator, data.Vector(), schema, i);
				}
			}
			return errors;
		}

		std::string additionalItemsCheck(Validation::ValidationData & validator, const JsonNode & baseSchema, const JsonNode & schema, const JsonNode & data)
		{
			std::string errors;
			// "items" is struct or empty (defaults to empty struct) - validation always successful
			const JsonNode & items = baseSchema["items"];
			if (items.getType() != JsonNode::JsonType::DATA_VECTOR)
				return "";

			for (size_t i=items.Vector().size(); i<data.Vector().size(); i++)
			{
				if (schema.getType() == JsonNode::JsonType::DATA_STRUCT)
					errors += itemEntryCheck(validator, data.Vector(), schema, i);
				else if(!schema.isNull() && !schema.Bool())
					errors += validator.makeErrorMessage("Unknown entry found");
			}
			return errors;
		}

		std::string minItemsCheck(Validation::ValidationData & validator, const JsonNode & baseSchema, const JsonNode & schema, const JsonNode & data)
		{
			if (data.Vector().size() < schema.Float())
				return validator.makeErrorMessage((boost::format("Length is smaller than %d") % schema.Float()).str());
			return "";
		}

		std::string maxItemsCheck(Validation::ValidationData & validator, const JsonNode & baseSchema, const JsonNode & schema, const JsonNode & data)
		{
			if (data.Vector().size() > schema.Float())
				return validator.makeErrorMessage((boost::format("Length is bigger than %d") % schema.Float()).str());
			return "";
		}

		std::string uniqueItemsCheck(Validation::ValidationData & validator, const JsonNode & baseSchema, const JsonNode & schema, const JsonNode & data)
		{
			if (schema.Bool())
			{
				for (auto itA = schema.Vector().begin(); itA != schema.Vector().end(); itA++)
				{
					auto itB = itA;
					while (++itB != schema.Vector().end())
					{
						if (*itA == *itB)
							return validator.makeErrorMessage("List must consist from unique items");
					}
				}
			}
			return "";
		}
	}

	namespace Struct
	{
		std::string maxPropertiesCheck(Validation::ValidationData & validator, const JsonNode & baseSchema, const JsonNode & schema, const JsonNode & data)
		{
			if (data.Struct().size() > schema.Float())
				return validator.makeErrorMessage((boost::format("Number of entries is bigger than %d") % schema.Float()).str());
			return "";
		}

		std::string minPropertiesCheck(Validation::ValidationData & validator, const JsonNode & baseSchema, const JsonNode & schema, const JsonNode & data)
		{
			if (data.Struct().size() < schema.Float())
				return validator.makeErrorMessage((boost::format("Number of entries is less than %d") % schema.Float()).str());
			return "";
		}

		std::string uniquePropertiesCheck(Validation::ValidationData & validator, const JsonNode & baseSchema, const JsonNode & schema, const JsonNode & data)
		{
			for (auto itA = data.Struct().begin(); itA != data.Struct().end(); itA++)
			{
				auto itB = itA;
				while (++itB != data.Struct().end())
				{
					if (itA->second == itB->second)
						return validator.makeErrorMessage("List must consist from unique items");
				}
			}
			return "";
		}

		std::string requiredCheck(Validation::ValidationData & validator, const JsonNode & baseSchema, const JsonNode & schema, const JsonNode & data)
		{
			std::string errors;
			for(const auto & required : schema.Vector())
			{
				if (data[required.String()].isNull())
					errors += validator.makeErrorMessage("Required entry " + required.String() + " is missing");
			}
			return errors;
		}

		std::string dependenciesCheck(Validation::ValidationData & validator, const JsonNode & baseSchema, const JsonNode & schema, const JsonNode & data)
		{
			std::string errors;
			for(const auto & deps : schema.Struct())
			{
				if (!data[deps.first].isNull())
				{
					if (deps.second.getType() == JsonNode::JsonType::DATA_VECTOR)
					{
						JsonVector depList = deps.second.Vector();
						for(auto & depEntry : depList)
						{
							if (data[depEntry.String()].isNull())
								errors += validator.makeErrorMessage("Property " + depEntry.String() + " required for " + deps.first + " is missing");
						}
					}
					else
					{
						if (!check(deps.second, data, validator).empty())
							errors += validator.makeErrorMessage("Requirements for " + deps.first + " are not fulfilled");
					}
				}
			}
			return errors;
		}

		std::string propertyEntryCheck(Validation::ValidationData & validator, const JsonNode &node, const JsonNode & schema, const std::string & nodeName)
		{
			validator.currentPath.emplace_back();
			validator.currentPath.back().String() = nodeName;
			auto onExit = vstd::makeScopeGuard([&]()
			{
				validator.currentPath.pop_back();
			});

			// there is schema specifically for this item
			if (!schema.isNull())
				return check(schema, node, validator);
			return "";
		}

		std::string propertiesCheck(Validation::ValidationData & validator, const JsonNode & baseSchema, const JsonNode & schema, const JsonNode & data)
		{
			std::string errors;

			for(const auto & entry : data.Struct())
				errors += propertyEntryCheck(validator, entry.second, schema[entry.first], entry.first);
			return errors;
		}

		std::string additionalPropertiesCheck(Validation::ValidationData & validator, const JsonNode & baseSchema, const JsonNode & schema, const JsonNode & data)
		{
			std::string errors;
			for(const auto & entry : data.Struct())
			{
				if (baseSchema["properties"].Struct().count(entry.first) == 0)
				{
					// try generic additionalItems schema
					if (schema.getType() == JsonNode::JsonType::DATA_STRUCT)
						errors += propertyEntryCheck(validator, entry.second, schema, entry.first);

					// or, additionalItems field can be bool which indicates if such items are allowed
					else if(!schema.isNull() && !schema.Bool()) // present and set to false - error
						errors += validator.makeErrorMessage("Unknown entry found: " + entry.first);
				}
			}
			return errors;
		}
	}

	namespace Formats
	{
		bool testFilePresence(const std::string & scope, const ResourcePath & resource)
		{
			std::set<std::string> allowedScopes;
			if(scope != ModScope::scopeBuiltin() && !scope.empty()) // all real mods may have dependencies
			{
				//NOTE: recursive dependencies are not allowed at the moment - update code if this changes
				bool found = true;
				allowedScopes = VLC->modh->getModDependencies(scope, found);

				if(!found)
					return false;

				allowedScopes.insert(ModScope::scopeBuiltin()); // all mods can use H3 files
			}
			allowedScopes.insert(scope); // mods can use their own files

			for(const auto & entry : allowedScopes)
			{
				if (CResourceHandler::get(entry)->existsResource(resource))
					return true;
			}
			return false;
		}

		#define TEST_FILE(scope, prefix, file, type) \
			if (testFilePresence(scope, ResourcePath(prefix + file, type))) \
				return ""

		std::string testAnimation(const std::string & path, const std::string & scope)
		{
			TEST_FILE(scope, "Sprites/", path, EResType::ANIMATION);
			TEST_FILE(scope, "Sprites/", path, EResType::JSON);
			return "Animation file \"" + path + "\" was not found";
		}

		std::string textFile(const JsonNode & node)
		{
			TEST_FILE(node.meta, "", node.String(), EResType::JSON);
			return "Text file \"" + node.String() + "\" was not found";
		}

		std::string musicFile(const JsonNode & node)
		{
			TEST_FILE(node.meta, "Music/", node.String(), EResType::SOUND);
			TEST_FILE(node.meta, "", node.String(), EResType::SOUND);
			return "Music file \"" + node.String() + "\" was not found";
		}

		std::string soundFile(const JsonNode & node)
		{
			TEST_FILE(node.meta, "Sounds/", node.String(), EResType::SOUND);
			return "Sound file \"" + node.String() + "\" was not found";
		}

		std::string defFile(const JsonNode & node)
		{
			return testAnimation(node.String(), node.meta);
		}

		std::string animationFile(const JsonNode & node)
		{
			return testAnimation(node.String(), node.meta);
		}

		std::string imageFile(const JsonNode & node)
		{
			TEST_FILE(node.meta, "Data/", node.String(), EResType::IMAGE);
			TEST_FILE(node.meta, "Sprites/", node.String(), EResType::IMAGE);
			if (node.String().find(':') != std::string::npos)
				return testAnimation(node.String().substr(0, node.String().find(':')), node.meta);
			return "Image file \"" + node.String() + "\" was not found";
		}

		std::string videoFile(const JsonNode & node)
		{
			TEST_FILE(node.meta, "Video/", node.String(), EResType::VIDEO);
			return "Video file \"" + node.String() + "\" was not found";
		}

		#undef TEST_FILE
	}

	Validation::TValidatorMap createCommonFields()
	{
		Validation::TValidatorMap ret;

		ret["format"] =  Common::formatCheck;
		ret["allOf"] = Common::allOfCheck;
		ret["anyOf"] = Common::anyOfCheck;
		ret["oneOf"] = Common::oneOfCheck;
		ret["enum"]  = Common::enumCheck;
		ret["type"]  = Common::typeCheck;
		ret["not"]   = Common::notCheck;
		ret["$ref"]  = Common::refCheck;

		// fields that don't need implementation
		ret["title"] = Common::emptyCheck;
		ret["$schema"] = Common::emptyCheck;
		ret["default"] = Common::emptyCheck;
		ret["description"] = Common::emptyCheck;
		ret["definitions"] = Common::emptyCheck;
		return ret;
	}

	Validation::TValidatorMap createStringFields()
	{
		Validation::TValidatorMap ret = createCommonFields();
		ret["maxLength"] = String::maxLengthCheck;
		ret["minLength"] = String::minLengthCheck;

		ret["pattern"] = Common::notImplementedCheck;
		return ret;
	}

	Validation::TValidatorMap createNumberFields()
	{
		Validation::TValidatorMap ret = createCommonFields();
		ret["maximum"]    = Number::maximumCheck;
		ret["minimum"]    = Number::minimumCheck;
		ret["multipleOf"] = Number::multipleOfCheck;

		ret["exclusiveMaximum"] = Common::emptyCheck;
		ret["exclusiveMinimum"] = Common::emptyCheck;
		return ret;
	}

	Validation::TValidatorMap createVectorFields()
	{
		Validation::TValidatorMap ret = createCommonFields();
		ret["items"]           = Vector::itemsCheck;
		ret["minItems"]        = Vector::minItemsCheck;
		ret["maxItems"]        = Vector::maxItemsCheck;
		ret["uniqueItems"]     = Vector::uniqueItemsCheck;
		ret["additionalItems"] = Vector::additionalItemsCheck;
		return ret;
	}

	Validation::TValidatorMap createStructFields()
	{
		Validation::TValidatorMap ret = createCommonFields();
		ret["additionalProperties"]  = Struct::additionalPropertiesCheck;
		ret["uniqueProperties"]      = Struct::uniquePropertiesCheck;
		ret["maxProperties"]         = Struct::maxPropertiesCheck;
		ret["minProperties"]         = Struct::minPropertiesCheck;
		ret["dependencies"]          = Struct::dependenciesCheck;
		ret["properties"]            = Struct::propertiesCheck;
		ret["required"]              = Struct::requiredCheck;

		ret["patternProperties"] = Common::notImplementedCheck;
		return ret;
	}

	Validation::TFormatMap createFormatMap()
	{
		Validation::TFormatMap ret;
		ret["textFile"]      = Formats::textFile;
		ret["musicFile"]     = Formats::musicFile;
		ret["soundFile"]     = Formats::soundFile;
		ret["defFile"]       = Formats::defFile;
		ret["animationFile"] = Formats::animationFile;
		ret["imageFile"]     = Formats::imageFile;
		ret["videoFile"]     = Formats::videoFile;

		return ret;
	}
}

namespace Validation
{
	std::string ValidationData::makeErrorMessage(const std::string &message)
	{
		std::string errors;
		errors += "At ";
		if (!currentPath.empty())
		{
			for(const JsonNode &path : currentPath)
			{
				errors += "/";
				if (path.getType() == JsonNode::JsonType::DATA_STRING)
					errors += path.String();
				else
					errors += std::to_string(static_cast<unsigned>(path.Float()));
			}
		}
		else
			errors += "<root>";
		errors += "\n\t Error: " + message + "\n";
		return errors;
	}

	std::string check(const std::string & schemaName, const JsonNode & data)
	{
		ValidationData validator;
		return check(schemaName, data, validator);
	}

	std::string check(const std::string & schemaName, const JsonNode & data, ValidationData & validator)
	{
		validator.usedSchemas.push_back(schemaName);
		auto onscopeExit = vstd::makeScopeGuard([&]()
		{
			validator.usedSchemas.pop_back();
		});
		return check(JsonUtils::getSchema(schemaName), data, validator);
	}

	std::string check(const JsonNode & schema, const JsonNode & data, ValidationData & validator)
	{
		const TValidatorMap & knownFields = getKnownFieldsFor(data.getType());
		std::string errors;
		for(const auto & entry : schema.Struct())
		{
			auto checker = knownFields.find(entry.first);
			if (checker != knownFields.end())
				errors += checker->second(validator, schema, entry.second, data);
			//else
			//	errors += validator.makeErrorMessage("Unknown entry in schema " + entry.first);
		}
		return errors;
	}

	const TValidatorMap & getKnownFieldsFor(JsonNode::JsonType type)
	{
		static const TValidatorMap commonFields = createCommonFields();
		static const TValidatorMap numberFields = createNumberFields();
		static const TValidatorMap stringFields = createStringFields();
		static const TValidatorMap vectorFields = createVectorFields();
		static const TValidatorMap structFields = createStructFields();

		switch (type)
		{
			case JsonNode::JsonType::DATA_FLOAT:
			case JsonNode::JsonType::DATA_INTEGER:
				return numberFields;
			case JsonNode::JsonType::DATA_STRING: return stringFields;
			case JsonNode::JsonType::DATA_VECTOR: return vectorFields;
			case JsonNode::JsonType::DATA_STRUCT: return structFields;
			default: return commonFields;
		}
	}

	const TFormatMap & getKnownFormats()
	{
		static TFormatMap knownFormats = createFormatMap();
		return knownFormats;
	}

} // Validation namespace

VCMI_LIB_NAMESPACE_END
