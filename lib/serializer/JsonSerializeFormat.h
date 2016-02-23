/*
 * JsonSerializeFormat.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

class JsonNode;

class JsonSerializeFormat;

class JsonStructSerializer: public boost::noncopyable
{
public:
	JsonStructSerializer(JsonStructSerializer && other);
	virtual ~JsonStructSerializer();

	JsonStructSerializer enterStruct(const std::string & fieldName);

	JsonNode & get();

	JsonSerializeFormat * operator->();
private:
	JsonStructSerializer(JsonSerializeFormat & owner_, const std::string & fieldName);
	JsonStructSerializer(JsonStructSerializer & parent, const std::string & fieldName);

	bool restoreState;
	JsonSerializeFormat & owner;
	JsonNode * parentNode;
	JsonNode * thisNode;
	friend class JsonSerializeFormat;
};

class JsonSerializeFormat: public boost::noncopyable
{
public:
	///user-provided callback to resolve string identifier
	///returns resolved identifier or -1 on error
	typedef std::function<si32(const std::string &)> TDecoder;

	///user-provided callback to get string identifier
	///may assume that object index is valid
	typedef std::function<std::string(si32)> TEncoder;

	struct LIC
	{
		LIC(const std::vector<bool> & Standard, const TDecoder & Decoder, const TEncoder & Encoder);

		const std::vector<bool> & standard;
		const TDecoder & decoder;
		const TEncoder & encoder;
		std::vector<bool> all, any, none;
	};

	const bool saving;

	JsonSerializeFormat() = delete;
	virtual ~JsonSerializeFormat() = default;

	JsonNode & getRoot()
	{
		return * root;
	};

	JsonNode & getCurrent()
	{
		return * current;
	};

	JsonStructSerializer enterStruct(const std::string & fieldName);

	template <typename T>
	void serializeBool(const std::string & fieldName, const T trueValue, const T falseValue, T & value)
	{
		bool temp = (value == trueValue);
		serializeBool(fieldName, temp);
		if(!saving)
			value = temp ? trueValue : falseValue;
	}

	virtual void serializeBool(const std::string & fieldName, bool & value) = 0;

	virtual void serializeEnum(const std::string & fieldName, const std::string & trueValue, const std::string & falseValue, bool & value) = 0;

	/** @brief Restrictive ("anyOf") simple serialization of Logical identifier condition, simple deserialization (allOf=anyOf)
	 *
	 * @param fieldName
	 * @param decoder resolve callback, should report errors itself and do not throw
	 * @param encoder encode callback, should report errors itself and do not throw
	 * @param value target value, must be resized properly
	 *
	 */
	virtual void serializeLIC(const std::string & fieldName, const TDecoder & decoder, const TEncoder & encoder, const std::vector<bool> & standard, std::vector<bool> & value) = 0;

	/** @brief Complete serialization of Logical identifier condition
	 */
	virtual void serializeLIC(const std::string & fieldName, LIC & value) = 0;

	template <typename T>
	void serializeNumericEnum(const std::string & fieldName, const std::vector<std::string> & enumMap, const T defaultValue, T & value)
	{
		si32 temp = value;
		serializeIntEnum(fieldName, enumMap, defaultValue, temp);
		if(!saving)
			value = temp;
	};

	template <typename T>
	void serializeNumeric(const std::string & fieldName, T & value)
	{
		double temp = value;
		serializeFloat(fieldName, temp);
		if(!saving)
			value = temp;
	};

	virtual void serializeString(const std::string & fieldName, std::string & value) = 0;

	template <typename T>
	void serializeId(const std::string & fieldName, const TDecoder & decoder, const TEncoder & encoder, const T & defaultValue, T & value)
	{
		const si32 tempDefault = defaultValue.num;
		si32 tempValue = value.num;
		serializeIntId(fieldName, decoder, encoder, tempDefault, tempValue);
		if(!saving)
			value = T(tempValue);
	}

protected:
	JsonNode * root;
	JsonNode * current;

	JsonSerializeFormat(JsonNode & root_, const bool saving_);

	virtual void serializeFloat(const std::string & fieldName, double & value) = 0;

	virtual void serializeIntEnum(const std::string & fieldName, const std::vector<std::string> & enumMap, const si32 defaultValue, si32 & value) = 0;

	virtual void serializeIntId(const std::string & fieldName, const TDecoder & decoder, const TEncoder & encoder, const si32 defaultValue, si32 & value) = 0;
private:
	friend class JsonStructSerializer;
};

