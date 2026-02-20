/*
 * CHeroClass.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include <vcmi/HeroClass.h>

#include "../../constants/EntityIdentifiers.h"
#include "../../constants/Enumerations.h"
#include "../../filesystem/ResourcePath.h"

VCMI_LIB_NAMESPACE_BEGIN

namespace vstd
{
class RNG;
}

class DLL_LINKAGE CHeroClass : public HeroClass
{
	friend class CHeroClassHandler;
	HeroClassID id; // use getId instead
	std::string modScope;
	std::string identifier; // use getJsonKey instead

public:
	enum EClassAffinity
	{
		MIGHT,
		MAGIC
	};

	//double aggression; // not used in vcmi.
	FactionID faction;
	ui8 affinity; // affinity, using EClassAffinity enum

	// default chance for hero of specific class to appear in tavern, if field "tavern" was not set
	// resulting chance = sqrt(town.chance * heroClass.chance)
	ui32 defaultTavernChance;

	CreatureID commander;

	std::vector<int> primarySkillInitial; // initial primary skills
	std::vector<int> primarySkillLowLevel; // probability (%) of getting point of primary skill when getting level
	std::vector<int> primarySkillHighLevel; // same for high levels (> 10)

	std::map<SecondarySkill, int> secSkillProbability; //probabilities of gaining secondary skills (out of 112), in id order

	std::map<FactionID, int> selectionProbability; //probability of selection in towns

	AnimationPath imageBattleMale;
	AnimationPath imageBattleFemale;
	std::string imageMapMale;
	std::string imageMapFemale;

	CHeroClass();

	int32_t getIndex() const override;
	int32_t getIconIndex() const override;
	std::string getJsonKey() const override;
	std::string getModScope() const override;
	HeroClassID getId() const override;
	void registerIcons(const IconRegistar & cb) const override;

	std::string getNameTranslated() const override;
	std::string getNameTextID() const override;

	bool isMagicHero() const;

	void updateFrom(const JsonNode & data);
	void serializeJson(JsonSerializeFormat & handler);

	EAlignment getAlignment() const;

	int tavernProbability(FactionID faction) const;
};

VCMI_LIB_NAMESPACE_END
