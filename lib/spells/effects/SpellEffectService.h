/*
 * SpellEffectHandler.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "../../constants/EntityIdentifiers.h"

VCMI_LIB_NAMESPACE_BEGIN

class JsonNode;

namespace spells::effects
{

class Effect;

class DLL_LINKAGE ISpellEffectFactory
{
public:
	/// (modScope, sourcePath) pair identifying one patch script layer.
	using PatchEntry = std::pair<std::string, std::string>;

	virtual ~ISpellEffectFactory() = default;

	/// effectId is the unique identifier of the effect (e.g. "core:damage"), used as cache key.
	/// scope/name point at the base script; patches list extra layers stacked over the base in order.
	virtual void initialize(const std::string & effectId,
		const std::string & scope, const std::string & name,
		const std::vector<PatchEntry> & patches) = 0;
	virtual std::shared_ptr<Effect> create(const std::string & effectId) const = 0;
};

class DLL_LINKAGE SpellEffectService : boost::noncopyable
{
public:
	virtual ~SpellEffectService() = default;
	virtual std::shared_ptr<Effect> create(SpellEffectID effectID) const = 0;
	virtual void registerFactory(const std::string & typeName, std::shared_ptr<ISpellEffectFactory> factory) = 0;
	virtual void prepareEffect(SpellEffectID effectID, JsonNode & data, const std::string & spellScope, const std::string & spellIdentifier, const std::string & effectName) const = 0;

};

}

VCMI_LIB_NAMESPACE_END
