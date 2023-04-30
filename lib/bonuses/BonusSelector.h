/*
 * BonusSelector.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "HeroBonus.h"

VCMI_LIB_NAMESPACE_BEGIN

class CSelector : std::function<bool(const Bonus*)>
{
	using TBase = std::function<bool(const Bonus*)>;
public:
	CSelector() = default;
	template<typename T>
	CSelector(const T &t,	//SFINAE trick -> include this c-tor in overload resolution only if parameter is class
							//(includes functors, lambdas) or function. Without that VC is going mad about ambiguities.
		typename std::enable_if < boost::mpl::or_ < std::is_class<T>, std::is_function<T >> ::value>::type *dummy = nullptr)
		: TBase(t)
	{}

	CSelector(std::nullptr_t)
	{}

	CSelector And(CSelector rhs) const
	{
		//lambda may likely outlive "this" (it can be even a temporary) => we copy the OBJECT (not pointer)
		auto thisCopy = *this;
		return [thisCopy, rhs](const Bonus *b) mutable { return thisCopy(b) && rhs(b); };
	}
	CSelector Or(CSelector rhs) const
	{
		auto thisCopy = *this;
		return [thisCopy, rhs](const Bonus *b) mutable { return thisCopy(b) || rhs(b); };
	}

	CSelector Not() const
	{
		auto thisCopy = *this;
		return [thisCopy](const Bonus *b) mutable { return !thisCopy(b); };
	}

	bool operator()(const Bonus *b) const
	{
		return TBase::operator()(b);
	}

	operator bool() const
	{
		return !!static_cast<const TBase&>(*this);
	}
};

template<typename T>
class CSelectFieldEqual
{
	T Bonus::*ptr;

public:
	CSelectFieldEqual(T Bonus::*Ptr)
		: ptr(Ptr)
	{
	}

	CSelector operator()(const T &valueToCompareAgainst) const
	{
		auto ptr2 = ptr; //We need a COPY because we don't want to reference this (might be outlived by lambda)
		return [ptr2, valueToCompareAgainst](const Bonus *bonus)
		{
			return bonus->*ptr2 == valueToCompareAgainst;
		};
	}
};

class DLL_LINKAGE CWillLastTurns
{
public:
	int turnsRequested;

	bool operator()(const Bonus *bonus) const
	{
		return turnsRequested <= 0					//every present effect will last zero (or "less") turns
			|| !Bonus::NTurns(bonus) //so do every not expriing after N-turns effect
			|| bonus->turnsRemain > turnsRequested;
	}
	CWillLastTurns& operator()(const int &setVal)
	{
		turnsRequested = setVal;
		return *this;
	}
};

class DLL_LINKAGE CWillLastDays
{
public:
	int daysRequested;

	bool operator()(const Bonus *bonus) const
	{
		if(daysRequested <= 0 || Bonus::Permanent(bonus) || Bonus::OneBattle(bonus))
			return true;
		else if(Bonus::OneDay(bonus))
			return false;
		else if(Bonus::NDays(bonus) || Bonus::OneWeek(bonus))
		{
			return bonus->turnsRemain > daysRequested;
		}

		return false; // TODO: ONE_WEEK need support for turnsRemain, but for now we'll exclude all unhandled durations
	}
	CWillLastDays& operator()(const int &setVal)
	{
		daysRequested = setVal;
		return *this;
	}
};


namespace Selector
{
	extern DLL_LINKAGE CSelectFieldEqual<Bonus::BonusType> & type();
	extern DLL_LINKAGE CSelectFieldEqual<TBonusSubtype> & subtype();
	extern DLL_LINKAGE CSelectFieldEqual<CAddInfo> & info();
	extern DLL_LINKAGE CSelectFieldEqual<Bonus::BonusSource> & sourceType();
	extern DLL_LINKAGE CSelectFieldEqual<Bonus::BonusSource> & targetSourceType();
	extern DLL_LINKAGE CSelectFieldEqual<Bonus::LimitEffect> & effectRange();
	extern DLL_LINKAGE CWillLastTurns turns;
	extern DLL_LINKAGE CWillLastDays days;

	CSelector DLL_LINKAGE typeSubtype(Bonus::BonusType Type, TBonusSubtype Subtype);
	CSelector DLL_LINKAGE typeSubtypeInfo(Bonus::BonusType type, TBonusSubtype subtype, const CAddInfo & info);
	CSelector DLL_LINKAGE source(Bonus::BonusSource source, ui32 sourceID);
	CSelector DLL_LINKAGE sourceTypeSel(Bonus::BonusSource source);
	CSelector DLL_LINKAGE valueType(Bonus::ValueType valType);

	/**
	 * Selects all bonuses
	 * Usage example: Selector::all.And(<functor>).And(<functor>)...)
	 */
	extern DLL_LINKAGE CSelector all;

	/**
	 * Selects nothing
	 * Usage example: Selector::none.Or(<functor>).Or(<functor>)...)
	 */
	extern DLL_LINKAGE CSelector none;
}

VCMI_LIB_NAMESPACE_END