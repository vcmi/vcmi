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

	virtual std::string getJsonKey() const = 0;
	virtual const std::string & getSource() const = 0;
};

class DLL_LINKAGE Pool
{
public:
	virtual ~Pool() = default;

	virtual std::shared_ptr<Context> getContext(const Script * script) const = 0;
};

class DLL_LINKAGE Service
{
public:
	virtual ~Service() = default;

	virtual std::unique_ptr<Pool> createPoolInstance(const Environment * ENV) const = 0;
};

class DLL_LINKAGE Module
{
public:
	virtual ~Module() = default;

	virtual void loadObject(std::string scope, std::string name, const JsonNode & data) = 0;
	virtual void afterLoadFinalization() = 0;

	virtual std::unique_ptr<Pool> createPoolInstance(const Environment * ENV) const = 0;

};

}

VCMI_LIB_NAMESPACE_END
