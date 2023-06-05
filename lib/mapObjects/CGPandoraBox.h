/*
 * CGPandoraBox.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "CArmedInstance.h"
#include "../ResourceSet.h"

VCMI_LIB_NAMESPACE_BEGIN

struct InfoWindow;

class DLL_LINKAGE CGPandoraBox : public CArmedInstance
{
public:
	std::string message;
	mutable bool hasGuardians = false; //helper - after battle even though we have no stacks, allows us to know that there was battle

	//gained things:
	ui32 gainedExp = 0;
	si32 manaDiff = 0; //amount of gained / lost mana
	si32 moraleDiff = 0; //morale modifier
	si32 luckDiff = 0; //luck modifier
	TResources resources;//gained / lost resources
	std::vector<si32> primskills;//gained / lost prim skills
	std::vector<SecondarySkill> abilities; //gained abilities
	std::vector<si32> abilityLevels; //levels of gained abilities
	std::vector<ArtifactID> artifacts; //gained artifacts
	std::vector<SpellID> spells; //gained spells
	CCreatureSet creatures; //gained creatures

	void initObj(CRandomGenerator & rand) override;
	void onHeroVisit(const CGHeroInstance * h) const override;
	void battleFinished(const CGHeroInstance *hero, const BattleResult &result) const override;
	void blockingDialogAnswered(const CGHeroInstance *hero, ui32 answer) const override;
	void heroLevelUpDone(const CGHeroInstance *hero) const override;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<CArmedInstance&>(*this);
		h & message;
		h & hasGuardians;
		h & gainedExp;
		h & manaDiff;
		h & moraleDiff;
		h & luckDiff;
		h & resources;
		h & primskills;
		h & abilities;
		h & abilityLevels;
		h & artifacts;
		h & spells;
		h & creatures;
	}
protected:
	void giveContentsUpToExp(const CGHeroInstance *h) const;
	void giveContentsAfterExp(const CGHeroInstance *h) const;
	void serializeJsonOptions(JsonSerializeFormat & handler) override;
private:
	void getText( InfoWindow &iw, bool &afterBattle, int val, int negative, int positive, const CGHeroInstance * h ) const;
	void getText( InfoWindow &iw, bool &afterBattle, int text, const CGHeroInstance * h ) const;
	virtual void afterSuccessfulVisit() const;
};

class DLL_LINKAGE CGEvent : public CGPandoraBox  //event objects
{
public:
	bool removeAfterVisit = false; //true if event is removed after occurring
	ui8 availableFor = 0; //players whom this event is available for
	bool computerActivate = false; //true if computer player can activate this event
	bool humanActivate = false; //true if human player can activate this event

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<CGPandoraBox &>(*this);
		h & removeAfterVisit;
		h & availableFor;
		h & computerActivate;
		h & humanActivate;
	}

	void onHeroVisit(const CGHeroInstance * h) const override;
protected:
	void serializeJsonOptions(JsonSerializeFormat & handler) override;
private:
	void activated(const CGHeroInstance * h) const;
	void afterSuccessfulVisit() const override;
};

VCMI_LIB_NAMESPACE_END
