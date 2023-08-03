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

#include "CCreatureSet.h"

VCMI_LIB_NAMESPACE_BEGIN

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
};

struct DLL_LINKAGE InfoAboutArmy
{
	PlayerColor owner;
	std::string name;

	ArmyDescriptor army;

	InfoAboutArmy();
	InfoAboutArmy(const CArmedInstance *Army, bool detailed);

	void initFromArmy(const CArmedInstance *Army, bool detailed);
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
	};

	Details * details = nullptr;

	const CHeroClass *hclass;
	int portrait;

	enum EInfoLevel
	{
		BASIC,
		DETAILED,
		INBATTLE
	};

	InfoAboutHero();
	InfoAboutHero(const InfoAboutHero & iah);
	InfoAboutHero(const CGHeroInstance *h, EInfoLevel infoLevel);
	~InfoAboutHero();

	InfoAboutHero & operator=(const InfoAboutHero & iah);

	void initFromHero(const CGHeroInstance *h, EInfoLevel infoLevel);
};

/// Struct which holds a int information about a town
struct DLL_LINKAGE InfoAboutTown : public InfoAboutArmy
{
	struct DLL_LINKAGE Details
	{
		si32 hallLevel, goldIncome;
		bool customRes;
		bool garrisonedHero;

	} *details;

	const CTown *tType;

	si32 built;
	si32 fortLevel; //0 - none

	InfoAboutTown();
	InfoAboutTown(const CGTownInstance *t, bool detailed);
	~InfoAboutTown();
	void initFromTown(const CGTownInstance *t, bool detailed);
};

VCMI_LIB_NAMESPACE_END
