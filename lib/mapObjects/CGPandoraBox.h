#pragma once

#include "CObjectHandler.h"
#include "CArmedInstance.h"
#include "../ResourceSet.h"

/*
 * CGPandoraBox.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

struct InfoWindow;

class DLL_LINKAGE CGPandoraBox : public CArmedInstance
{
public:
	std::string message;
	mutable bool hasGuardians; //helper - after battle even though we have no stacks, allows us to know that there was battle

	//gained things:
	ui32 gainedExp;
	si32 manaDiff; //amount of gained / lost mana
	si32 moraleDiff; //morale modifier
	si32 luckDiff; //luck modifier
	TResources resources;//gained / lost resources
	std::vector<si32> primskills;//gained / lost prim skills
	std::vector<SecondarySkill> abilities; //gained abilities
	std::vector<si32> abilityLevels; //levels of gained abilities
	std::vector<ArtifactID> artifacts; //gained artifacts
	std::vector<SpellID> spells; //gained spells
	CCreatureSet creatures; //gained creatures

	CGPandoraBox() : gainedExp(0), manaDiff(0), moraleDiff(0), luckDiff(0){};
	void initObj() override;
	void onHeroVisit(const CGHeroInstance * h) const override;
	void battleFinished(const CGHeroInstance *hero, const BattleResult &result) const override;
	void blockingDialogAnswered(const CGHeroInstance *hero, ui32 answer) const override;
	void heroLevelUpDone(const CGHeroInstance *hero) const override;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<CArmedInstance&>(*this);
		h & message & hasGuardians & gainedExp & manaDiff & moraleDiff & luckDiff & resources & primskills
			& abilities & abilityLevels & artifacts & spells & creatures;
	}
protected:
	void giveContentsUpToExp(const CGHeroInstance *h) const;
	void giveContentsAfterExp(const CGHeroInstance *h) const;
private:
	void getText( InfoWindow &iw, bool &afterBattle, int val, int negative, int positive, const CGHeroInstance * h ) const;
	void getText( InfoWindow &iw, bool &afterBattle, int text, const CGHeroInstance * h ) const;
	virtual void afterSuccessfulVisit() const;
};

class DLL_LINKAGE CGEvent : public CGPandoraBox  //event objects
{
public:
	bool removeAfterVisit; //true if event is removed after occurring
	ui8 availableFor; //players whom this event is available for
	bool computerActivate; //true if computer player can activate this event
	bool humanActivate; //true if human player can activate this event

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<CGPandoraBox &>(*this);
		h & removeAfterVisit & availableFor & computerActivate & humanActivate;
	}

	CGEvent() : CGPandoraBox(){};
	void onHeroVisit(const CGHeroInstance * h) const override;
private:
	void activated(const CGHeroInstance * h) const;
	void afterSuccessfulVisit() const override;
};
