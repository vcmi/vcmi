/*
 * CGameStateCampaign.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../GameConstants.h"

VCMI_LIB_NAMESPACE_BEGIN

class CScenarioTravel;
class CGHeroInstance;
class CGameState;
class CMap;

struct CampaignHeroReplacement
{
	CampaignHeroReplacement(CGHeroInstance * hero, const ObjectInstanceID & heroPlaceholderId);
	CGHeroInstance * hero;
	ObjectInstanceID heroPlaceholderId;
};

struct CrossoverHeroesList
{
	std::vector<CGHeroInstance *> heroesFromPreviousScenario;
	std::vector<CGHeroInstance *> heroesFromAnyPreviousScenarios;
	void addHeroToBothLists(CGHeroInstance * hero);
	void removeHeroFromBothLists(CGHeroInstance * hero);
};

class CGameStateCampaign
{
	CGameState * gameState;

	CrossoverHeroesList getCrossoverHeroesFromPreviousScenarios() const;

	/// returns heroes and placeholders in where heroes will be put
	std::vector<CampaignHeroReplacement> generateCampaignHeroesToReplace(CrossoverHeroesList & crossoverHeroes);

	/// Trims hero parameters that should not transfer between scenarios according to travelOptions flags
	void trimCrossoverHeroesParameters(std::vector<CampaignHeroReplacement> & campaignHeroReplacements, const CScenarioTravel & travelOptions);

	void replaceHeroesPlaceholders(const std::vector<CampaignHeroReplacement> & campaignHeroReplacements);

	void giveCampaignBonusToHero(CGHeroInstance * hero);

public:
	CGameStateCampaign() = default;
	CGameStateCampaign(CGameState * owner);

	void placeCampaignHeroes();
	void initStartingResources();
	void initHeroes();
	void initTowns();

	bool playerHasStartingHero(PlayerColor player) const;
	CMap * getCurrentMap() const;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & gameState;
	}
};

VCMI_LIB_NAMESPACE_END
