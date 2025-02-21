/*
 * CGResource.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "CArmedInstance.h"

VCMI_LIB_NAMESPACE_BEGIN

class ResourceInstanceConstructor;

class DLL_LINKAGE CGResource : public CArmedInstance
{
	friend class Inspector;
	friend class CMapLoaderH3M;
	friend class ResourceInstanceConstructor;

	MetaString message;

	static constexpr uint32_t RANDOM_AMOUNT = 0;
	uint32_t amount = RANDOM_AMOUNT; //0 if random

	std::shared_ptr<ResourceInstanceConstructor> getResourceHandler() const;
	int getAmountMultiplier() const;
	void collectRes(const PlayerColor & player) const;
	void serializeJsonOptions(JsonSerializeFormat & handler) override;

public:
	using CArmedInstance::CArmedInstance;

	void onHeroVisit(const CGHeroInstance * h) const override;
	void initObj(vstd::RNG & rand) override;
	void pickRandomObject(vstd::RNG & rand) override;
	void battleFinished(const CGHeroInstance *hero, const BattleResult &result) const override;
	void blockingDialogAnswered(const CGHeroInstance *hero, int32_t answer) const override;
	std::string getHoverText(PlayerColor player) const override;

	GameResID resourceID() const;
	uint32_t getAmount() const;

	template <typename Handler> void serialize(Handler &h)
	{
		h & static_cast<CArmedInstance&>(*this);
		h & amount;
		h & message;
	}
};

VCMI_LIB_NAMESPACE_END
