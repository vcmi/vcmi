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
#include "../campaign/CampaignConstants.h"

VCMI_LIB_NAMESPACE_BEGIN

struct CampaignBonus;
struct CampaignTravel;
class CGHeroInstance;
class CGameState;
class CMap;

struct CampaignHeroReplacement
{
	CampaignHeroReplacement(CGHeroInstance * hero, const ObjectInstanceID & heroPlaceholderId);
	CGHeroInstance * hero;
	ObjectInstanceID heroPlaceholderId;
};

class CGameStateCampaign
{
	CGameState * gameState;

	/// Returns ID of scenario from which hero placeholders should be selected
	std::optional<CampaignScenarioID> getHeroesSourceScenario() const;

	/// returns heroes and placeholders in where heroes will be put
	std::vector<CampaignHeroReplacement> generateCampaignHeroesToReplace();

	std::optional<CampaignBonus> currentBonus() const;

	/// Trims hero parameters that should not transfer between scenarios according to travelOptions flags
	void trimCrossoverHeroesParameters(std::vector<CampaignHeroReplacement> & campaignHeroReplacements, const CampaignTravel & travelOptions);

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
	std::unique_ptr<CMap> getCurrentMap() const;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & gameState;
	}
};

VCMI_LIB_NAMESPACE_END
