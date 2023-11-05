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

	struct hash
	{
		size_t operator()(const IdentifierBase & id) const
		{
			return std::hash<int>()(id.num);
		}
	};

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & num;
	}

	constexpr void advance(int change)
	{
		num += change;
	}

//	constexpr operator int32_t () const
//	{
//		return num;
//	}

	friend std::ostream& operator<<(std::ostream& os, const IdentifierBase& dt)
	{
		return os << dt.num;
	}
};
