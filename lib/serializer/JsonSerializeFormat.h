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

#include "../JsonNode.h"

class JsonSerializeFormat;
class JsonStructSerializer;
class JsonArraySerializer;

class IInstanceResolver
{
public:
	virtual ~IInstanceResolver(){};
	virtual si32 decode(const std::string & identifier) const = 0;
	virtual std::string encode(si32 identifier) const = 0;
};

class JsonSerializeHelper : public boost::noncopyable
{
public:
	JsonSerializeHelper(JsonSerializeHelper && other);
	virtual ~JsonSerializeHelper();

	JsonNode & get();

	JsonSerializeFormat * operator->();

	JsonStructSerializer enterStruct(const std::string & fieldName);
	JsonArraySerializer enterArray(const std::string & fieldName);

protected:
	JsonSerializeHelper(JsonSerializeFormat & owner_, JsonNode * thisNode_);
	JsonSerializeHelper(JsonSerializeHelper & parent, const std::string & fieldName);

	JsonSerializeFormat & owner;

	JsonNode * thisNode;
	JsonNode * parentNode;

	friend class JsonStructSerializer;

private:
	bool restoreState;
};

class JsonStructSerializer : public JsonSerializeHelper
{
public:
	bool optional;
	JsonStructSerializer(JsonStructSerializer && other);
	~JsonStructSerializer();

protected:
	JsonStructSerializer(JsonSerializeFormat & owner_, JsonNode * thisNode_);
	JsonStructSerializer(JsonSerializeFormat & owner_, const std::string & fieldName);
	JsonStructSerializer(JsonSerializeHelper & parent, const std::string & fieldName);

	friend class JsonSerializeFormat;
	friend class JsonSerializeHelper;
	friend class JsonArraySerializer;
};

class JsonArraySerializer : public JsonSerializeHelper
{
public:
	JsonArraySerializer(JsonStructSerializer && other);

	JsonStructSerializer enterStruct(const size_t index);

	template<typename Container>
	void syncSize(Container & c, JsonNode::JsonType type = JsonNode::DATA_NULL);

	///vector of serializable <-> Json vector of structs
	template<typename Element>
	void serializeStruct(std::vector<Element> & value)
	{
		syncSize(value, JsonNode::DATA_STRUCT);

		for(size_t idx = 0; idx < size(); idx++)
		{
			auto s = enterStruct(idx);
			value[idx].serializeJson(owner);
		}
	}

	void resize(const size_t newSize);
	void resize(const size_t newSize, JsonNode::JsonType type);
	size_t size() const;

protected:
	JsonArraySerializer(JsonSerializeFormat & owner_, const std::string & fieldName);
	JsonArraySerializer(JsonSerializeHelper & parent, const std::string & fieldName);

	friend class JsonSerializeFormat;
	friend class JsonSerializeHelper;
};

class JsonSerializeFormat : public boost::noncopyable
{
public:
	///user-provided callback to resolve string identifier
	///returns resolved identifier or -1 on error
	typedef std::function<si32(const std::string &)> TDecoder;

	///user-provided callback to get string identifier
	///may assume that object index is valid
	typedef std::function<std::string(si32)> TEncoder;

	typedef std::function<void (JsonSerializeFormat &)> TSerialize;

	struct LIC
	{
		LIC(const std::vector<bool> & Standard, const TDecoder Decoder, const TEncoder Encoder);

		const std::vector<bool> & standard;
		const TDecoder decoder;
		const TEncoder encoder;
		std::vector<bool> all, any, none;
	};

	struct LICSet
	{
		LICSet(const std::set<si32> & Standard, const TDecoder Decoder, const TEncoder Encoder);

		const std::set<si32> & standard;
		const TDecoder decoder;
		const TEncoder encoder;
		std::set<si32> all, any, none;
	};

	const bool saving;

	JsonSerializeFormat() = delete;
	virtual ~JsonSerializeFormat() = default;

	JsonNode & getRoot()
	{
		return *root;
	};

	JsonNode & getCurrent()
	{
		return *current;
	};

	JsonStructSerializer enterStruct(const std::string & fieldName);
	JsonArraySerializer enterArray(const std::string & fieldName);

