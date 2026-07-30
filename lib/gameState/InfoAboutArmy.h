/*
 * InfoAboutArmy.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../mapObjects/army/CStackBasicDescriptor.h"

class CGTownInstance;
class CHeroClass;
class CTown;

//numbers of creatures are exact numbers if detailed else they are quantity ids (1 - a few, 2 - several and so on; additionally 0 - unknown)
struct ArmyDescriptor : public std::map<SlotID, CStackBasicDescriptor>
{
	bool isDetailed;
	DLL_LINKAGE ArmyDescriptor(const CArmedInstance *army, bool detailed); //not detailed -> quantity ids as count
	DLL_LINKAGE ArmyDescriptor();

	DLL_LINKAGE int getStrength() const;

	bool operator==(const ArmyDescriptor & other) const = default;
};

struct DLL_LINKAGE InfoAboutArmy
{
	PlayerColor owner;
	std::string name;

	ArmyDescriptor army;

	InfoAboutArmy();
	InfoAboutArmy(const CArmedInstance *Army, bool detailed);

	void initFromArmy(const CArmedInstance *Army, bool detailed);

	bool operator==(const InfoAboutArmy & other) const = default;
};

struct DLL_LINKAGE InfoAboutHero : public InfoAboutArmy
{
private:
	void assign(const InfoAboutHero & iah);

public:
	struct DLL_LINKAGE Details
	{
		std::vector<si32> primskills;
		si32 mana, manaLimit, luck, morale;

		bool operator==(const Details & other) const = default;
	};

	std::optional<Details> details;

	const CHeroClass *hclass;
	HeroTypeID portraitSource;

	enum EInfoLevel
	{
		BASIC,
		DETAILED,
		INBATTLE
	};

	InfoAboutHero();
	InfoAboutHero(const InfoAboutHero & iah);
	InfoAboutHero(const CGHeroInstance *h, EInfoLevel infoLevel);

	InfoAboutHero & operator=(const InfoAboutHero & iah);

	void initFromHero(const CGHeroInstance *h, EInfoLevel infoLevel);
	int32_t getIconIndex() const;

	/// Defaulted on purpose: UI code compares the value it last rendered against a freshly built
	/// one, so adding a member above extends that check automatically
	bool operator==(const InfoAboutHero & other) const = default;
};

/// Struct which holds a int information about a town
struct DLL_LINKAGE InfoAboutTown : public InfoAboutArmy
{
	struct DLL_LINKAGE Details
	{
		si32 hallLevel, goldIncome;
		bool customRes;
		bool garrisonedHero;

	};

	std::optional<Details> details;

	const CTown *tType;

	si32 built;
	si32 fortLevel; //0 - none

	InfoAboutTown();
	InfoAboutTown(const CGTownInstance *t, bool detailed);
	void initFromTown(const CGTownInstance *t, bool detailed);
};
