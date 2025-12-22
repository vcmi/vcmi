/*
 * base.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include <any>
#include <string>
#include <vector>

#include <boost/core/demangle.hpp>
#include <boost/format.hpp>

// Import + Export macro declarations
// If MMAI_DLL is defined => this header is imported from VCMI's MMAI lib.
// Otherwise, it is imported from vcmi-gym.
#if defined(_WIN32)
#	if defined(__GNUC__)
#		define MMAI_IMPORT __attribute__((dllimport))
#		define MMAI_EXPORT __attribute__((dllexport))
#	else
#		define MMAI_IMPORT __declspec(dllimport)
#		define MMAI_EXPORT __declspec(dllexport)
#	endif
#	ifndef ELF_VISIBILITY
#		define ELF_VISIBILITY
#	endif
#else
#	ifdef __GNUC__
#		define MMAI_IMPORT __attribute__((visibility("default")))
#		define MMAI_EXPORT __attribute__((visibility("default")))
#		ifndef ELF_VISIBILITY
#			define ELF_VISIBILITY __attribute__((visibility("default")))
#		endif
#	endif
#endif

#ifdef MMAI_DLL
#	define MMAI_DLL_LINKAGE MMAI_EXPORT
#else
#	define MMAI_DLL_LINKAGE MMAI_IMPORT
#endif

namespace MMAI::Schema
{
#define EI(enum_value) static_cast<int>(enum_value)

using Action = int;
using BattlefieldState = std::vector<float>;
using ActionMask = std::vector<bool>;
using AttentionMask = std::vector<float>;

// Same control actions for all versions
constexpr Action ACTION_RETREAT = 0;
constexpr Action ACTION_RESET = -1;
constexpr Action ACTION_RENDER_ANSI = -2;

class IState
{
public:
	virtual const ActionMask * getActionMask() const = 0;
	virtual const AttentionMask * getAttentionMask() const = 0;
	virtual const BattlefieldState * getBattlefieldState() const = 0;

	// Supplementary data may differ across versions => expose it as std::any
	// XXX: ensure the real data type has MMAI_DLL_LINKAGE to prevent std::any_cast errors
	virtual std::any getSupplementaryData() const = 0;

	virtual int version() const = 0;
	virtual ~IState() = default;
};

enum class ModelType : int
{
	SCRIPTED, // e.g. BattleAI, StupidAI
	NN, // pre-trained models stored in a file
	_count
};

enum class Side : int
{
	LEFT, // BattleSide::LEFT
	RIGHT, // BattleSide::RIGHT
	BOTH // for models able to play as either left or right
};

class IModel
{
public:
	virtual ModelType getType() = 0;
	virtual std::string getName() = 0;
	virtual int getVersion() = 0;
	virtual int getAction(const IState *) = 0;
	virtual double getValue(const IState *) = 0;
	virtual Side getSide() = 0;

	virtual ~IModel() = default;
};

// Convenience formatter for std::any cast errors
inline std::string AnyCastError(const std::any & any, const std::type_info & t)
{
	if(!any.has_value())
	{
		return "no value";
	}
	else if(any.type() != t)
	{
		return boost::str(
			boost::format("type mismatch: want: %s/%u, have: %s/%u") % boost::core::demangle(t.name()) % t.hash_code()
			% boost::core::demangle(any.type().name()) % any.type().hash_code()
		);
	}
	else
	{
		return "";
	}
}
}
