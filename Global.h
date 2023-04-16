/*
 * Global.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

/* ---------------------------------------------------------------------------- */
/* Compiler detection */
/* ---------------------------------------------------------------------------- */
// Fixed width bool data type is important for serialization
static_assert(sizeof(bool) == 1, "Bool needs to be 1 byte in size.");

/* ---------------------------------------------------------------------------- */
/* System detection. */
/* ---------------------------------------------------------------------------- */
// Based on: http://sourceforge.net/p/predef/wiki/OperatingSystems/
//	 and on: http://stackoverflow.com/questions/5919996/how-to-detect-reliably-mac-os-x-ios-linux-windows-in-c-preprocessor
// TODO?: Should be moved to vstd\os_detect.h (and then included by Global.h)
#ifdef _WIN16			// Defined for 16-bit environments
#  error "16-bit Windows isn't supported"
#elif defined(_WIN64)	// Defined for 64-bit environments
#  define VCMI_WINDOWS
#  define VCMI_WINDOWS_64
#elif defined(_WIN32)	// Defined for both 32-bit and 64-bit environments
#  define VCMI_WINDOWS
#  define VCMI_WINDOWS_32
#elif defined(_WIN32_WCE)
#  error "Windows CE isn't supported"
#elif defined(__linux__) || defined(__gnu_linux__) || defined(linux) || defined(__linux)
#  define VCMI_UNIX
#  define VCMI_XDG
#  if defined(__ANDROID__) || defined(ANDROID)
#    define VCMI_ANDROID
#  endif
#elif defined(__FreeBSD_kernel__) || defined(__FreeBSD__)
#  define VCMI_UNIX
#  define VCMI_XDG
#  define VCMI_FREEBSD
#elif defined(__HAIKU__)
#  define VCMI_UNIX
#  define VCMI_XDG
#  define VCMI_HAIKU
#elif defined(__GNU__) || defined(__gnu_hurd__) || (defined(__MACH__) && !defined(__APPLE__))
#  define VCMI_UNIX
#  define VCMI_XDG
#  define VCMI_HURD
#elif defined(__APPLE__) && defined(__MACH__)
#  define VCMI_UNIX
#  define VCMI_APPLE
#  include "TargetConditionals.h"
#  if TARGET_OS_SIMULATOR || TARGET_IPHONE_SIMULATOR
#    define VCMI_IOS
#    define VCMI_IOS_SIM
#  elif TARGET_OS_IPHONE
#    define VCMI_IOS
#  elif TARGET_OS_MAC
#    define VCMI_MAC
#  else
//#  warning "Unknown Apple target."?
#  endif
#else
#  error "This platform isn't supported"
#endif

#if defined(VCMI_ANDROID) || defined(VCMI_IOS)
#define VCMI_MOBILE
#endif

/* ---------------------------------------------------------------------------- */
/* Commonly used C++, Boost headers */
/* ---------------------------------------------------------------------------- */
#ifdef VCMI_WINDOWS
#  ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN		 // Exclude rarely-used stuff from Windows headers - delete this line if something is missing.
#  endif
#  ifndef NOMINMAX
#    define NOMINMAX				 // Exclude min/max macros from <Windows.h>. Use std::[min/max] from <algorithm> instead.
#  endif
#  ifndef _NO_W32_PSEUDO_MODIFIERS
#    define _NO_W32_PSEUDO_MODIFIERS // Exclude more macros for compiling with MinGW on Linux.
#  endif
#endif

/* ---------------------------------------------------------------------------- */
/* A macro to force inlining some of our functions */
/* ---------------------------------------------------------------------------- */
// Compiler (at least MSVC) is not so smart here-> without that displaying is MUCH slower
#ifdef _MSC_VER
#  define STRONG_INLINE __forceinline
#elif __GNUC__
#  define STRONG_INLINE inline __attribute__((always_inline))
#else
#  define STRONG_INLINE inline
#endif

#define _USE_MATH_DEFINES

