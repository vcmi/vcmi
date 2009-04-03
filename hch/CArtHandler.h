#ifndef __CARTHANDLER_H__
#define __CARTHANDLER_H__
#include "../global.h"
#include "../lib/HeroBonus.h"
#include <list>
#include <string>
#include <vector>

enum EartClass {SartClass=0, TartClass, NartClass, JartClass, RartClass}; //artifact class (relict, treasure, strong, weak etc.)
class CDefHandler;

class DLL_EXPORT CArtifact //container for artifacts
{
	std::string name, description; //set if custom
public:
	const std::string &Name() const;
	const std::string &Description() const;

	ui32 price;
	std::vector<ui16> possibleSlots; //ids of slots where artifact can be placed
	EartClass aClass;
	int id;
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
