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

#include "../constants/IdentifierBase.h"
#include "../json/JsonNode.h"
#include "../modding/IdentifierStorage.h"
#include "../modding/ModScope.h"
#include "../GameLibrary.h"

VCMI_LIB_NAMESPACE_BEGIN

class JsonSerializeFormat;
class JsonStructSerializer;
class JsonArraySerializer;

class DLL_LINKAGE IInstanceResolver
{
public:
	virtual ~IInstanceResolver(){};
	virtual si32 decode(const std::string & identifier) const = 0;
	virtual std::string encode(si32 identifier) const = 0;
};

class DLL_LINKAGE JsonSerializeHelper: public boost::noncopyable
{
public:
	JsonSerializeHelper(JsonSerializeHelper && other) noexcept;
	virtual ~JsonSerializeHelper();

	JsonSerializeFormat * operator->();

protected:
	JsonSerializeHelper(JsonSerializeFormat * owner_);

	JsonSerializeFormat * owner;
private:
	bool restoreState;
};

class DLL_LINKAGE JsonStructSerializer: public JsonSerializeHelper
{
public:
	JsonStructSerializer(JsonStructSerializer && other) noexcept;

protected:
	JsonStructSerializer(JsonSerializeFormat * owner_);

	friend class JsonSerializeFormat;
	friend class JsonArraySerializer;
};

class DLL_LINKAGE JsonArraySerializer: public JsonSerializeHelper
{
public:
	JsonArraySerializer(JsonArraySerializer && other) noexcept;

	JsonStructSerializer enterStruct(const size_t index);
	JsonArraySerializer enterArray(const size_t index);

	template <typename Container>
	void syncSize(Container & c, JsonNode::JsonType type = JsonNode::JsonType::DATA_NULL);

	///Anything int64-convertible <-> Json integer
	template <typename T>
	void serializeInt(const size_t index, T & value);

	///String <-> Json string
	void serializeString(const size_t index, std::string & value);

	///vector of anything int-convertible <-> Json vector of integers
	template<typename T>
	void serializeArray(std::vector<T> & value)
	{
		syncSize(value, JsonNode::JsonType::DATA_STRUCT);

		for(size_t idx = 0; idx < size(); idx++)
			serializeInt(idx, value[idx]);
	}
	
	///vector of strings <-> Json vector of strings
	void serializeArray(std::vector<std::string> & value)
	{
		syncSize(value, JsonNode::JsonType::DATA_STRUCT);

		for(size_t idx = 0; idx < size(); idx++)
			serializeString(idx, value[idx]);
	}
	
	///vector of anything with custom serializing function <-> Json vector of structs
	template <typename Element>
	void serializeStruct(std::vector<Element> & value, std::function<void(JsonSerializeFormat&, Element&)> serializer)
	{
		syncSize(value, JsonNode::JsonType::DATA_STRUCT);

		for(size_t idx = 0; idx < size(); idx++)
		{
			auto s = enterStruct(idx);
			serializer(*owner, value[idx]);
		}
	}
	
	///vector of serializable <-> Json vector of structs
	template <typename Element>
	void serializeStruct(std::vector<Element> & value)
	{
		serializeStruct<Element>(value, [](JsonSerializeFormat & h, Element & e){e.serializeJson(h);});
	}

	void resize(const size_t newSize);
	void resize(const size_t newSize, JsonNode::JsonType type);
	size_t size() const;
protected:
	JsonArraySerializer(JsonSerializeFormat * owner_);

	friend class JsonSerializeFormat;
private:
	const JsonNode * thisNode;

	void serializeInt64(const size_t index, int64_t & value);
};

class DLL_LINKAGE JsonSerializeFormat: public boost::noncopyable
{
public:
	///user-provided callback to resolve string identifier
	///returns resolved identifier or -1 on error
	using TDecoder = std::function<si32(const std::string &)>;

	///user-provided callback to get string identifier
	///may assume that object index is valid
	using TEncoder = std::function<std::string(si32)>;

