/*
 * JsonValidator.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "JsonValidator.h"

#include "JsonUtils.h"

#include "../VCMI_Lib.h"
#include "../filesystem/Filesystem.h"
#include "../modding/ModScope.h"
#include "../modding/CModHandler.h"
#include "../ScopeGuard.h"

VCMI_LIB_NAMESPACE_BEGIN

static const std::unordered_map<std::string, JsonNode::JsonType> stringToType =
{
	{"null",   JsonNode::JsonType::DATA_NULL},
	{"boolean", JsonNode::JsonType::DATA_BOOL},
	{"number", JsonNode::JsonType::DATA_FLOAT},
	{"integer", JsonNode::JsonType::DATA_INTEGER},
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

			// for "number" type both float and integer are allowed
			if(type == JsonNode::JsonType::DATA_FLOAT && data.isNumber())
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
			{
				const std::string name = validator.usedSchemas.back();
				const std::string nameClean = name.substr(0, name.find('#'));
				URI = nameClean + URI;
			}
			return check(URI, data, validator);
		}

		std::string formatCheck(Validation::ValidationData & validator, const JsonNode & baseSchema, const JsonNode & schema, const JsonNode & data)
		{
			auto formats = Validation::getKnownFormats();
			std::string errors;
			auto checker = formats.find(schema.String());
			if (checker != formats.end())
			{
				if (data.isString())
				{
					std::string result = checker->second(data);
					if (!result.empty())
						errors += validator.makeErrorMessage(result);
				}
				else
				{
					errors += validator.makeErrorMessage("Format value must be string: " + schema.String());
				}
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
			if (data.Float() > schema.Float())
				return validator.makeErrorMessage((boost::format("Value is bigger than %d") % schema.Float()).str());
			return "";
		}

		std::string minimumCheck(Validation::ValidationData & validator, const JsonNode & baseSchema, const JsonNode & schema, const JsonNode & data)
		{
			if (data.Float() < schema.Float())
				return validator.makeErrorMessage((boost::format("Value is smaller than %d") % schema.Float()).str());
			return "";
		}

		std::string exclusiveMaximumCheck(Validation::ValidationData & validator, const JsonNode & baseSchema, const JsonNode & schema, const JsonNode & data)
		{
			if (data.Float() >= schema.Float())
				return validator.makeErrorMessage((boost::format("Value is bigger than %d") % schema.Float()).str());
			return "";
		}

		std::string exclusiveMinimumCheck(Validation::ValidationData & validator, const JsonNode & baseSchema, const JsonNode & schema, const JsonNode & data)
		{
			if (data.Float() <= schema.Float())
				return validator.makeErrorMessage((boost::format("Value is smaller than %d") % schema.Float()).str());
			return "";
		}

		std::string multipleOfCheck(Validation::ValidationData & validator, const JsonNode & baseSchema, const JsonNode & schema, const JsonNode & data)
		{
			double result = data.Float() / schema.Float();
			if (!vstd::isAlmostEqual(floor(result), result))
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

		// Not implemented
		ret["propertyNames"] = Common::emptyCheck;
		ret["contains"] = Common::emptyCheck;
		ret["const"] = Common::emptyCheck;
		ret["examples"] = Common::emptyCheck;

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

		ret["exclusiveMaximum"] = Number::exclusiveMaximumCheck;
		ret["exclusiveMinimum"] = Number::exclusiveMinimumCheck;
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

		//TODO:
		// uri-reference
		// uri-template
		// json-pointer

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
