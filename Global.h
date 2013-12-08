#pragma once

/*
 * Global.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

/* ---------------------------------------------------------------------------- */
/* Compiler detection */
/* ---------------------------------------------------------------------------- */
// Fixed width bool data type is important for serialization
static_assert(sizeof(bool) == 1, "Bool needs to be 1 byte in size.");

#if defined _M_X64 && defined _WIN32 //Win64 -> cannot load 32-bit DLLs for video handling
#  define DISABLE_VIDEO
#endif

#ifdef __GNUC__
#  define GCC_VERSION (__GNUC__ * 100 + __GNUC_MINOR__ * 10 + __GNUC_PATCHLEVEL__)
#endif

#if !defined(__clang__) && defined(__GNUC__) && (GCC_VERSION < 460)
#  error VCMI requires at least gcc-4.6 for successfull compilation or clang-3.1. Please update your compiler
#endif

#if defined(__GNUC__) && (GCC_VERSION == 470 || GCC_VERSION == 471)
#  error This GCC version has buggy std::array::at version and should not be used. Please update to 4.7.2 or use 4.6.x.
#endif

/* ---------------------------------------------------------------------------- */
/* Guarantee compiler features */
/* ---------------------------------------------------------------------------- */
//defining available c++11 features

//initialization lists - gcc or clang
#if defined(__clang__) || defined(__GNUC__)
#  define CPP11_USE_INITIALIZERS_LIST
#endif

//override keyword - not present in gcc-4.6
#if !defined(_MSC_VER) && !defined(__clang__) && !(defined(__GNUC__) && (GCC_VERSION >= 470))
#  define override
#endif

/* ---------------------------------------------------------------------------- */
/* Suppress some compiler warnings */
/* ---------------------------------------------------------------------------- */
#ifdef _MSC_VER
#  pragma warning (disable : 4800 ) /* disable conversion to bool warning -- I think it's intended in all places */
#endif

/* ---------------------------------------------------------------------------- */
/* Commonly used C++, Boost headers */
/* ---------------------------------------------------------------------------- */
#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
#define _USE_MATH_DEFINES

#include <cstdio>
#include <stdio.h>

#include <algorithm>
#include <array>
#include <cassert>
#include <climits>
#include <cmath>
#include <cstdlib>
#include <functional>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <memory>
#include <numeric>
#include <queue>
#include <random>
#include <set>
#include <sstream>
#include <string>
#include <unordered_set>
#include <unordered_map>
#include <utility>
#include <vector>

//The only available version is 3, as of Boost 1.50
#include <boost/version.hpp>

#define BOOST_FILESYSTEM_VERSION 3
#if ( BOOST_VERSION>105000 )
#define BOOST_THREAD_VERSION 3
#endif
#define BOOST_THREAD_DONT_PROVIDE_THREAD_DESTRUCTOR_CALLS_TERMINATE_IF_JOINABLE 1
//#define BOOST_BIND_NO_PLACEHOLDERS

#include <boost/algorithm/string.hpp>
#include <boost/assign.hpp>
#include <boost/cstdint.hpp>
#include <boost/current_function.hpp>
#include <boost/crc.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/posix_time/posix_time_io.hpp>
#include <boost/filesystem.hpp>
#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/logic/tribool.hpp>
#include <boost/optional.hpp>
#include <boost/program_options.hpp>
#include <boost/range/adaptor/filtered.hpp>
#include <boost/range/adaptor/reversed.hpp>
#include <boost/range/algorithm.hpp>
#include <boost/thread.hpp>
#include <boost/variant.hpp>

#include <boost/math/special_functions/round.hpp>


#ifdef ANDROID
#include <android/log.h>
#endif

/* ---------------------------------------------------------------------------- */
/* Usings */
/* ---------------------------------------------------------------------------- */
using std::shared_ptr;
using std::unique_ptr;
using std::make_shared;
//using namespace std::placeholders;
namespace range = boost::range;

/* ---------------------------------------------------------------------------- */
/* Typedefs */
/* ---------------------------------------------------------------------------- */
// Integral data types
typedef boost::uint64_t ui64; //unsigned int 64 bits (8 bytes)
typedef boost::uint32_t ui32;  //unsigned int 32 bits (4 bytes)
typedef boost::uint16_t ui16; //unsigned int 16 bits (2 bytes)
typedef boost::uint8_t ui8; //unsigned int 8 bits (1 byte)
typedef boost::int64_t si64; //signed int 64 bits (8 bytes)
typedef boost::int32_t si32; //signed int 32 bits (4 bytes)
typedef boost::int16_t si16; //signed int 16 bits (2 bytes)
typedef boost::int8_t si8; //signed int 8 bits (1 byte)

