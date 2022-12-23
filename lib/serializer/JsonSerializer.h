/*
 * JsonSerializer.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include  "JsonTreeSerializer.h"

VCMI_LIB_NAMESPACE_BEGIN

class DLL_LINKAGE JsonSerializer : public JsonTreeSerializer<JsonNode *>
{
public:
	JsonSerializer(const IInstanceResolver * instanceResolver_, JsonNode & root_);

	void serializeLIC(const std::string & fieldName, const TDecoder & decoder, const TEncoder & encoder, const std::vector<bool> & standard, std::vector<bool> & value) override;
	void serializeLIC(const std::string & fieldName, LIC & value) override;
	void serializeLIC(const std::string & fieldName, LICSet & value) override;
	void serializeString(const std::string & fieldName, std::string & value) override;

	void serializeRaw(const std::string & fieldName, JsonNode & value, const boost::optional<const JsonNode &> defaultValue) override;

protected:
	void serializeInternal(const std::string & fieldName, boost::logic::tribool & value) override;
	void serializeInternal(const std::string & fieldName, si32 & value, const boost::optional<si32> & defaultValue, const TDecoder & decoder, const TEncoder & encoder)	override;
	void serializeInternal(const std::string & fieldName, std::vector<si32> & value, const TDecoder & decoder, const TEncoder & encoder) override;
	void serializeInternal(const std::string & fieldName, double & value, const boost::optional<double> & defaultValue) override;
	void serializeInternal(const std::string & fieldName, si64 & value, const boost::optional<si64> & defaultValue) override;
	void serializeInternal(const std::string & fieldName, si32 & value, const boost::optional<si32> & defaultValue, const std::vector<std::string> & enumMap) override;

	void serializeInternal(std::string & value) override;
	void serializeInternal(int64_t & value) override;

	void pushStruct(const std::string & fieldName) override;
	void pushArray(const std::string & fieldName) override;
	void pushArrayElement(const size_t index) override;
	void resizeCurrent(const size_t newSize, JsonNode::JsonType type) override;
private:
	void writeLICPart(const std::string & fieldName, const std::string & partName, const TEncoder & encoder, const std::vector<bool> & data);
	void writeLICPart(const std::string & fieldName, const std::string & partName, const TEncoder & encoder, const std::set<si32> & data);
	void writeLICPartBuffer(const std::string & fieldName, const std::string & partName, std::vector<std::string> & buffer);

};

VCMI_LIB_NAMESPACE_END
