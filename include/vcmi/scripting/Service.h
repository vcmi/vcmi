/*
 * scripting/Service.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#if SCRIPTING_ENABLED
#include <vcmi/Environment.h>

VCMI_LIB_NAMESPACE_BEGIN

class Services;
class JsonNode;
class ServerCallback;

namespace scripting
{

using BattleCb = Environment::BattleCb;
using GameCb = Environment::GameCb;

class DLL_LINKAGE Context
{
public:
	virtual ~Context() = default;

	virtual void run(const JsonNode & initialState) = 0;
	virtual void run(ServerCallback * server, const JsonNode & initialState) = 0;

	virtual JsonNode callGlobal(const std::string & name, const JsonNode & parameters) = 0;
	virtual JsonNode callGlobal(ServerCallback * server, const std::string & name, const JsonNode & parameters) = 0;

	virtual void setGlobal(const std::string & name, int value) = 0;
	virtual void setGlobal(const std::string & name, const std::string & value) = 0;
	virtual void setGlobal(const std::string & name, double value) = 0;
	virtual void setGlobal(const std::string & name, const JsonNode & value) = 0;

	virtual void getGlobal(const std::string & name, int & value) = 0;
	virtual void getGlobal(const std::string & name, std::string & value) = 0;
	virtual void getGlobal(const std::string & name, double & value) = 0;
	virtual void getGlobal(const std::string & name, JsonNode & value) = 0;

	virtual JsonNode saveState() = 0;
};

class DLL_LINKAGE Script
{
public:
	virtual ~Script() = default;

	virtual const std::string & getName() const = 0;
	virtual const std::string & getSource() const = 0;

	virtual std::shared_ptr<Context> createContext(const Environment * env) const = 0;
};

class DLL_LINKAGE Pool
{
public:
	virtual ~Pool() = default;

	virtual void serializeState(const bool saving, JsonNode & data) = 0;

	virtual std::shared_ptr<Context> getContext(const Script * script) = 0;
};

class DLL_LINKAGE Service
{
public:
	virtual ~Service() = default;

	virtual void performRegistration(Services * services) const = 0;
	virtual void run(std::shared_ptr<Pool> pool) const = 0;
};


}

VCMI_LIB_NAMESPACE_END
#endif
