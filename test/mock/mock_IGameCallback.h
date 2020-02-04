/*
 * mock_IGameCallback.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "../../lib/IGameCallback.h"

//TODO: move/rename PacketSender to better place
#include "../../lib/int3.h"
#include "../../lib/spells/Magic.h"


class GameCallbackMock : public IGameCallback
{
public:
	using UpperCallback = ::spells::PacketSender;

	GameCallbackMock(const UpperCallback * upperCallback_);
	virtual ~GameCallbackMock();

	void setGameState(CGameState * gameState);

	///STUBS, to be removed as long as same methods moved from GameHandler

	//all calls to such methods should be replaced with other object calls or actual netpacks
	//currently they are declared in callbacks, overridden in GameHandler and stubbed in client

	//TODO: fail all stub calls

	void changeSpells(const CGHeroInstance * hero, bool give, const std::set<SpellID> &spells) override {};
	bool removeObject(const CGObjectInstance * obj) override {return false;};
	void setBlockVis(ObjectInstanceID objid, bool bv) override {};
	void setOwner(const CGObjectInstance * objid, PlayerColor owner) override {};
	void changePrimSkill(const CGHeroInstance * hero, PrimarySkill::PrimarySkill which, si64 val, bool abs=false) override {};
	void changeSecSkill(const CGHeroInstance * hero, SecondarySkill which, int val, bool abs=false) override {};
	void showBlockingDialog(BlockingDialog *iw) override {};
	void showGarrisonDialog(ObjectInstanceID upobj, ObjectInstanceID hid, bool removableUnits) override {}; //cb will be called when player closes garrison window
	void showTeleportDialog(TeleportDialog *iw) override {};
	void showThievesGuildWindow(PlayerColor player, ObjectInstanceID requestingObjId) override {};
	void giveResource(PlayerColor player, Res::ERes which, int val) override {};
	void giveResources(PlayerColor player, TResources resources) override {};

	void giveCreatures(const CArmedInstance *objid, const CGHeroInstance * h, const CCreatureSet &creatures, bool remove) override {};
	void takeCreatures(ObjectInstanceID objid, const std::vector<CStackBasicDescriptor> &creatures) override {};
	bool changeStackCount(const StackLocation &sl, TQuantity count, bool absoluteValue = false) override {return false;};
	bool changeStackType(const StackLocation &sl, const CCreature *c) override {return false;};
	bool insertNewStack(const StackLocation &sl, const CCreature *c, TQuantity count = -1) override {return false;}; //count -1 => moves whole stack
	bool eraseStack(const StackLocation &sl, bool forceRemoval = false) override {return false;};
	bool swapStacks(const StackLocation &sl1, const StackLocation &sl2) override {return false;};
	bool addToSlot(const StackLocation &sl, const CCreature *c, TQuantity count) override {return false;}; //makes new stack or increases count of already existing
	void tryJoiningArmy(const CArmedInstance *src, const CArmedInstance *dst, bool removeObjWhenFinished, bool allowMerging) override {}; //merges army from src do dst or opens a garrison window
	bool moveStack(const StackLocation &src, const StackLocation &dst, TQuantity count) override {return false;};

	void removeAfterVisit(const CGObjectInstance *object) override {}; //object will be destroyed when interaction is over. Do not call when interaction is not ongoing!

	void giveHeroNewArtifact(const CGHeroInstance *h, const CArtifact *artType, ArtifactPosition pos) override {};
	void giveHeroArtifact(const CGHeroInstance *h, const CArtifactInstance *a, ArtifactPosition pos) override {}; //pos==-1 - first free slot in backpack=0; pos==-2 - default if available or backpack
	void putArtifact(const ArtifactLocation &al, const CArtifactInstance *a) override {};
	void removeArtifact(const ArtifactLocation &al) override {};
	bool moveArtifact(const ArtifactLocation &al1, const ArtifactLocation &al2) override {return false;};
	void synchronizeArtifactHandlerLists() override {};

	void showCompInfo(ShowInInfobox * comp) override {};
	void heroVisitCastle(const CGTownInstance * obj, const CGHeroInstance * hero) override {};
	void stopHeroVisitCastle(const CGTownInstance * obj, const CGHeroInstance * hero) override {};
	void startBattlePrimary(const CArmedInstance *army1, const CArmedInstance *army2, int3 tile, const CGHeroInstance *hero1, const CGHeroInstance *hero2, std::string creatureBankName = "", const CGTownInstance *town = nullptr) override {}; //use hero=nullptr for no hero
	void startBattleI(const CArmedInstance *army1, const CArmedInstance *army2, int3 tile, std::string creatureBankName = "") override {}; //if any of armies is hero, hero will be used
	void startBattleI(const CArmedInstance *army1, const CArmedInstance *army2, std::string creatureBankName = "") override {}; //if any of armies is hero, hero will be used, visitable tile of second obj is place of battle
	void setAmount(ObjectInstanceID objid, ui32 val) override {};
	bool moveHero(ObjectInstanceID hid, int3 dst, ui8 teleporting, bool transit = false, PlayerColor asker = PlayerColor::NEUTRAL) override {return false;};
	void giveHeroBonus(GiveBonus * bonus) override {};
	void setMovePoints(SetMovePoints * smp) override {};
	void setManaPoints(ObjectInstanceID hid, int val) override {};
	void giveHero(ObjectInstanceID id, PlayerColor player) override {};
	void changeObjPos(ObjectInstanceID objid, int3 newPos, ui8 flags) override {};
	void heroExchange(ObjectInstanceID hero1, ObjectInstanceID hero2) override {}; //when two heroes meet on adventure map
	void changeFogOfWar(int3 center, ui32 radius, PlayerColor player, bool hide) override {};
	void changeFogOfWar(std::unordered_set<int3, ShashInt3> &tiles, PlayerColor player, bool hide) override {};

	///useful callback methods
	void commitPackage(CPackForClient * pack) override;
	void sendAndApply(CPackForClient * pack) override;
private:
	const UpperCallback * upperCallback;
};