#include <algorithm>
#include <any>
#include <array>
#include <atomic>
#include <bitset>
#include <cassert>
#include <climits>
#include <cmath>
#include <cstdlib>
#include <cstdio>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <numeric>
#include <queue>
#include <random>
#include <set>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

//The only available version is 3, as of Boost 1.50
#include <boost/version.hpp>

#define BOOST_FILESYSTEM_VERSION 3
#if BOOST_VERSION > 105000
#  define BOOST_THREAD_VERSION 3
#endif
#define BOOST_THREAD_DONT_PROVIDE_THREAD_DESTRUCTOR_CALLS_TERMINATE_IF_JOINABLE 1
//need to link boost thread dynamically to avoid https://stackoverflow.com/questions/35978572/boost-thread-interupt-does-not-work-when-crossing-a-dll-boundary
#define BOOST_THREAD_USE_DLL //for example VCAI::finish() may freeze on thread join after interrupt when linking this statically
#define BOOST_BIND_NO_PLACEHOLDERS

#if BOOST_VERSION >= 106600
#define BOOST_ASIO_ENABLE_OLD_SERVICES
#endif

#include <boost/algorithm/string.hpp>
#include <boost/crc.hpp>
#include <boost/current_function.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/posix_time/posix_time_io.hpp>
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/format.hpp>
#include <boost/functional/hash.hpp>
#include <boost/lexical_cast.hpp>
#ifdef VCMI_WINDOWS
#include <boost/locale/generator.hpp>
#endif
#include <boost/logic/tribool.hpp>
#include <boost/multi_array.hpp>
#include <boost/range/adaptor/filtered.hpp>
#include <boost/range/adaptor/reversed.hpp>
#include <boost/range/algorithm.hpp>
#include <boost/thread.hpp>

#ifndef M_PI
#  define M_PI 3.14159265358979323846
#endif

/* ---------------------------------------------------------------------------- */
/* Usings */
/* ---------------------------------------------------------------------------- */
using namespace std::placeholders;
namespace range = boost::range;

/* ---------------------------------------------------------------------------- */
/* Typedefs */
/* ---------------------------------------------------------------------------- */
// Integral data types
typedef uint64_t ui64; //unsigned int 64 bits (8 bytes)
typedef uint32_t ui32;  //unsigned int 32 bits (4 bytes)
typedef uint16_t ui16; //unsigned int 16 bits (2 bytes)
typedef uint8_t ui8; //unsigned int 8 bits (1 byte)
typedef int64_t si64; //signed int 64 bits (8 bytes)
typedef int32_t si32; //signed int 32 bits (4 bytes)
typedef int16_t si16; //signed int 16 bits (2 bytes)
typedef int8_t si8; //signed int 8 bits (1 byte)

// Lock typedefs
using TLockGuard = std::lock_guard<std::mutex>;
using TLockGuardRec = std::lock_guard<std::recursive_mutex>;

/* ---------------------------------------------------------------------------- */
/* Macros */
/* ---------------------------------------------------------------------------- */
// Import + Export macro declarations
#ifdef VCMI_WINDOWS
#ifdef VCMI_DLL_STATIC
#    define DLL_IMPORT
#    define DLL_EXPORT
#elif defined(__GNUC__)
#    define DLL_IMPORT __attribute__((dllimport))
#    define DLL_EXPORT __attribute__((dllexport))
#  else
#    define DLL_IMPORT __declspec(dllimport)
#    define DLL_EXPORT __declspec(dllexport)
#  endif
#  define ELF_VISIBILITY
#else
#  ifdef __GNUC__
#    define DLL_IMPORT	__attribute__ ((visibility("default")))
#    define DLL_EXPORT __attribute__ ((visibility("default")))
#    define ELF_VISIBILITY __attribute__ ((visibility("default")))
#  endif
#endif

#ifdef VCMI_DLL
#  define DLL_LINKAGE DLL_EXPORT
#else
#  define DLL_LINKAGE DLL_IMPORT
#endif

