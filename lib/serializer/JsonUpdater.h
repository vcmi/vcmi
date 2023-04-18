/*
 * JsonUpdater.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "ILICReader.h"
#include "JsonTreeSerializer.h"

VCMI_LIB_NAMESPACE_BEGIN

class CBonusSystemNode;

class DLL_LINKAGE JsonUpdater: public JsonTreeSerializer<const JsonNode *>, public ILICReader
{
public:
	JsonUpdater(const IInstanceResolver * instanceResolver_, const JsonNode & root_);

	void serializeLIC(const std::string & fieldName, const TDecoder & decoder, const TEncoder & encoder, const std::vector<bool> & standard, std::vector<bool> & value) override;
	void serializeLIC(const std::string & fieldName, LIC & value) override;
	void serializeLIC(const std::string & fieldName, LICSet & value) override;
	void serializeString(const std::string & fieldName, std::string & value) override;

	void serializeRaw(const std::string & fieldName, JsonNode & value, const std::optional<std::reference_wrapper<const JsonNode>> defaultValue) override;

	void serializeBonuses(const std::string & fieldName, CBonusSystemNode * value);

protected:
	void serializeInternal(const std::string & fieldName, boost::logic::tribool & value) override;
	void serializeInternal(const std::string & fieldName, si32 & value, const std::optional<si32> & defaultValue, const TDecoder & decoder, const TEncoder & encoder)	override;
	void serializeInternal(const std::string & fieldName, std::vector<si32> & value, const TDecoder & decoder, const TEncoder & encoder) override;
	void serializeInternal(const std::string & fieldName, double & value, const std::optional<double> & defaultValue) override;
	void serializeInternal(const std::string & fieldName, si64 & value, const std::optional<si64> & defaultValue) override;
	void serializeInternal(const std::string & fieldName, si32 & value, const std::optional<si32> & defaultValue, const std::vector<std::string> & enumMap) override;

	void serializeInternal(std::string & value) override;
	void serializeInternal(int64_t & value) override;
};

VCMI_LIB_NAMESPACE_END
