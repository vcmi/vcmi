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
	std::vector<ArtifactPosition> transferrableArtifacts;
};

class CGameStateCampaign
{
	CGameState * gameState;

	/// Contains list of heroes that may be available in this scenario
	/// temporary helper for game initialization, not serialized
	std::vector<CampaignHeroReplacement> campaignHeroReplacements;

	/// Returns ID of scenario from which hero placeholders should be selected
	std::optional<CampaignScenarioID> getHeroesSourceScenario() const;

	/// returns heroes and placeholders in where heroes will be put
	void generateCampaignHeroesToReplace();

	std::optional<CampaignBonus> currentBonus() const;

	/// Trims hero parameters that should not transfer between scenarios according to travelOptions flags
	void trimCrossoverHeroesParameters(const CampaignTravel & travelOptions);

	void replaceHeroesPlaceholders();
	void transferMissingArtifacts(const CampaignTravel & travelOptions);

	void giveCampaignBonusToHero(CGHeroInstance * hero);

public:
	CGameStateCampaign() = default;
	CGameStateCampaign(CGameState * owner);

	void placeCampaignHeroes();
	void initStartingResources();
	void initHeroes();
	void initTowns();

	bool playerHasStartingHero(PlayerColor player) const;
	std::unique_ptr<CMap> getCurrentMap();

	template <typename Handler> void serialize(Handler &h)
	{
		h & gameState;
	}
};

VCMI_LIB_NAMESPACE_END
