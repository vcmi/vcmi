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
#include "../serializer/Serializeable.h"

VCMI_LIB_NAMESPACE_BEGIN

struct CampaignBonus;
struct CampaignTravel;
class CGHeroInstance;
class CGameState;
class CMap;

struct CampaignHeroReplacement
{
	CampaignHeroReplacement(std::shared_ptr<CGHeroInstance> hero, const ObjectInstanceID & heroPlaceholderId);
	std::shared_ptr<CGHeroInstance> hero;
	ObjectInstanceID heroPlaceholderId;
	std::vector<ArtifactPosition> transferrableArtifacts;
};

class CGameStateCampaign : public Serializeable
{
	CGameState * gameState = nullptr;

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
	CGameStateCampaign();
	CGameStateCampaign(CGameState * owner);
	void setGamestate(CGameState * owner);

	void placeCampaignHeroes();
	void initStartingResources();
	void initHeroes();
	void initTowns();

	bool playerHasStartingHero(PlayerColor player) const;
	std::unique_ptr<CMap> getCurrentMap();

	template <typename Handler> void serialize(Handler &h)
	{
		if (h.saving || h.hasFeature(Handler::Version::NO_RAW_POINTERS_IN_SERIALIZER))
		{
			// no-op, but needed to auto-create this class if gamestate had it during serialization
		}
		else
		{
			bool dummyA = false;
			uint32_t dummyB = 0;
			uint16_t dummyC = 0;

			h & dummyA;
			h & dummyB;
			h & dummyC;
		}
	}
};

VCMI_LIB_NAMESPACE_END
