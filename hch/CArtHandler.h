#ifndef __CARTHANDLER_H__
#define __CARTHANDLER_H__
#include "../global.h"
#include "../lib/HeroBonus.h"
#include <list>
#include <string>
#include <vector>

/*
 * CArtHandler.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
class CDefHandler;

class DLL_EXPORT CArtifact //container for artifacts
{
	std::string name, description; //set if custom
public:
	enum EartClass {ART_SPECIAL=1, ART_TREASURE=2, ART_MINOR=4, ART_MAJOR=8, ART_RELIC=16}; //artifact classes
	const std::string &Name() const; //getter
	const std::string &Description() const; //getter

	ui32 price;
	std::vector<ui16> possibleSlots; //ids of slots where artifact can be placed
	EartClass aClass;
	ui32 id;
	std::list<HeroBonus> bonuses; //bonuses given by artifact

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & name & description & price & possibleSlots & aClass & id & bonuses;
	}
};

class DLL_EXPORT CArtHandler //handles artifacts
{
	void giveArtBonus(int aid, HeroBonus::BonusType type, int val, int subtype = -1);
public:
	std::vector<CArtifact*> treasures, minors, majors, relics;
	std::vector<CArtifact> artifacts;

	void loadArtifacts(bool onlyTxt);
	void sortArts();
	void addBonuses();
	void clear();
	static int convertMachineID(int id, bool creToArt);
	CArtHandler();

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & artifacts;
		if(!h.saving)
			sortArts();
	}
};


#endif // __CARTHANDLER_H__