	struct LICSet
	{
		LICSet(const std::set<si32> & Standard, TDecoder Decoder, TEncoder Encoder);

		const std::set<si32> & standard;
		const TDecoder decoder;
		const TEncoder encoder;
		std::set<si32> all;
		std::set<si32> any;
		std::set<si32> none;
	};

	const bool saving;
	const bool updating;

	JsonSerializeFormat() = delete;
	virtual ~JsonSerializeFormat() = default;

	virtual const JsonNode & getCurrent() = 0;

	JsonStructSerializer enterStruct(const std::string & fieldName);
	JsonArraySerializer enterArray(const std::string & fieldName);

	///Anything comparable <-> Json bool
	template <typename T>
	void serializeBool(const std::string & fieldName, T & value, const T trueValue, const T falseValue, const T defaultValue)
	{
		boost::logic::tribool temp(boost::logic::indeterminate);

		if(value == defaultValue)
			;//leave as indeterminate
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
	void serializeBool(const std::string & fieldName, bool & value, const bool defaultValue);

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
	virtual void serializeLIC(const std::string & fieldName, const TDecoder & decoder, const TEncoder & encoder, const std::set<int32_t> & standard, std::set<int32_t> & value) = 0;

	template<typename T>
	void serializeLIC(const std::string & fieldName, const TDecoder & decoder, const TEncoder & encoder, const std::set<T> & standard, std::set<T> & value)
	{
		std::set<int32_t> standardInt;
		std::set<int32_t> valueInt;

		for (auto entry : standard)
			standardInt.insert(entry.getNum());

		for (auto entry : value)
			valueInt.insert(entry.getNum());

		serializeLIC(fieldName, decoder, encoder, standardInt, valueInt);

		value.clear();
		for (auto entry : valueInt)
			value.insert(T(entry));
	}

	/** @brief Complete serialization of Logical identifier condition.
	 * Assumes that all values are allowed by default, and standard contains them
	 */
	virtual void serializeLIC(const std::string & fieldName, LICSet & value) = 0;

	///String <-> Json string
	virtual void serializeString(const std::string & fieldName, std::string & value) = 0;

	///si32-convertible enum <-> Json string enum
	template <typename T>
	void serializeEnum(const std::string & fieldName, T & value, const std::vector<std::string> & enumMap)
	{
		doSerializeInternal<T, T, si32>(fieldName, value, std::nullopt, enumMap);
	};

	///si32-convertible enum <-> Json string enum
	template <typename T, typename U>
	void serializeEnum(const std::string & fieldName, T & value, const U & defaultValue, const std::vector<std::string> & enumMap)
	{
		doSerializeInternal<T, U, si32>(fieldName, value, defaultValue, enumMap);
	};

	template <typename T, typename U, typename C>
	void serializeEnum(const std::string & fieldName, T & value, const U & defaultValue, const C & enumMap)
	{
		std::vector<std::string> enumMapCopy;

		std::copy(std::begin(enumMap), std::end(enumMap), std::back_inserter(enumMapCopy));

		doSerializeInternal<T, U, si32>(fieldName, value, defaultValue, enumMapCopy);
	};

	///Anything double-convertible <-> Json double
	template <typename T>
	void serializeFloat(const std::string & fieldName, T & value)
	{
		doSerializeInternal<T, T, double>(fieldName, value, std::nullopt);
	};

	///Anything double-convertible <-> Json double
	template <typename T, typename U>
	void serializeFloat(const std::string & fieldName, T & value, const U & defaultValue)
	{
		doSerializeInternal<T, U, double>(fieldName, value, defaultValue);
	};

	///Anything int64-convertible <-> Json integer
	///no default value
	template <typename T>
	void serializeInt(const std::string & fieldName, T & value)
	{
		doSerializeInternal<T, T, si64>(fieldName, value, std::nullopt);
	};

	///Anything int64-convertible <-> Json integer
	///custom default value
	template <typename T, typename U>
	void serializeInt(const std::string & fieldName, T & value, const U & defaultValue)
	{
		doSerializeInternal<T, U, si64>(fieldName, value, defaultValue);
	};

	///Anything int64-convertible <-> Json integer
	///default value is std::nullopt
	template<typename T>
	void serializeInt(const std::string & fieldName, std::optional<T> & value)
	{
		dispatchOptional<T, si64>(fieldName, value);
	};

	///si32-convertible identifier <-> Json string
	template <typename T, typename U>
	void serializeId(const std::string & fieldName, T & value, const U & defaultValue, const TDecoder & decoder, const TEncoder & encoder)
	{
		doSerializeInternal<T, U, si32>(fieldName, value, defaultValue, decoder, encoder);
	}

	///si32-convertible identifier <-> Json string
	template <typename IdentifierType, typename IdentifierTypeBase = IdentifierType>
	void serializeId(const std::string & fieldName, IdentifierType & value, const IdentifierTypeBase & defaultValue = IdentifierType::NONE)
	{
		static_assert(std::is_base_of_v<IdentifierBase, IdentifierType>, "This method can only serialize Identifier classes!");

		if (saving)
		{
			if (value != defaultValue)
			{
				std::string fieldValue = IdentifierType::encode(value.getNum());
				serializeString(fieldName, fieldValue);
			}
		}
		else
		{
			const JsonNode & fieldValue = getCurrent()[fieldName];

			if (!fieldValue.String().empty())
			{
				LIBRARY->identifiers()->requestIdentifier(IdentifierType::entityType(), fieldValue, [&value](int32_t index){
					value = IdentifierType(index);
				});
			}
			else
			{
				value = IdentifierType(defaultValue);
			}
		}
	}

	///si32-convertible identifier vector <-> Json array of string
	template <typename T, typename E = T>
	void serializeIdArray(const std::string & fieldName, std::vector<T> & value)
	{
		if (saving)
		{
			std::vector<std::string> fieldValue;

			for(const T & vitem : value)
				fieldValue.push_back(E::encode(vitem.getNum()));

			serializeInternal(fieldName, fieldValue);
		}
		else
		{
			const JsonVector & fieldValue = getCurrent()[fieldName].Vector();
			value.resize(fieldValue.size());

			for(size_t i = 0; i < fieldValue.size(); ++i)
			{
				LIBRARY->identifiers()->requestIdentifier(E::entityType(), fieldValue[i], [&value, i](int32_t index){
					value[i] = T(index);
				});
			}
		}
	}

	///si32-convertible identifier set <-> Json array of string
	template <typename T, typename U = T>
	void serializeIdArray(const std::string & fieldName, std::set<T> & value)
	{
		if (saving)
		{
			std::vector<std::string> fieldValue;

			for(const T & vitem : value)
				fieldValue.push_back(U::encode(vitem.getNum()));

			serializeInternal(fieldName, fieldValue);
		}
		else
		{
			for (const auto & element : getCurrent()[fieldName].Vector())
			{
				LIBRARY->identifiers()->requestIdentifierIfFound(U::entityType(), element, [&value](int32_t index){
					value.insert(T(index));
				});
			}
		}
	}

	///si32-convertible instance identifier <-> Json string
	template <typename T>
	void serializeInstance(const std::string & fieldName, T & value, const T & defaultValue)
	{
		const TDecoder decoder = std::bind(&IInstanceResolver::decode, instanceResolver, _1);
		const TEncoder encoder = std::bind(&IInstanceResolver::encode, instanceResolver, _1);

		if (saving)
		{
			if (value != defaultValue)
			{
				std::string fieldValue = encoder(value.getNum());
				serializeString(fieldName, fieldValue);
			}
		}
		else
		{
			std::string fieldValue;
			serializeString(fieldName, fieldValue);

			if (!fieldValue.empty())
				value = T(decoder(fieldValue));
			else
				value = T(defaultValue);
		}
	}

	///any serializable object <-> Json struct
	template <typename T>
	void serializeStruct(const std::string & fieldName, T & value)
	{
		auto guard = enterStruct(fieldName);
		value.serializeJson(*this);
	}

	virtual void serializeRaw(const std::string & fieldName, JsonNode & value, const std::optional<std::reference_wrapper<const JsonNode>> defaultValue) = 0;

protected:
	JsonSerializeFormat(const IInstanceResolver * instanceResolver_, const bool saving_, const bool updating_);

	///bool <-> Json bool, indeterminate is default
	virtual void serializeInternal(const std::string & fieldName, boost::logic::tribool & value) = 0;

	///Numeric Id <-> String Id
	virtual void serializeInternal(const std::string & fieldName, si32 & value, const std::optional<si32> & defaultValue, const TDecoder & decoder, const TEncoder & encoder) = 0;

	///Numeric Id vector <-> String Id vector
	virtual void serializeInternal(const std::string & fieldName, std::vector<si32> & value, const TDecoder & decoder, const TEncoder & encoder) = 0;

	///Numeric <-> Json double
	virtual void serializeInternal(const std::string & fieldName, double & value, const std::optional<double> & defaultValue) = 0;

	///Numeric <-> Json integer
	virtual void serializeInternal(const std::string & fieldName, si64 & value, const std::optional<si64> & defaultValue) = 0;

	///Enum/Numeric <-> Json string enum
	virtual void serializeInternal(const std::string & fieldName, si32 & value, const std::optional<si32> & defaultValue, const std::vector<std::string> & enumMap) = 0;

	///String vector <-> Json string vector
	virtual void serializeInternal(const std::string & fieldName, std::vector<std::string> & value) = 0;

	virtual void pop() = 0;
	virtual void pushStruct(const std::string & fieldName) = 0;
	virtual void pushArray(const std::string & fieldName) = 0;
	virtual void pushArrayElement(const size_t index) = 0;
	virtual void pushField(const std::string & fieldName) = 0;

	virtual void resizeCurrent(const size_t newSize, JsonNode::JsonType type){};

	virtual void serializeInternal(std::string & value) = 0;
	virtual void serializeInternal(int64_t & value) = 0;

	void readLICPart(const JsonNode & part, const JsonSerializeFormat::TDecoder & decoder, std::set<si32> & value) const;

private:
	const IInstanceResolver * instanceResolver;

	template<typename VType, typename DVType, typename IType, typename... Args>
	void doSerializeInternal(const std::string & fieldName, VType & value, const std::optional<DVType> & defaultValue, Args... args)
	{
		const std::optional<IType> tempDefault = defaultValue ? std::optional<IType>(static_cast<IType>(defaultValue.value())) : std::nullopt;
		auto temp = static_cast<IType>(value);

		serializeInternal(fieldName, temp, tempDefault, args...);

		if(!saving)
			value = static_cast<VType>(temp);
	}

	template<typename VType, typename IType, typename... Args>
	void dispatchOptional(const std::string & fieldName, std::optional<VType> & value, Args... args)
	{
		if(saving)
		{
			if(value)
			{
				auto temp = static_cast<IType>(value.value());
				pushField(fieldName);
				serializeInternal(temp, args...);
				pop();
			}
		}
		else
		{
			pushField(fieldName);

			if(getCurrent().getType() == JsonNode::JsonType::DATA_NULL)
			{
				value = std::nullopt;
			}
			else
			{
				IType temp = IType();
				serializeInternal(temp, args...);
				value = std::make_optional(temp);
			}

			pop();
		}
	}

	friend class JsonSerializeHelper;
	friend class JsonStructSerializer;
	friend class JsonArraySerializer;
};

template <typename Container>
void JsonArraySerializer::syncSize(Container & c, JsonNode::JsonType type)
{
	if(owner->saving)
		resize(c.size(), type);
	else
		c.resize(size());
}

template <typename T>
void JsonArraySerializer::serializeInt(const size_t index, T & value)
{
	auto temp = static_cast<int64_t>(value);

	serializeInt64(index, temp);

	if (!owner->saving)
		value = static_cast<T>(temp);
};

VCMI_LIB_NAMESPACE_END