// Lock typedefs
typedef boost::lock_guard<boost::mutex> TLockGuard;
typedef boost::lock_guard<boost::recursive_mutex> TLockGuardRec;

/* ---------------------------------------------------------------------------- */
/* Macros */
/* ---------------------------------------------------------------------------- */
// Import + Export macro declarations
#ifdef _WIN32
#  ifdef __GNUC__
#    define DLL_EXPORT __attribute__((dllexport))
#  else
#    define DLL_EXPORT __declspec(dllexport)
#  endif
#else
#  ifdef __GNUC__
#    define DLL_EXPORT	__attribute__ ((visibility("default")))
#  endif
#endif

#ifdef _WIN32
#  ifdef __GNUC__
#    define DLL_IMPORT __attribute__((dllimport))
#  else
#    define DLL_IMPORT __declspec(dllimport)
#  endif
#else
#  ifdef __GNUC__
#    define DLL_IMPORT	__attribute__ ((visibility("default")))
#  endif
#endif

#ifdef VCMI_DLL
#  define DLL_LINKAGE DLL_EXPORT
#else
#  define DLL_LINKAGE DLL_IMPORT
#endif

#define THROW_FORMAT(message, formatting_elems)  throw std::runtime_error(boost::str(boost::format(message) % formatting_elems))

#define ASSERT_IF_CALLED_WITH_PLAYER if(!player) {logGlobal->errorStream() << BOOST_CURRENT_FUNCTION; assert(0);}

//XXX pls dont - 'debug macros' are usually more trouble than it's worth
#define HANDLE_EXCEPTION  \
    catch (const std::exception& e) {	\
	logGlobal->errorStream() << e.what();		\
    throw;								\
}									\
    catch (const std::exception * e)	\
{									\
	logGlobal->errorStream() << e->what();	\
    throw;							\
}									\
    catch (const std::string& e) {		\
	logGlobal->errorStream() << e;		\
    throw;							\
}

#define HANDLE_EXCEPTIONC(COMMAND)  \
    catch (const std::exception& e) {	\
    COMMAND;						\
	logGlobal->errorStream() << e.what();	\
    throw;							\
}									\
    catch (const std::string &e)	\
{									\
    COMMAND;						\
	logGlobal->errorStream() << e;	\
    throw;							\
}

// can be used for counting arrays
template<typename T, size_t N> char (&_ArrayCountObj(const T (&)[N]))[N];
#define ARRAY_COUNT(arr)    (sizeof(_ArrayCountObj(arr)))

// should be used for variables that becomes unused in release builds (e.g. only used for assert checks)
#define UNUSED(VAR) ((void)VAR)

/* ---------------------------------------------------------------------------- */
/* VCMI standard library */
/* ---------------------------------------------------------------------------- */
template<typename T>
std::ostream & operator<<(std::ostream & out, const boost::optional<T> & opt)
{
	if(opt) return out << *opt;
	else return out << "empty";
}

template<typename T>
std::ostream & operator<<(std::ostream & out, const std::vector<T> & container)
{
	out << "[";
	for(auto it = container.begin(); it != container.end(); ++it)
	{
		out << *it;
		if(std::prev(container.end()) != it) out << ", ";
	}
	return out << "]";
}

namespace vstd
{
	
	// combine hashes. Present in boost but not in std
	template <class T>
	inline void hash_combine(std::size_t& seed, const T& v)
	{
		std::hash<T> hasher;
		seed ^= hasher(v) + 0x9e3779b9 + (seed<<6) + (seed>>2);
	}
	
	//returns true if container c contains item i
	template <typename Container, typename Item>
	bool contains(const Container & c, const Item &i)
	{
		return std::find(std::begin(c), std::end(c),i) != std::end(c);
	}

	//returns true if container c contains item i
	template <typename Container, typename Pred>
	bool contains_if(const Container & c, Pred p)
	{
		return std::find_if(std::begin(c), std::end(c), p) != std::end(c);
	}

	//returns true if map c contains item i
	template <typename V, typename Item, typename Item2>
	bool contains(const std::map<Item,V> & c, const Item2 &i)
	{
		return c.find(i)!=c.end();
	}

	//returns true if unordered set c contains item i
	template <typename Item>
	bool contains(const std::unordered_set<Item> & c, const Item &i)
	{
		return c.find(i)!=c.end();
	}

	template <typename V, typename Item, typename Item2>
	bool contains(const std::unordered_map<Item,V> & c, const Item2 &i)
	{
		return c.find(i)!=c.end();
	}