#define THROW_FORMAT(message, formatting_elems)  throw std::runtime_error(boost::str(boost::format(message) % formatting_elems))

// old iOS SDKs compatibility
#ifdef VCMI_IOS
#include <AvailabilityVersions.h>

#ifndef __IPHONE_13_0
#define __IPHONE_13_0 130000
#endif
#endif // VCMI_IOS

// single-process build makes 2 copies of the main lib by wrapping it in a namespace
#ifdef VCMI_LIB_NAMESPACE
#define VCMI_LIB_NAMESPACE_BEGIN namespace VCMI_LIB_NAMESPACE {
#define VCMI_LIB_NAMESPACE_END }
#define VCMI_LIB_USING_NAMESPACE using namespace VCMI_LIB_NAMESPACE;
#define VCMI_LIB_WRAP_NAMESPACE(x) VCMI_LIB_NAMESPACE::x
#else
#define VCMI_LIB_NAMESPACE_BEGIN
#define VCMI_LIB_NAMESPACE_END
#define VCMI_LIB_USING_NAMESPACE
#define VCMI_LIB_WRAP_NAMESPACE(x) ::x
#endif

/* ---------------------------------------------------------------------------- */
/* VCMI standard library */
/* ---------------------------------------------------------------------------- */
#include <vstd/CLoggerBase.h>

VCMI_LIB_NAMESPACE_BEGIN

