/*
 * IdentifierBase.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

VCMI_LIB_NAMESPACE_BEGIN

class IdentifierResolutionException : public std::runtime_error
{
public:
	const std::string identifierName;

	IdentifierResolutionException(const std::string & identifierName )
		: std::runtime_error("Failed to resolve identifier " + identifierName)
		, identifierName(identifierName)
	{}
};

class IdentifierBase
{
protected:
	constexpr IdentifierBase():
		num(-1)
	{}

	explicit constexpr IdentifierBase(int32_t value):
		num(value)
	{}

	~IdentifierBase() = default;

	/// Attempts to resolve identifier using provided entity type
	/// Returns resolved numeric identifier
	/// Throws IdentifierResolutionException on failure
	static int32_t resolveIdentifier(const std::string & entityType, const std::string identifier);
public:
	int32_t num;

	constexpr int32_t getNum() const
	{
		return num;
	}

	constexpr void setNum(int32_t value)
	{
		num = value;
	}

	constexpr bool hasValue() const
	{
		return num >= 0;
	}

	struct hash
	{
		size_t operator()(const IdentifierBase & id) const
		{
			return std::hash<int>()(id.num);
		}
	};

	constexpr void advance(int change)
	{
		num += change;
	}

	constexpr operator int32_t () const
	{
		return num;
	}

	friend std::ostream& operator<<(std::ostream& os, const IdentifierBase& dt)
	{
		return os << dt.num;
	}
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Note: use template to force different type, blocking any Identifier<A> <=> Identifier<B> comparisons
template<typename FinalClass>
class Identifier : public IdentifierBase
{
	using BaseClass = IdentifierBase;
public:
	constexpr Identifier()
	{}

	explicit constexpr Identifier(int32_t value):
		IdentifierBase(value)
	{}

	constexpr bool operator == (const Identifier & b) const { return BaseClass::num == b.num; }
	constexpr bool operator <= (const Identifier & b) const { return BaseClass::num <= b.num; }
	constexpr bool operator >= (const Identifier & b) const { return BaseClass::num >= b.num; }
	constexpr bool operator != (const Identifier & b) const { return BaseClass::num != b.num; }
	constexpr bool operator <  (const Identifier & b) const { return BaseClass::num <  b.num; }
	constexpr bool operator >  (const Identifier & b) const { return BaseClass::num >  b.num; }

	constexpr FinalClass & operator++()
	{
		++BaseClass::num;
		return static_cast<FinalClass&>(*this);
	}

	constexpr FinalClass & operator--()
	{
		--BaseClass::num;
		return static_cast<FinalClass&>(*this);
	}

	constexpr FinalClass operator--(int)
	{
		FinalClass ret(num);
		--BaseClass::num;
		return ret;
	}

	constexpr FinalClass operator++(int)
	{
		FinalClass ret(num);
		++BaseClass::num;
		return ret;
	}
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template<typename FinalClass, typename BaseClass>
class IdentifierWithEnum : public BaseClass
{
	using EnumType = typename BaseClass::Type;

	static_assert(std::is_same_v<std::underlying_type_t<EnumType>, int32_t>, "Entity Identifier must use int32_t");
public:
	constexpr EnumType toEnum() const
	{
		return static_cast<EnumType>(BaseClass::num);
	}

	constexpr IdentifierWithEnum(const EnumType & enumValue)
	{
		BaseClass::num = static_cast<int32_t>(enumValue);
	}

	constexpr IdentifierWithEnum(int32_t _num = -1)
	{
		BaseClass::num = _num;
	}

	constexpr bool operator == (const EnumType & b) const { return BaseClass::num == static_cast<int32_t>(b); }
	constexpr bool operator <= (const EnumType & b) const { return BaseClass::num <= static_cast<int32_t>(b); }
	constexpr bool operator >= (const EnumType & b) const { return BaseClass::num >= static_cast<int32_t>(b); }
	constexpr bool operator != (const EnumType & b) const { return BaseClass::num != static_cast<int32_t>(b); }
	constexpr bool operator <  (const EnumType & b) const { return BaseClass::num <  static_cast<int32_t>(b); }
	constexpr bool operator >  (const EnumType & b) const { return BaseClass::num >  static_cast<int32_t>(b); }

	constexpr bool operator == (const IdentifierWithEnum & b) const { return BaseClass::num == b.num; }
	constexpr bool operator <= (const IdentifierWithEnum & b) const { return BaseClass::num <= b.num; }
	constexpr bool operator >= (const IdentifierWithEnum & b) const { return BaseClass::num >= b.num; }
	constexpr bool operator != (const IdentifierWithEnum & b) const { return BaseClass::num != b.num; }
	constexpr bool operator <  (const IdentifierWithEnum & b) const { return BaseClass::num <  b.num; }
	constexpr bool operator >  (const IdentifierWithEnum & b) const { return BaseClass::num >  b.num; }

	constexpr FinalClass & operator++()
	{
		++BaseClass::num;
		return static_cast<FinalClass&>(*this);
	}

	constexpr FinalClass operator++(int)
	{
		FinalClass ret(BaseClass::num);
		++BaseClass::num;
		return ret;
	}
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template<typename FinalClass>
class EntityIdentifier : public Identifier<FinalClass>
{
public:
	using Identifier<FinalClass>::Identifier;

	template <typename Handler>
	void serialize(Handler &h)
	{
		auto * finalClass = static_cast<FinalClass*>(this);
		std::string value;

		if (h.saving)
			value = FinalClass::encode(finalClass->num);

		h & value;

		if (!h.saving)
			finalClass->num = FinalClass::decode(value);
	}
};

template<typename FinalClass, typename BaseClass>
class EntityIdentifierWithEnum : public IdentifierWithEnum<FinalClass, BaseClass>
{
public:
	using IdentifierWithEnum<FinalClass, BaseClass>::IdentifierWithEnum;

	template <typename Handler>
	void serialize(Handler &h)
	{
		auto * finalClass = static_cast<FinalClass*>(this);
		std::string value;

		if (h.saving)
			value = FinalClass::encode(finalClass->num);

		h & value;

		if (!h.saving)
			finalClass->num = FinalClass::decode(value);
	}
};

template<typename FinalClass>
class StaticIdentifier : public Identifier<FinalClass>
{
public:
	using Identifier<FinalClass>::Identifier;

	template <typename Handler>
	void serialize(Handler &h)
	{
		auto * finalClass = static_cast<FinalClass*>(this);
		h & finalClass->num;
	}
};

template<typename FinalClass, typename BaseClass>
class StaticIdentifierWithEnum : public IdentifierWithEnum<FinalClass, BaseClass>
{
public:
	using IdentifierWithEnum<FinalClass, BaseClass>::IdentifierWithEnum;

	template <typename Handler>
	void serialize(Handler &h)
	{
		auto * finalClass = static_cast<FinalClass*>(this);
		h & finalClass->num;
	}
};

VCMI_LIB_NAMESPACE_END
