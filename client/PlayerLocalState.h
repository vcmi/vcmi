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

#include "../lib/constants/EntityIdentifiers.h"

VCMI_LIB_NAMESPACE_BEGIN

class CGHeroInstance;
class CGTownInstance;
class CArmedInstance;
class JsonNode;
struct CGPath;
class int3;
struct CPathsInfo;

VCMI_LIB_NAMESPACE_END

class CPlayerInterface;

struct PlayerSpellbookSetting
{
	//on which page we left spellbook
	int spellbookLastPageBattle = 0;
	int spellbookLastPageAdvmap = 0;
	SpellSchool spellbookLastTabBattle = SpellSchool::ANY;
	SpellSchool spellbookLastTabAdvmap = SpellSchool::ANY;
};

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

	PlayerSpellbookSetting spellbookSettings;

	SpellID currentSpell;

	void synchronizeState();
public:

	explicit PlayerLocalState(CPlayerInterface & owner);

	bool isHeroSleeping(const CGHeroInstance * hero) const;
	void setHeroAsleep(const CGHeroInstance * hero);
	void setHeroAwaken(const CGHeroInstance * hero);

	const PlayerSpellbookSetting & getSpellbookSettings() const;
	void setSpellbookSettings(const PlayerSpellbookSetting & newSettings);

	const std::vector<const CGTownInstance *> & getOwnedTowns();
	const CGTownInstance * getOwnedTown(size_t index);
	void addOwnedTown(const CGTownInstance * hero);
	void removeOwnedTown(const CGTownInstance * hero);
	void swapOwnedTowns(size_t pos1, size_t pos2);

	const std::vector<const CGHeroInstance *> & getWanderingHeroes();
	const CGHeroInstance * getWanderingHero(size_t index);
	const CGHeroInstance * getNextWanderingHero(const CGHeroInstance * hero);
	void addWanderingHero(const CGHeroInstance * hero);
	void removeWanderingHero(const CGHeroInstance * hero);
	void swapWanderingHero(size_t pos1, size_t pos2);

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

	// returns currently cast spell, if any
	SpellID getCurrentSpell() const;

	void setCurrentSpell(SpellID castedSpell);

	void serialize(JsonNode & dest) const;
	void deserialize(const JsonNode & source);

	/// Changes currently selected object
	void setSelection(const CArmedInstance *sel);
	void setSelection(const CArmedInstance *sel, bool force);
};
