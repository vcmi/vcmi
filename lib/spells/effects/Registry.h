/*
 * Registry.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "Effect.h"

VCMI_LIB_NAMESPACE_BEGIN

namespace spells
{
namespace effects
{

class DLL_LINKAGE IEffectFactory
{
public:
	virtual ~IEffectFactory() = default;

	virtual Effect * create() const = 0;
};

class DLL_LINKAGE Registry
{
public:
	using FactoryPtr = std::shared_ptr<IEffectFactory>;

	virtual ~Registry() = default; //Required for child classes
	virtual const IEffectFactory * find(const std::string & name) const = 0;
	virtual void add(const std::string & name, FactoryPtr item) = 0;
};

class DLL_LINKAGE GlobalRegistry
{
public:
    static Registry * get();
};

template<typename E>
class EffectFactory : public IEffectFactory
{
public:
	Effect * create() const override
	{
		return new E();
	}
};

}
}


VCMI_LIB_NAMESPACE_END
