/*
* CBankInstanceConstructor.h, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/
#pragma once

#include "CDefaultObjectTypeHandler.h"
#include "IObjectInfo.h"

#include "../CCreatureSet.h"
#include "../ResourceSet.h"
#include "../json/JsonNode.h"
#include "../mapObjects/CBank.h"

VCMI_LIB_NAMESPACE_BEGIN

struct BankConfig
{
	ui32 chance = 0; //chance for this level being chosen
	std::vector<CStackBasicDescriptor> guards; //creature ID, amount
	ResourceSet resources; //resources given in case of victory
	std::vector<CStackBasicDescriptor> creatures; //creatures granted in case of victory (creature ID, amount)
	std::vector<ArtifactID> artifacts; //artifacts given in case of victory
	std::vector<SpellID> spells; // granted spell(s), for Pyramid

	template <typename Handler> void serialize(Handler &h)
	{
		h & chance;
		h & guards;
		h & resources;
		h & creatures;
		h & artifacts;
		h & spells;
	}
};

using TPossibleGuards = std::vector<std::pair<ui8, IObjectInfo::CArmyStructure>>;

template <typename T>
struct DLL_LINKAGE PossibleReward
{
	int chance;
	T data;

	PossibleReward(int chance, const T & data) : chance(chance), data(data) {}
};

class DLL_LINKAGE CBankInfo : public IObjectInfo
{
	const JsonVector & config;
public:
	CBankInfo(const JsonVector & Config);

	TPossibleGuards getPossibleGuards(IGameCallback * cb) const;
	std::vector<PossibleReward<TResources>> getPossibleResourcesReward() const;
	std::vector<PossibleReward<CStackBasicDescriptor>> getPossibleCreaturesReward(IGameCallback * cb) const;

	bool givesResources() const override;
	bool givesArtifacts() const override;
	bool givesCreatures() const override;
	bool givesSpells() const override;
};

class CBankInstanceConstructor : public CDefaultObjectTypeHandler<CBank>
{
	BankConfig generateConfig(IGameCallback * cb, const JsonNode & conf, CRandomGenerator & rng) const;

	JsonVector levels;

	// all banks of this type will be reset N days after clearing,
	si32 bankResetDuration = 0;

	// bank is only visitable from adjacent tile
	bool blockVisit;
	// bank is visitable from land even when bank is on water tile
	bool coastVisitable;
protected:
	void initTypeData(const JsonNode & input) override;

public:

	void randomizeObject(CBank * object, CRandomGenerator & rng) const override;

	bool hasNameTextID() const override;

	std::unique_ptr<IObjectInfo> getObjectInfo(std::shared_ptr<const ObjectTemplate> tmpl) const override;
};

VCMI_LIB_NAMESPACE_END