	//returns position of first element in vector c equal to s, if there is no such element, -1 is returned
	template <typename Container, typename T2>
	int find_pos(const Container & c, const T2 &s)
	{
		size_t i=0;
		for (auto iter = std::begin(c); iter != std::end(c); iter++, i++)
			if(*iter == s)
				return i;
		return -1;
	}

	//Func f tells if element matches
	template <typename Container, typename Func>
	int find_pos_if(const Container & c, const Func &f)
	{
		auto ret = boost::range::find_if(c, f);
		if(ret != std::end(c))
			return std::distance(std::begin(c), ret);

		return -1;
	}

	//returns iterator to the given element if present in container, end() if not
	template <typename Container, typename Item>
	typename Container::iterator find(Container & c, const Item &i)
	{
		return std::find(c.begin(),c.end(),i);
	}

	//returns const iterator to the given element if present in container, end() if not
	template <typename Container, typename Item>
	typename Container::const_iterator find(const Container & c, const Item &i)
	{
		return std::find(c.begin(),c.end(),i);
	}

	//removes element i from container c, returns false if c does not contain i
	template <typename Container, typename Item>
	typename Container::size_type operator-=(Container &c, const Item &i)
	{
		typename Container::iterator itr = find(c,i);
		if(itr == c.end())
			return false;
		c.erase(itr);
		return true;
	}

	//assigns greater of (a, b) to a and returns maximum of (a, b)
	template <typename t1, typename t2>
	t1 &amax(t1 &a, const t2 &b)
	{
		if(a >= b)
			return a;
		else
		{
			a = b;
			return a;
		}
	}

	//assigns smaller of (a, b) to a and returns minimum of (a, b)
	template <typename t1, typename t2>
	t1 &amin(t1 &a, const t2 &b)
	{
		if(a <= b)
			return a;
		else
		{
			a = b;
			return a;
		}
	}

	//makes a to fit the range <b, c>
	template <typename t1, typename t2, typename t3>
	t1 &abetween(t1 &a, const t2 &b, const t3 &c)
	{
		amax(a,b);
		amin(a,c);
		return a;
	}

	//checks if a is between b and c
	template <typename t1, typename t2, typename t3>
	bool isbetween(const t1 &value, const t2 &min, const t3 &max)
	{
		return value > min && value < max;
	}

	//checks if a is within b and c
	template <typename t1, typename t2, typename t3>
	bool iswithin(const t1 &value, const t2 &min, const t3 &max)
	{
		return value >= min && value <= max;
	}

	template <typename t1, typename t2>
	struct assigner
	{
	public:
		t1 &op1;
		t2 op2;
		assigner(t1 &a1, const t2 & a2)
			:op1(a1), op2(a2)
		{}
		void operator()()
		{
			op1 = op2;
		}
	};

	// Assigns value a2 to a1. The point of time of the real operation can be controlled
	// with the () operator.
	template <typename t1, typename t2>
	assigner<t1,t2> assigno(t1 &a1, const t2 &a2)
	{
		return assigner<t1,t2>(a1,a2);
	}

	//deleted pointer and sets it to nullptr
	template <typename T>
	void clear_pointer(T* &ptr)
	{
		delete ptr;
		ptr = nullptr;
	}

#if _MSC_VER >= 1800
	using std::make_unique;
#else
	template<typename T>
	std::unique_ptr<T> make_unique()
	{
		return std::unique_ptr<T>(new T());
	}
	template<typename T, typename Arg1>
	std::unique_ptr<T> make_unique(Arg1 &&arg1)
	{
		return std::unique_ptr<T>(new T(std::forward<Arg1>(arg1)));
	}
	template<typename T, typename Arg1, typename Arg2>
	std::unique_ptr<T> make_unique(Arg1 &&arg1, Arg2 &&arg2)
	{
		return std::unique_ptr<T>(new T(std::forward<Arg1>(arg1), std::forward<Arg2>(arg2)));
	}
	template<typename T, typename Arg1, typename Arg2, typename Arg3>
	std::unique_ptr<T> make_unique(Arg1 &&arg1, Arg2 &&arg2, Arg3 &&arg3)
	{
		return std::unique_ptr<T>(new T(std::forward<Arg1>(arg1), std::forward<Arg2>(arg2), std::forward<Arg3>(arg3)));
	}
	template<typename T, typename Arg1, typename Arg2, typename Arg3, typename Arg4>
	std::unique_ptr<T> make_unique(Arg1 &&arg1, Arg2 &&arg2, Arg3 &&arg3, Arg4 &&arg4)
	{
		return std::unique_ptr<T>(new T(std::forward<Arg1>(arg1), std::forward<Arg2>(arg2), std::forward<Arg3>(arg3), std::forward<Arg4>(arg4)));
	}
#endif