	///Anything comparable <-> Json bool
	template<typename T>
	void serializeBool(const std::string & fieldName, T & value, const T trueValue, const T falseValue, const T defaultValue)
	{
		boost::logic::tribool temp(boost::logic::indeterminate);

		if(value == defaultValue)
			; //leave as indeterminate
		else if(value == trueValue)
			temp = true;
		else if(value == falseValue)
			temp = false;

		serializeInternal(fieldName, temp);
		if(!saving)
		{
			if(boost::logic::indeterminate(temp))
				value = defaultValue;
			else
				value = temp ? trueValue : falseValue;
		}
	}

	///bool <-> Json bool
	void serializeBool(const std::string & fieldName, bool & value);

	///tribool <-> Json bool
	void serializeBool(const std::string & fieldName, boost::logic::tribool & value)
	{
		serializeInternal(fieldName, value);
	};

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

	/** @brief Complete serialization of Logical identifier condition. (Special version)
	 * Assumes that all values are allowed by default, and standard contains them
	 */
	virtual void serializeLIC(const std::string & fieldName, LICSet & value) = 0;

	///String <-> Json string
	virtual void serializeString(const std::string & fieldName, std::string & value) = 0;

	///si32-convertible enum <-> Json string enum
	template<typename T>
	void serializeEnum(const std::string & fieldName, T & value, const std::vector<std::string> & enumMap)
	{
		doSerializeInternal<T, T, si32>(fieldName, value, boost::none, enumMap);
	};

	///si32-convertible enum <-> Json string enum
	template<typename T, typename U>
	void serializeEnum(const std::string & fieldName, T & value, const U & defaultValue, const std::vector<std::string> & enumMap)
	{
		doSerializeInternal<T, U, si32>(fieldName, value, defaultValue, enumMap);
	};

	template<typename T, typename U, typename C>
	void serializeEnum(const std::string & fieldName, T & value, const U & defaultValue, const C & enumMap)
	{
		std::vector<std::string> enumMapCopy;

		std::copy(std::begin(enumMap), std::end(enumMap), std::back_inserter(enumMapCopy));

		doSerializeInternal<T, U, si32>(fieldName, value, defaultValue, enumMapCopy);
	};

	///Anything double-convertible <-> Json double
	template<typename T>
	void serializeFloat(const std::string & fieldName, T & value)
	{
		doSerializeInternal<T, T, double>(fieldName, value, boost::none);
	};

	///Anything double-convertible <-> Json double
	template<typename T, typename U>
	void serializeFloat(const std::string & fieldName, T & value, const U & defaultValue)
	{
		doSerializeInternal<T, U, double>(fieldName, value, defaultValue);
	};

	///Anything int64-convertible <-> Json integer
	template<typename T>
	void serializeInt(const std::string & fieldName, T & value)
	{
		doSerializeInternal<T, T, si64>(fieldName, value, boost::none);
	};

	///Anything int64-convertible <-> Json integer
	template<typename T, typename U>
	void serializeInt(const std::string & fieldName, T & value, const U & defaultValue)
	{
		doSerializeInternal<T, U, si64>(fieldName, value, defaultValue);
	};

	///si32-convertible identifier <-> Json string
	template<typename T, typename U>
	void serializeId(const std::string & fieldName, T & value, const U & defaultValue, const TDecoder & decoder, const TEncoder & encoder)
	{
		doSerializeInternal<T, U, si32>(fieldName, value, defaultValue, decoder, encoder);
	}

	///si32-convertible identifier vector <-> Json array of string
	template<typename T>
	void serializeIdArray(const std::string & fieldName, std::vector<T> & value, const TDecoder & decoder, const TEncoder & encoder)
	{
		std::vector<si32> temp;

		if(saving)
		{
			temp.reserve(value.size());

			for(const T & vitem : value)
			{
				si32 item = static_cast<si32>(vitem);
				temp.push_back(item);
			}
		}

		serializeInternal(fieldName, temp, decoder, encoder);
		if(!saving)
		{
			value.clear();
			value.reserve(temp.size());

			for(const si32 item : temp)
			{
				T vitem = static_cast<T>(item);
				value.push_back(vitem);
			}
		}
	}

