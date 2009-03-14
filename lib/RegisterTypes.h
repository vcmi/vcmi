#pragma  once
//templates for registering object types

//first set of types - derivatives of CGObjectInstance
template<typename Serializer> DLL_EXPORT void registerTypes1(Serializer &s)
{
	s.registerType<CGHeroInstance>();
	s.registerType<CGTownInstance>();
	s.registerType<CGEvent>();
	s.registerType<CGVisitableOPH>();
	s.registerType<CGVisitableOPW>();
	s.registerType<CGTeleport>();
	s.registerType<CGPickable>();
	s.registerType<CGCreature>();
	s.registerType<CGSignBottle>();
	s.registerType<CGSeerHut>();
	s.registerType<CGWitchHut>();
	s.registerType<CGScholar>();
	s.registerType<CGGarrison>();
	s.registerType<CGArtifact>();
	s.registerType<CGResource>();
	s.registerType<CGMine>();
	s.registerType<CGShrine>();
	s.registerType<CGPandoraBox>();
	s.registerType<CGQuestGuard>();
	s.registerType<CGBonusingObject>();
	s.registerType<CGMagicWell>();
	s.registerType<CGObservatory>();
	s.registerType<CGObjectInstance>();
}


//second set of types - derivatives of CPack for client (network VCMI packages)
template<typename Serializer> DLL_EXPORT void registerTypes2(Serializer &s)
{
	s.registerType<SystemMessage>();
	s.registerType<YourTurn>();
	s.registerType<SetResource>();
	s.registerType<SetResources>();
	s.registerType<SetPrimSkill>();
	s.registerType<SetSecSkill>();
	s.registerType<HeroVisitCastle>();
	s.registerType<ChangeSpells>();
	s.registerType<SetMana>();
	s.registerType<SetMovePoints>();
	s.registerType<FoWChange>();
	s.registerType<SetAvailableHeroes>();
	s.registerType<GiveBonus>();
	s.registerType<ChangeObjPos>();
	s.registerType<RemoveObject>();
	s.registerType<TryMoveHero>();
	s.registerType<SetGarrisons>();
	s.registerType<NewStructures>();
	s.registerType<SetAvailableCreatures>();
	s.registerType<SetHeroesInTown>();
	s.registerType<SetHeroArtifacts>();
	s.registerType<HeroRecruited>();
	s.registerType<GiveHero>();
	s.registerType<NewTurn>();
	s.registerType<InfoWindow>();
	s.registerType<SetObjectProperty>();
	s.registerType<SetHoverName>();
	s.registerType<HeroLevelUp>();
	s.registerType<SelectionDialog>();
	s.registerType<YesNoDialog>();
	s.registerType<BattleStart>();
	s.registerType<BattleNextRound>();
	s.registerType<BattleSetActiveStack>();
	s.registerType<BattleResult>();
	s.registerType<BattleStackMoved>();
	s.registerType<BattleStackAttacked>();
	s.registerType<BattleAttack>();
	s.registerType<StartAction>();
	s.registerType<EndAction>();
	s.registerType<SpellCasted>();
	s.registerType<SetStackEffect>();
	s.registerType<ShowInInfobox>();

	s.registerType<SetSelection>();
	s.registerType<PlayerMessage>();
}

template<typename Serializer> DLL_EXPORT void registerTypes3(Serializer &s)
{
	s.registerType<SaveGame>();
	s.registerType<CloseServer>();
	s.registerType<EndTurn>();
	s.registerType<DismissHero>();
	s.registerType<MoveHero>();
	s.registerType<ArrangeStacks>();
	s.registerType<DisbandCreature>();
	s.registerType<BuildStructure>();
	s.registerType<RecruitCreatures>();
	s.registerType<UpgradeCreature>();
	s.registerType<GarrisonHeroSwap>();
	s.registerType<ExchangeArtifacts>();
	s.registerType<BuyArtifact>();
	s.registerType<TradeOnMarketplace>();
	s.registerType<SetFormation>();
	s.registerType<HireHero>();
	s.registerType<QueryReply>();
	s.registerType<MakeAction>();
	s.registerType<MakeCustomAction>();

	s.registerType<SetSelection>();
	s.registerType<PlayerMessage>();
}

//register all
template<typename Serializer> DLL_EXPORT void registerTypes(Serializer &s)
{
	registerTypes1(s);
	registerTypes2(s);
	registerTypes3(s);
}