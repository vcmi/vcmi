/*
 * CallbackDefines.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

class CallbackInvalidCallException : public std::runtime_error
{
public:
	CallbackInvalidCallException(const std::string & functionName)
		:std::runtime_error(functionName + "called with invalid parameters!")
	{}
};

class CallbackOutsideBattleCalledException : public std::runtime_error
{
public:
	CallbackOutsideBattleCalledException(const std::string & functionName)
		:std::runtime_error(functionName + "called when no battle!")
	{}
};

#define THROW_INVALID_CALL() do { logGlobal->error("%s called with invalid parameters!", BOOST_CURRENT_FUNCTION); throw CallbackInvalidCallException( BOOST_CURRENT_FUNCTION ); } while (false)
#define THROW_IF(cond, txt) do { if(cond) {logGlobal->error("invalid call to %s: %s", BOOST_CURRENT_FUNCTION, txt); throw CallbackInvalidCallException( std::string(BOOST_CURRENT_FUNCTION) + ": " + txt + " - " ); } } while (false)
#define THROW_IF_CALLED_WITH_PLAYER() do { if(!getPlayerID()) {logGlobal->error("%s called with invalid player!", BOOST_CURRENT_FUNCTION); throw CallbackInvalidCallException( BOOST_CURRENT_FUNCTION ); } } while (false)
#define THROW_IF_NOT_BATTLE() do { if(!duringBattle()) {logGlobal->error("%s called when no battle!", BOOST_CURRENT_FUNCTION); throw CallbackOutsideBattleCalledException( BOOST_CURRENT_FUNCTION ); } } while (false)
