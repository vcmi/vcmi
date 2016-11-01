/*
 * JsonDeserializer.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include  "JsonSerializeFormat.h"

class JsonNode;

class JsonDeserializer: public JsonSerializeFormat
{
public:
	JsonDeserializer(JsonNode & root_);

	void serializeBool(const std::string & fieldName, bool & value) override;
	void serializeEnum(const std::string & fieldName, const std::string & trueValue, const std::string & falseValue, bool & value) override;
	void serializeLIC(const std::string & fieldName, const TDecoder & decoder, const TEncoder & encoder, const std::vector<bool> & standard, std::vector<bool> & value) override;
	void serializeLIC(const std::string & fieldName, LIC & value) override;
	void serializeString(const std::string & fieldName, std::string & value) override;

protected:
	void serializeFloat(const std::string & fieldName, double & value) override;
	void serializeIntEnum(const std::string & fieldName, const std::vector<std::string> & enumMap, const si32 defaultValue, si32 & value) override;
	void serializeIntId(const std::string & fieldName, const TDecoder & decoder, const TEncoder & encoder, const si32 defaultValue, si32 & value) override;
private:
	void readLICPart(const JsonNode & part, const TDecoder & decoder, const bool val, std::vector<bool> & value);
};