	template <typename Container>
	typename Container::const_reference circularAt(const Container &r, size_t index)
	{
		assert(r.size());
		index %= r.size();
		auto itr = std::begin(r);
		std::advance(itr, index);
		return *itr;
	}

	template<typename Range, typename Predicate>
	void erase_if(Range &vec, Predicate pred)
	{
		vec.erase(boost::remove_if(vec, pred),vec.end());
	}

	template<typename Elem, typename Predicate>
	void erase_if(std::set<Elem> &setContainer, Predicate pred)
	{
		auto itr = setContainer.begin();
		auto endItr = setContainer.end(); 
		while(itr != endItr)
		{
			auto tmpItr = itr++;
			if(pred(*tmpItr))
				setContainer.erase(tmpItr);
		}
	}

	//works for map and std::map, maybe something else
	template<typename Key, typename Val, typename Predicate>
	void erase_if(std::map<Key, Val> &container, Predicate pred)
	{
		auto itr = container.begin();
		auto endItr = container.end(); 
		while(itr != endItr)
		{
			auto tmpItr = itr++;
			if(pred(*tmpItr))
				container.erase(tmpItr);
		}
	}

	template<typename InputRange, typename OutputIterator, typename Predicate>
	OutputIterator copy_if(const InputRange &input, OutputIterator result, Predicate pred)
	{
		return std::copy_if(boost::const_begin(input), std::end(input), result, pred);
	}

	template <typename Container>
	std::insert_iterator<Container> set_inserter(Container &c)
	{
		return std::inserter(c, c.end());
	}

	//Returns iterator to the element for which the value of ValueFunction is minimal
	template<class ForwardRange, class ValueFunction>
	auto minElementByFun(const ForwardRange& rng, ValueFunction vf) -> decltype(std::begin(rng))
	{
		typedef decltype(*std::begin(rng)) ElemType;
		return boost::min_element(rng, [&] (ElemType lhs, ElemType rhs) -> bool
		{
			return vf(lhs) < vf(rhs);
		});
	}
		
	//Returns iterator to the element for which the value of ValueFunction is maximal
	template<class ForwardRange, class ValueFunction>
	auto maxElementByFun(const ForwardRange& rng, ValueFunction vf) -> decltype(std::begin(rng))
	{
		typedef decltype(*std::begin(rng)) ElemType;
		return boost::max_element(rng, [&] (ElemType lhs, ElemType rhs) -> bool
		{
			return vf(lhs) < vf(rhs);
		});
	}

	static inline int retreiveRandNum(const std::function<int()> &randGen)
	{
		if (randGen)
			return randGen();
		else
			return rand();
	}

	template <typename T> const T & pickRandomElementOf(const std::vector<T> &v, const std::function<int()> &randGen)
	{
		return v.at(retreiveRandNum(randGen) % v.size());
	}

	template<typename T>
	void advance(T &obj, int change)
	{
		obj = (T)(((int)obj) + change);
	}

	template <typename Container>
	typename Container::value_type backOrNull(const Container &c) //returns last element of container or nullptr if it is empty (to be used with containers of pointers)
	{
		if(c.size())
			return c.back();
		else
			return typename Container::value_type();
	}

	template <typename Container>
	typename Container::value_type frontOrNull(const Container &c) //returns first element of container or nullptr if it is empty (to be used with containers of pointers)
	{
		if(c.size())
			return c.front();
		else
			return nullptr;
	}

	template <typename Container, typename Index>
	bool isValidIndex(const Container &c, Index i)
	{
		return i >= 0  &&  i < c.size();
	}

	template <typename Container, typename Index>
	boost::optional<typename Container::const_reference> tryAt(const Container &c, Index i)
	{
		if(isValidIndex(c, i))
		{
			auto itr = c.begin();
			std::advance(itr, i);
			return *itr;
		}
		return boost::none;
	}

	template <typename Container, typename Pred>
	static boost::optional<typename Container::const_reference> tryFindIf(const Container &r, const Pred &t)
	{
		auto pos = range::find_if(r, t);
		if(pos == boost::end(r))
			return boost::none;
		else
			return *pos;
	}

	template <typename Container>
	typename Container::const_reference atOrDefault(const Container &r, size_t index, const typename Container::const_reference &defaultValue)
	{
		if(index < r.size())
			return r[index];
		
		return defaultValue;
	}

	using boost::math::round;
}
using vstd::operator-=;
using vstd::make_unique;

/* ---------------------------------------------------------------------------- */
/* VCMI headers */
/* ---------------------------------------------------------------------------- */
#include "lib/logging/CLogger.h"
