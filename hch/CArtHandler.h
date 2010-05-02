#ifndef __CARTHANDLER_H__
#define __CARTHANDLER_H__
#include "../global.h"
#include "../lib/HeroBonus.h"
#include <set>
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
	bool isBig () const;
	bool fitsAt (const std::map<ui16, ui32> &artifWorn, ui16 slot) const;
	bool canBeAssembledTo (const std::map<ui16, ui32> &artifWorn, ui32 artifactID) const;

	ui32 price;
	std::vector<ui16> possibleSlots; //ids of slots where artifact can be placed
	std::vector<ui32> * constituents; // Artifacts IDs a combined artifact consists of, or NULL.
	std::vector<ui32> * constituentOf; // Reverse map of constituents.
	EartClass aClass;
	ui32 id;
	std::list<Bonus> bonuses; //bonuses given by artifact

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & name & description & price & possibleSlots & constituents & constituentOf & aClass & id & bonuses;
	}
};

class DLL_EXPORT CArtHandler //handles artifacts
{
	void giveArtBonus(int aid, Bonus::BonusType type, int val, int subtype = -1, int valType = Bonus::BASE_NUMBER);
public:
	std::vector<CArtifact*> treasures, minors, majors, relics;
	std::vector<CArtifact> artifacts;
	std::set<ui32> bigArtifacts; // Artifacts that cannot be moved to backpack, e.g. war machines.

	void loadArtifacts(bool onlyTxt);
	void sortArts();
	void addBonuses();
	void clear();
	bool isBigArtifact (ui32 artID) {return bigArtifacts.find(artID) != bigArtifacts.end();}
	void equipArtifact (std::map<ui16, ui32> &artifWorn, ui16 slotID, ui32 artifactID);
	void unequipArtifact (std::map<ui16, ui32> &artifWorn, ui16 slotID);
	static int convertMachineID(int id, bool creToArt);
	CArtHandler();
	~CArtHandler();

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & artifacts;
		if(!h.saving)
			sortArts();
	}
};


#endif // __CARTHANDLER_H__
