#define VCMI_DLL
#include "Connection.h"
#include "NetPacks.h"
#include "VCMI_Lib.h"
#include "../hch/CObjectHandler.h"
#include "../hch/CHeroHandler.h"
#include "../hch/CTownHandler.h"
#include "RegisterTypes.h"

/*
 * RegisterTypes.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

template<typename Serializer> DLL_EXPORT
void registerTypes1(Serializer &s)
{
	s.template registerType<CGHeroPlaceholder>();
	s.template registerType<CGHeroInstance>();
	s.template registerType<CGTownInstance>();
	s.template registerType<CTownBonus>();
	s.template registerType<CGPandoraBox>();
	s.template registerType<CGEvent>();
	s.template registerType<CGDwelling>();
	s.template registerType<CGVisitableOPH>();
	s.template registerType<CGVisitableOPW>();
	s.template registerType<CGTeleport>();
	s.template registerType<CGPickable>();
	s.template registerType<CGCreature>();
	s.template registerType<CGSignBottle>();
	s.template registerType<CGSeerHut>();
	s.template registerType<CGQuestGuard>();
	s.template registerType<CGWitchHut>();
	s.template registerType<CGScholar>();
	s.template registerType<CGGarrison>();
	s.template registerType<CGArtifact>();
	s.template registerType<CGResource>();
	s.template registerType<CGMine>();
	s.template registerType<CGShrine>();
	s.template registerType<CGBonusingObject>();
	s.template registerType<CGMagicSpring>();
	s.template registerType<CGMagicWell>();
	s.template registerType<CGObservatory>();
	s.template registerType<CGKeys>();
	s.template registerType<CGKeymasterTent>();
	s.template registerType<CGBorderGuard>();
	s.template registerType<CGBoat>();
	s.template registerType<CGMagi>();
	s.template registerType<CGSirens>();
	s.template registerType<CGOnceVisitable>();
	s.template registerType<CBank>();
	s.template registerType<CGPyramid>();
	s.template registerType<CGShipyard>();
	s.template registerType<CCartographer>();
	s.template registerType<CGObjectInstance>();
	s.template registerType<COPWBonus>();
}

template<typename Serializer> DLL_EXPORT 
void registerTypes2(Serializer &s)
{
	s.template registerType<PackageApplied>();
	s.template registerType<SystemMessage>();
	s.template registerType<PlayerBlocked>();
	s.template registerType<YourTurn>();
	s.template registerType<SetResource>();
	s.template registerType<SetResources>();
	s.template registerType<SetPrimSkill>();
	s.template registerType<SetSecSkill>();
	s.template registerType<HeroVisitCastle>();
	s.template registerType<ChangeSpells>();
	s.template registerType<SetMana>();
	s.template registerType<SetMovePoints>();
	s.template registerType<FoWChange>();
	s.template registerType<SetAvailableHeroes>();
	s.template registerType<GiveBonus>();
	s.template registerType<ChangeObjPos>();
	s.template registerType<PlayerEndsGame>();
	s.template registerType<RemoveObject>();
	s.template registerType<TryMoveHero>();
	s.template registerType<SetGarrisons>();
	s.template registerType<NewStructures>();
	s.template registerType<RazeStructures>();
	s.template registerType<SetAvailableCreatures>();
	s.template registerType<SetHeroesInTown>();
	s.template registerType<SetHeroArtifacts>();
	s.template registerType<HeroRecruited>();
	s.template registerType<GiveHero>();
	s.template registerType<NewTurn>();
	s.template registerType<InfoWindow>();
	s.template registerType<SetObjectProperty>();
	s.template registerType<SetHoverName>();
	s.template registerType<HeroLevelUp>();
	s.template registerType<BlockingDialog>();
	s.template registerType<GarrisonDialog>();
	s.template registerType<BattleStart>();
	s.template registerType<BattleNextRound>();
	s.template registerType<BattleSetActiveStack>();
	s.template registerType<BattleResult>();
	s.template registerType<BattleStackMoved>();
	s.template registerType<BattleStackAttacked>();
	s.template registerType<BattleAttack>();
	s.template registerType<StartAction>();
	s.template registerType<EndAction>();
	s.template registerType<SpellCast>();
	s.template registerType<SetStackEffect>();
	s.template registerType<StacksInjured>();
	s.template registerType<BattleResultsApplied>();
	s.template registerType<StacksHealedOrResurrected>();
	s.template registerType<ObstaclesRemoved>();
	s.template registerType<CatapultAttack>();
	s.template registerType<BattleStacksRemoved>();
	s.template registerType<ShowInInfobox>();
	s.template registerType<OpenWindow>();
	s.template registerType<NewObject>();

	s.template registerType<SaveGame>();
	s.template registerType<SetSelection>();
	s.template registerType<PlayerMessage>();
	s.template registerType<CenterView>();
}

template<typename Serializer> DLL_EXPORT
void registerTypes3(Serializer &s)
{
	s.template registerType<CloseServer>();
	s.template registerType<EndTurn>();
	s.template registerType<DismissHero>();
	s.template registerType<MoveHero>();
	s.template registerType<ArrangeStacks>();
	s.template registerType<DisbandCreature>();
	s.template registerType<BuildStructure>();
	s.template registerType<RecruitCreatures>();
	s.template registerType<UpgradeCreature>();
	s.template registerType<GarrisonHeroSwap>();
	s.template registerType<ExchangeArtifacts>();
	s.template registerType<BuyArtifact>();
	s.template registerType<TradeOnMarketplace>();
	s.template registerType<SetFormation>();
	s.template registerType<HireHero>();
	s.template registerType<BuildBoat>();
	s.template registerType<QueryReply>();
	s.template registerType<MakeAction>();
	s.template registerType<MakeCustomAction>();

	s.template registerType<SaveGame>();
	s.template registerType<SetSelection>();
	s.template registerType<PlayerMessage>();
}

template<typename Serializer> DLL_EXPORT
void registerTypes(Serializer &s)
{
	registerTypes1(s);
	registerTypes2(s);
	registerTypes3(s);
}
