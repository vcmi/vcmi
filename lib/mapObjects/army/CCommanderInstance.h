/*
 * CCommanderInstance.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "CStackInstance.h"

class DLL_LINKAGE CCommanderInstance : public CStackInstance
{
public:
	//TODO: what if Commander is not a part of creature set?

	//commander class is determined by its base creature
	ui8 alive; //maybe change to bool when breaking save compatibility?
	ui32 level = 1;
	std::string name; // each Commander has different name
	std::vector<ui8> secondarySkills; //ID -> level
	std::set<ui8> specialSkills;
	//std::vector <CArtifactInstance *> arts;
	CCommanderInstance(IGameInfoCallback * cb);
	CCommanderInstance(IGameInfoCallback * cb, const CreatureID & id);
	void setAlive(bool alive);
	void levelUp();
	/// Gains one level and learns a random skill from available choices
	void levelUpAutomatically(vstd::RNG & rand);
	/// Skills that can be offered on level-up: indices of secondary skills, or 100 + index of special skill
	std::vector<int> getLevelUpSkillChoices() const;
	/// Bonuses granted by learning next level of given skill
	std::vector<Bonus> getSkillBonuses(int skill) const;

	bool canGainExperience() const override;
	bool gainsLevel() const; //true if commander has lower level than should upon his experience
	ui64 getPower() const override
	{
		return 0;
	};
	/// Commander can not be traded away as part of an army, so it has no value here
	ui64 estimateCombatValue() const override
	{
		return 0;
	};
	int getExpRank() const override;
	int getLevel() const override;
	ArtBearer bearerType() const override; //from CArtifactSet

	template<typename Handler>
	void serialize(Handler & h)
	{
		h & static_cast<CStackInstance &>(*this);
		h & alive;
		if(h.hasFeature(Handler::Version::HOTA_HERO_LEVEL_OPTIONS))
			h & level;
		else
		{
			ui8 legacyLevel = level;
			h & legacyLevel;
			level = legacyLevel;
		}
		h & name;
		h & secondarySkills;
		h & specialSkills;
	}
};