void inline handleException()
{
	try
	{
		throw;
	}
	catch(const std::exception & ex)
	{
		logGlobal->error(ex.what());
	}
	catch(const std::string & ex)
	{
		logGlobal->error(ex);
	}
	catch(...)
	{
		logGlobal->error("Sorry, caught unknown exception type. No more info available.");
	}
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
		int i=0;
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

	//returns first key that maps to given value if present, returns success via found if provided
	template <typename Key, typename T>
	Key findKey(const std::map<Key, T> & map, const T & value, bool * found = nullptr)
	{
		for(auto iter = map.cbegin(); iter != map.cend(); iter++)
		{
			if(iter->second == value)
			{
				if(found)
					*found = true;
				return iter->first;
			}
		}
		if(found)
			*found = false;
		return Key();
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
		if(a >= (t1)b)
			return a;
		else
		{
			a = t1(b);
			return a;
		}
	}

	//assigns smaller of (a, b) to a and returns minimum of (a, b)
	template <typename t1, typename t2>
	t1 &amin(t1 &a, const t2 &b)
	{
		if(a <= (t1)b)
			return a;
		else
		{
			a = t1(b);
			return a;
		}
	}

	//makes a to fit the range <b, c>
	template <typename T>
	void abetween(T &value, const T &min, const T &max)
	{
		value = std::clamp(value, min, max);
	}

	//checks if a is between b and c
	template <typename t1, typename t2, typename t3>
	bool isbetween(const t1 &value, const t2 &min, const t3 &max)
	{
		return value > (t1)min && value < (t1)max;
	}

	//checks if a is within b and c
	template <typename t1, typename t2, typename t3>
	bool iswithin(const t1 &value, const t2 &min, const t3 &max)
	{
		return value >= (t1)min && value <= (t1)max;
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

	//deleted pointer and sets it to nullptr
	template <typename T>
	void clear_pointer(T* &ptr)
	{
		delete ptr;
		ptr = nullptr;
	}

	template <typename Container>
	typename Container::const_reference circularAt(const Container &r, size_t index)
	{
		assert(r.size());
		index %= r.size();
		auto itr = std::begin(r);
		std::advance(itr, index);
		return *itr;
	}

	template <typename Container, typename Item>
	void erase(Container &c, const Item &item)
	{
		c.erase(boost::remove(c, item), c.end());
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
		return std::copy_if(std::cbegin(input), std::end(input), result, pred);
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
		/* Clang crashes when instantiating this function template and having PCH compilation enabled.
		 * There is a bug report here: http://llvm.org/bugs/show_bug.cgi?id=18744
		 * Current bugfix is to don't use a typedef for decltype(*std::begin(rng)) and to use decltype
		 * directly for both function parameters.
		 */
		return boost::min_element(rng, [&] (decltype(*std::begin(rng)) lhs, decltype(*std::begin(rng)) rhs) -> bool
		{
			return vf(lhs) < vf(rhs);
		});
	}

	//Returns iterator to the element for which the value of ValueFunction is maximal
	template<class ForwardRange, class ValueFunction>
	auto maxElementByFun(const ForwardRange& rng, ValueFunction vf) -> decltype(std::begin(rng))
	{
		/* Clang crashes when instantiating this function template and having PCH compilation enabled.
		 * There is a bug report here: http://llvm.org/bugs/show_bug.cgi?id=18744
		 * Current bugfix is to don't use a typedef for decltype(*std::begin(rng)) and to use decltype
		 * directly for both function parameters.
		 */
		return boost::max_element(rng, [&] (decltype(*std::begin(rng)) lhs, decltype(*std::begin(rng)) rhs) -> bool
		{
			return vf(lhs) < vf(rhs);
		});
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

	template <typename Container>
	typename Container::const_reference atOrDefault(const Container &r, size_t index, const typename Container::const_reference &defaultValue)
	{
		if(index < r.size())
			return r[index];

		return defaultValue;
	}

	template <typename Container, typename Item>
	bool erase_if_present(Container &c, const Item &item)
	{
		auto i = std::find(c.begin(), c.end(), item);
		if (i != c.end())
		{
			c.erase(i);
			return true;
		}

		return false;
	}

	template <typename V, typename Item, typename Item2>
	bool erase_if_present(std::map<Item,V> & c, const Item2 &item)
	{
		auto i = c.find(item);
		if (i != c.end())
		{
			c.erase(i);
			return true;
		}
		return false;
	}

	template<typename T>
	void removeDuplicates(std::vector<T> &vec)
	{
		std::sort(vec.begin(), vec.end());
		vec.erase(std::unique(vec.begin(), vec.end()), vec.end());
	}

	template <typename T>
	void concatenate(std::vector<T> &dest, const std::vector<T> &src)
	{
		dest.reserve(dest.size() + src.size());
		dest.insert(dest.end(), src.begin(), src.end());
	}

	template <typename T>
	std::vector<T> intersection(std::vector<T> &v1, std::vector<T> &v2)
	{
		std::vector<T> v3;
		std::sort(v1.begin(), v1.end());
		std::sort(v2.begin(), v2.end());
		std::set_intersection(v1.begin(), v1.end(), v2.begin(), v2.end(), std::back_inserter(v3));
		return v3;
	}

	template <typename Key, typename V>
	bool containsMapping(const std::multimap<Key,V> & map, const std::pair<const Key,V> & mapping)
	{
		auto range = map.equal_range(mapping.first);
		for(auto contained = range.first; contained != range.second; contained++)
		{
			if(mapping.second == contained->second)
				return true;
		}
		return false;
	}

	template<class M, class Key, class F>
	typename M::mapped_type & getOrCompute(M & m, const Key & k, F f)
	{
		typedef typename M::mapped_type V;

		std::pair<typename M::iterator, bool> r = m.insert(typename M::value_type(k, V()));
		V & v = r.first->second;

		if(r.second)
			f(v);

		return v;
	}

	//c++20 feature
	template<typename Arithmetic, typename Floating>
	Arithmetic lerp(const Arithmetic & a, const Arithmetic & b, const Floating & f)
	{
		return a + (b - a) * f;
	}

	///compile-time version of std::abs for ints for int3, in clang++15 std::abs is constexpr
	static constexpr int abs(int i) {
		if(i < 0) return -i;
		return i;
	}

	///C++23
	template< class Enum > constexpr std::underlying_type_t<Enum> to_underlying( Enum e ) noexcept
	{
		return static_cast<std::underlying_type_t<Enum>>(e);
	}
}
using vstd::operator-=;

VCMI_LIB_NAMESPACE_END