	///si32-convertible identifier set <-> Json array of string
	template<typename T>
	void serializeIdArray(const std::string & fieldName, std::set<T> & value, const TDecoder & decoder, const TEncoder & encoder)
	{
		std::vector<si32> temp;

		if(saving)
		{
			temp.reserve(value.size());

			for(const T & vitem : value)
			{
				si32 item = static_cast<si32>(vitem);
				temp.push_back(item);
			}
		}

		serializeInternal(fieldName, temp, decoder, encoder);
		if(!saving)
		{
			value.clear();

			for(const si32 item : temp)
			{
				T vitem = static_cast<T>(item);
				value.insert(vitem);
			}
		}
	}

	///bitmask <-> Json array of string
	template<typename T, int Size>
	void serializeIdArray(const std::string & fieldName, T & value, const T & defaultValue, const TDecoder & decoder, const TEncoder & encoder)
	{
		static_assert(8 * sizeof(T) >= Size, "Mask size too small");

		std::vector<si32> temp;
		temp.reserve(Size);

		if(saving && value != defaultValue)
		{
			for(si32 i = 0; i < Size; i++)
				if(value & (1 << i))
					temp.push_back(i);
			serializeInternal(fieldName, temp, decoder, encoder);
		}

		if(!saving)
		{
			serializeInternal(fieldName, temp, decoder, encoder);

			if(temp.empty())
				value = defaultValue;
			else
			{
				value = 0;
				for(auto i : temp)
					value |= (1 << i);
			}
		}
	}

	///si32-convertible instance identifier <-> Json string
	template<typename T>
	void serializeInstance(const std::string & fieldName, T & value, const T & defaultValue)
	{
		const TDecoder decoder = std::bind(&IInstanceResolver::decode, instanceResolver, _1);
		const TEncoder endoder = std::bind(&IInstanceResolver::encode, instanceResolver, _1);

		serializeId<T>(fieldName, value, defaultValue, decoder, endoder);
	}

protected:
	JsonNode * root;
	JsonNode * current;

	JsonSerializeFormat(const IInstanceResolver * instanceResolver_, JsonNode & root_, const bool saving_);

	///bool <-> Json bool, indeterminate is default
	virtual void serializeInternal(const std::string & fieldName, boost::logic::tribool & value) = 0;

	///Numeric Id <-> String Id
	virtual void serializeInternal(const std::string & fieldName, si32 & value, const boost::optional<si32> & defaultValue, const TDecoder & decoder, const TEncoder & encoder) = 0;

	///Numeric Id vector <-> String Id vector
	virtual void serializeInternal(const std::string & fieldName, std::vector<si32> & value, const TDecoder & decoder, const TEncoder & encoder) = 0;

	///Numeric <-> Json double
	virtual void serializeInternal(const std::string & fieldName, double & value, const boost::optional<double> & defaultValue) = 0;

	///Numeric <-> Json integer
	virtual void serializeInternal(const std::string & fieldName, si64 & value, const boost::optional<si64> & defaultValue) = 0;

	///Enum/Numeric <-> Json string enum
	virtual void serializeInternal(const std::string & fieldName, si32 & value, const boost::optional<si32> & defaultValue, const std::vector<std::string> & enumMap) = 0;

private:
	const IInstanceResolver * instanceResolver;

	template<typename VType, typename DVType, typename IType, typename ... Args>
	void doSerializeInternal(const std::string & fieldName, VType & value, const boost::optional<DVType> & defaultValue, Args ... args)
	{
		const boost::optional<IType> tempDefault = defaultValue ? boost::optional<IType>(static_cast<IType>(defaultValue.get())) : boost::none;
		IType temp = static_cast<IType>(value);

		serializeInternal(fieldName, temp, tempDefault, args ...);

		if(!saving)
			value = static_cast<VType>(temp);
	}

	friend class JsonSerializeHelper;
	friend class JsonStructSerializer;
	friend class JsonArraySerializer;
};

template<typename Container>
void JsonArraySerializer::syncSize(Container & c, JsonNode::JsonType type)
{
	if(owner.saving)
		resize(c.size(), type);
	else
		c.resize(size());
}
