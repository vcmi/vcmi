/*
 * PlayerLocalState.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

VCMI_LIB_NAMESPACE_BEGIN

class CGHeroInstance;
class CGTownInstance;
class CArmedInstance;
struct CGPath;
class int3;

VCMI_LIB_NAMESPACE_END

class CPlayerInterface;

/// Class that contains potentially serializeable state of a local player
class PlayerLocalState
{
	CPlayerInterface & owner;

	/// Currently selected object, can be town, hero or null
	const CArmedInstance * currentSelection;

	std::map<const CGHeroInstance *, CGPath> paths; //maps hero => selected path in adventure map
	std::vector<const CGHeroInstance *> sleepingHeroes; //if hero is in here, he's sleeping
	std::vector<const CGHeroInstance *> wanderingHeroes; //our heroes on the adventure map (not the garrisoned ones)
	std::vector<const CGTownInstance *> ownedTowns; //our towns on the adventure map

	void saveHeroPaths(std::map<const CGHeroInstance *, int3> & paths);
	void loadHeroPaths(std::map<const CGHeroInstance *, int3> & paths);

public:
	struct SpellbookLastSetting
	{
		//on which page we left spellbook
		int spellbookLastPageBattle = 0;
		int spellbokLastPageAdvmap = 0;
		int spellbookLastTabBattle = 4;
		int spellbookLastTabAdvmap = 4;

		template<typename Handler>
		void serialize(Handler & h, const int version)
		{
			h & spellbookLastPageBattle;
			h & spellbokLastPageAdvmap;
			h & spellbookLastTabBattle;
			h & spellbookLastTabAdvmap;
		}
	} spellbookSettings;

	explicit PlayerLocalState(CPlayerInterface & owner);

	bool isHeroSleeping(const CGHeroInstance * hero) const;
	void setHeroAsleep(const CGHeroInstance * hero);
	void setHeroAwaken(const CGHeroInstance * hero);

	const std::vector<const CGTownInstance *> & getOwnedTowns();
	const CGTownInstance * getOwnedTown(size_t index);
	void addOwnedTown(const CGTownInstance * hero);
	void removeOwnedTown(const CGTownInstance * hero);

	const std::vector<const CGHeroInstance *> & getWanderingHeroes();
	const CGHeroInstance * getWanderingHero(size_t index);
	const CGHeroInstance * getNextWanderingHero(const CGHeroInstance * hero);
	void addWanderingHero(const CGHeroInstance * hero);
	void removeWanderingHero(const CGHeroInstance * hero);

	void setPath(const CGHeroInstance * h, const CGPath & path);
	bool setPath(const CGHeroInstance * h, const int3 & destination);

	const CGPath & getPath(const CGHeroInstance * h) const;
	bool hasPath(const CGHeroInstance * h) const;

	void removeLastNode(const CGHeroInstance * h);
	void erasePath(const CGHeroInstance * h);
	void verifyPath(const CGHeroInstance * h);

	/// Returns currently selected object
	const CGHeroInstance * getCurrentHero() const;
	const CGTownInstance * getCurrentTown() const;
	const CArmedInstance * getCurrentArmy() const;

	/// Changes currently selected object
	void setSelection(const CArmedInstance *sel);

	template<typename Handler>
	void serialize(Handler & h, int version)
	{
		//WARNING: this code is broken and not used. See CClient::loadGame
		std::map<const CGHeroInstance *, int3> pathsMap; //hero -> dest
		if(h.saving)
			saveHeroPaths(pathsMap);

		h & pathsMap;

		if(!h.saving)
			loadHeroPaths(pathsMap);

		h & ownedTowns;
		h & wanderingHeroes;
		h & sleepingHeroes;
	}
};
