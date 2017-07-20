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

#define VCMI_REGISTER_SPELL_EFFECT(Type, Name) \
namespace\
{\
RegisterEffect<Type> register ## Type(Name);\
}\
\

namespace spells
{
namespace effects
{

class IEffectFactory
{
public:
	IEffectFactory() = default;
	virtual ~IEffectFactory() = default;

	virtual Effect * create() const = 0;
};

class Registry
{
public:
	virtual ~Registry();
	virtual const IEffectFactory * find(const std::string & name) const = 0;
	virtual void add(const std::string & name, IEffectFactory * item) = 0;

    static Registry * get();
protected:
	Registry();
};

template<typename E>
class EffectFactory : public IEffectFactory
{
public:
	EffectFactory() = default;
	virtual ~EffectFactory() = default;

	Effect * create() const override
	{
		return new E();
	}
};

template<typename E>
class RegisterEffect
{
public:
	RegisterEffect(const std::string & name)
	{
		IEffectFactory * f = new EffectFactory<E>();
		Registry::get()->add(name, f);
	}
};

}
}

